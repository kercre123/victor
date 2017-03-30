/**
 * File: pathComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-03-28
 *
 * Description: Component that handles path planning and following
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/components/pathComponent.h"

#include "anki/cozmo/basestation/faceAndApproachPlanner.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "anki/cozmo/basestation/minimalAnglePlanner.h"
#include "anki/cozmo/basestation/pathDolerOuter.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/speedChooser.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "util/logging/logging.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"

// TODO:(bn) not pound defines
#define MAX_DISTANCE_FOR_SHORT_PLANNER 40.0f
#define MAX_DISTANCE_TO_PREDOCK_POSE 20.0f
#define MIN_DISTANCE_FOR_MINANGLE_PLANNER 1.0f

namespace Anki {
namespace Cozmo {

struct FallbackPlannerPoses
{
  Pose3d driveCenterPose;
  std::vector<Pose3d> targetPoses;
};

PathComponent::PathComponent(Robot& robot, const RobotID_t robotID, const CozmoContext* context)
  : _speedChooser(new SpeedChooser(robot))
  , _shortPathPlanner(new FaceAndApproachPlanner)
  , _shortMinAnglePathPlanner(new MinimalAnglePlanner)
  , _currentPlannerGoal(new Pose3d)
  , _pathMotionProfile(new PathMotionProfile(DEFAULT_PATH_MOTION_PROFILE))
  , _fallbackPoseInfo(new FallbackPlannerPoses)
  , _robot(robot)
{
  if( context ) {
    // might not exist (e.g. unit tests)
    _pdo.reset(new PathDolerOuter(context->GetRobotManager()->GetMsgHandler(), robotID));
  }

  if (nullptr != context->GetDataPlatform()) {
    _longPathPlanner.reset(new LatticePlanner(&robot, context->GetDataPlatform()));
  }
  else {
    // For unit tests, or cases where we don't have data, use the short planner in it's place
    PRINT_NAMED_WARNING("Robot.NoDataPlatform.WrongPlanner",
                        "Using short planner as the long planner, since we dont have a data platform");
    _longPathPlanner = _shortPathPlanner;
  }

  // select a planner by default, but don't set a fallback
  _selectedPathPlanner = _longPathPlanner;

}

PathComponent::~PathComponent()
{
  Abort();
}

Result PathComponent::Abort()
{
  if( _selectedPathPlanner ) {
    _selectedPathPlanner->StopPlanning();
  }
  Result ret = ClearPath();
  _numPlansFinished = _numPlansStarted;

  return ret;
}

void PathComponent::UpdateRobotData(s8 currPathSegment, u16 lastRecvdPathID)
{
  SetLastRecvdPathID(lastRecvdPathID);
  SetCurrPathSegment(currPathSegment);

  // Dole out more path segments to the physical robot if needed:
  if (_pdo && IsTraversingPath() && GetLastRecvdPathID() == GetLastSentPathID()) {
    _pdo->Update(_currPathSegment);
  }
}

void PathComponent::Update()
{
  if( _driveToPoseStatus != ERobotDriveToPoseStatus::Waiting ) {

    bool forceReplan = _driveToPoseStatus == ERobotDriveToPoseStatus::Error;

    if( _numPlansFinished == _numPlansStarted ) {
      // nothing to do with the planners, so just update the status based on the path following
      if( IsTraversingPath() ) {
        _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;

        // haveOriginsChanged: note that we check if the parent of the currentPlannerGoal is different than the world
        // origin. With origin rejiggering it's possible that our origin is the current world origin, as long as we
        // could rejigger our parent to the new origin. If we could not rejigger, then our parent is also not the
        // origin, but moreover we can't get the goal with respect to the new origin
        const bool haveOriginsChanged = (_currentPlannerGoal->GetParent() != _robot.GetWorldOrigin());
        const bool canAdjustOrigin = _currentPlannerGoal->GetWithRespectTo(*_robot.GetWorldOrigin(), *_currentPlannerGoal);
        if ( haveOriginsChanged && !canAdjustOrigin )
        {
          // the origins changed and we can't rejigger our goal to the new origin (we probably delocalized),
          // completely abort
          _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
          Abort();
          PRINT_CH_INFO("Planner", "PathComponent.Update.Replan.NotPossible",
                        "Our goal is in another coordinate frame that we can't get wrt current, we can't replan");
        }
        else {
          // check if we need adjusting origins
          if (haveOriginsChanged && canAdjustOrigin)
          {
            // our origin changed, but we were able to recompute _currentPlannerGoal wrt current origin. Abort
            // the current plan and start driving to the updated _currentPlannerGoal. Note this can discard
            // other goals for multiple goal requests, but it's likely that the closest goal is still the
            // closest one, and this is a faster fix (rsam 2016)
            Abort();
            PRINT_CH_INFO("Planner", "PathComponent.Update.Replan.RejiggeringPlanner",
                          "Our goal is in another coordinate frame, but we are updating to current frame");
            const Result planningResult = StartDrivingToPose( *_currentPlannerGoal, *_pathMotionProfile );
            if ( planningResult != RESULT_OK ) {
              PRINT_NAMED_WARNING("PathComponent.Update.Replan.RejiggeringPlanner",
                                  "We could not start driving to rejiggered pose.");
              // We failed to replan, abort
              Abort();
              _driveToPoseStatus = ERobotDriveToPoseStatus::Error; // reset in case StartDriving didn't set it
            }
          }
          else if( _robot.GetBlockWorld().DidObjectsChange() || forceReplan )
          {
            // we didn't need to adjust origins
            // see if we need to replan, but only bother checking if the world objects changed
            _fallbackShouldForceReplan = forceReplan;
            RestartPlannerIfNeeded(forceReplan);
          }
        }
      }
      else {
        _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
      }
    }
    else {
      // we are waiting on a plan to currently compute
      // TODO:(bn) timeout logic might fit well here?
      switch( _selectedPathPlanner->CheckPlanningStatus() ) {
        case EPlannerStatus::Error:
          if( nullptr != _fallbackPathPlanner ) {
            _selectedPathPlanner = _fallbackPathPlanner;
            _fallbackPathPlanner.reset();
            _numPlansFinished = _numPlansStarted;
            PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Error",
                             "Running planner returned error status, using fallback planner instead");
            if( !StartPlannerWithFallbackPoses() ) {
              _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
              Abort();
              _numPlansFinished = _numPlansStarted;
            } else {
              if( IsTraversingPath() ) {
                _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
              }
              else {
                _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
              }
              _numPlansStarted++;
            }
          } else {
            _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
            PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Error", "Running planner returned error status");
            Abort();
            _numPlansFinished = _numPlansStarted;
          }
          break;

        case EPlannerStatus::Running:
          // status should stay the same, but double check it
          if( _driveToPoseStatus != ERobotDriveToPoseStatus::ComputingPath &&
              _driveToPoseStatus != ERobotDriveToPoseStatus::Replanning) {
            PRINT_NAMED_WARNING("PathComponent.Update.StatusError.Running",
                                "Status was invalid, setting to ComputePath");
            _driveToPoseStatus =  ERobotDriveToPoseStatus::ComputingPath;
          }
          break;

        case EPlannerStatus::CompleteWithPlan: {
          // get the path
          Planning::GoalID selectedPoseIdx;
          Planning::Path newPath;

          _selectedPathPlanner->GetCompletePath(_robot.GetDriveCenterPose(),
                                                newPath,
                                                selectedPoseIdx,
                                                _pathMotionProfile.get());
          
          // check for collisions
          bool collisionsAcceptable = _selectedPathPlanner->ChecksForCollisions() || newPath.GetNumSegments()==0;
          // Some children of IPathPlanner may return a path that hasn't been checked for obstacles.
          // Here, check if the planner used to compute that path considers obstacles. If it doesnt,
          // check for an obstacle penalty. If there is one, re-run with the lattice planner,
          // which considers obstacles in its search.
          if( (!collisionsAcceptable) && (nullptr != _longPathPlanner) ) {
            const float startPoseAngle = _robot.GetPose().GetRotationAngle<'Z'>().ToFloat();
            const bool __attribute__((unused)) obstaclesLoaded = _longPathPlanner->PreloadObstacles();
            DEV_ASSERT(obstaclesLoaded, "Lattice planner didn't preload obstacles.");
            if( !_longPathPlanner->CheckIsPathSafe(newPath, startPoseAngle) ) {
              // bad path. try with the fallback planner if possible
              if( nullptr != _fallbackPathPlanner ) {
                _selectedPathPlanner = _fallbackPathPlanner;
                _fallbackPathPlanner.reset();
                _numPlansFinished = _numPlansStarted;
                PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Collisions",
                              "Planner returned a path with obstacles, using fallback planner instead");
                if( !StartPlannerWithFallbackPoses() ) {
                  _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
                  Abort();
                  _numPlansFinished = _numPlansStarted;
                } else {
                  if( IsTraversingPath() ) {
                    _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
                  }
                  else {
                    _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
                  }
                  _numPlansStarted++;
                }
              } else {
                // we only have a path with collisions, abort
                PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteNoCollisionFreePlan",
                              "A plan was found, but it contains collisions");
                _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
                _numPlansFinished = _numPlansStarted;
              }
            } else {
              collisionsAcceptable = true;
            }
          }
          
          if( collisionsAcceptable ) {
            _numPlansFinished = _numPlansStarted;
            if( newPath.GetNumSegments()==0 ) {
              _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
              PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteWithPlan.EmptyPlan",
                            "Planner completed but with an empty plan");
            } else {
              _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
              PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteWithPlan",
                            "Running planner complete with a plan");
            
              ExecutePath(newPath, _usingManualPathSpeed);

              if( _plannerSelectedPoseIndexPtr != nullptr ) {
                // When someone called StartDrivingToPose with multiple possible poses, they had an option to pass
                // in a pointer to be set when we know which pose was selected by the planner. If that pointer is
                // non-null, set it now, then clear the pointer so we won't set it again

                // TODO:(bn) think about re-planning, here, what if replanning wanted to switch targets? For now,
                // replanning will always chose the same target pose, which should be OK for now
                *_plannerSelectedPoseIndexPtr = selectedPoseIdx;
                _plannerSelectedPoseIndexPtr = nullptr;
              }
            }
          }
          break;
        }

        case EPlannerStatus::CompleteNoPlan:
          PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteNoPlan",
                        "Running planner complete with no plan");
          _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
          _numPlansFinished = _numPlansStarted;
          break;
      }
    }
  }
}

void PathComponent::SelectPlanner(const Pose3d& targetPose)
{
  // set our current planner goal to the given pose flattened out, so that we can later compare if the origin
  // has changed since we started planning towards this pose. Also because the planner doesn't know about
  // pose origins, so this pose should be wrt origin always
  *_currentPlannerGoal = targetPose.GetWithRespectToOrigin();

  Pose2d target2d(*_currentPlannerGoal);
  Pose2d start2d(_robot.GetPose().GetWithRespectToOrigin());

  float distSquared = pow(target2d.GetX() - start2d.GetX(), 2) + pow(target2d.GetY() - start2d.GetY(), 2);

  if(distSquared < MAX_DISTANCE_FOR_SHORT_PLANNER * MAX_DISTANCE_FOR_SHORT_PLANNER) {

    Radians finalAngleDelta = _currentPlannerGoal->GetRotationAngle<'Z'>()
      - _robot.GetDriveCenterPose().GetRotationAngle<'Z'>();
    const bool withinFinalAngleTolerance = finalAngleDelta.getAbsoluteVal().ToFloat() <=
      2 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

    Radians initialTurnAngle = atan2( target2d.GetY() - _robot.GetDriveCenterPose().GetTranslation().y(),
                                      target2d.GetX() - _robot.GetDriveCenterPose().GetTranslation().x()) -
      _robot.GetDriveCenterPose().GetRotationAngle<'Z'>();

    const bool initialTurnAngleLarge = initialTurnAngle.getAbsoluteVal().ToFloat() >
      0.5 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

    const bool farEnoughAwayForMinAngle = distSquared > std::pow( MIN_DISTANCE_FOR_MINANGLE_PLANNER, 2);

    // if we would need to turn fairly far, but our current angle is fairly close to the goal, use the
    // planner which backs up first to minimize the turn
    if( withinFinalAngleTolerance && initialTurnAngleLarge && farEnoughAwayForMinAngle ) {
      PRINT_CH_INFO("Planner", "PathComponent.SelectPlanner.ShortMinAngle",
                    "distance^2 is %f, angleDelta is %f, intiialTurnAngle is %f, selecting short min_angle planner",
                    distSquared,
                    finalAngleDelta.getAbsoluteVal().ToFloat(),
                    initialTurnAngle.getAbsoluteVal().ToFloat());
      _selectedPathPlanner = _shortMinAnglePathPlanner;
    }
    else {
      PRINT_CH_INFO("Planner", "PathComponent.SelectPlanner.Short",
                    "distance^2 is %f, angleDelta is %f, intiialTurnAngle is %f, selecting short planner",
                    distSquared,
                    finalAngleDelta.getAbsoluteVal().ToFloat(),
                    initialTurnAngle.getAbsoluteVal().ToFloat());
      _selectedPathPlanner = _shortPathPlanner;
    }
  }
  else {
    PRINT_CH_INFO("Planner", "PathComponent.SelectPlanner.Long",
                  "distance^2 is %f, selecting long planner", distSquared);
    _selectedPathPlanner = _longPathPlanner;
  }
  
  if( _selectedPathPlanner != _longPathPlanner ) {
    _fallbackPathPlanner = _longPathPlanner;
  } else {
    _fallbackPathPlanner.reset();
  }
}

void PathComponent::SelectPlanner(const std::vector<Pose3d>& targetPoses)
{
  if( ! targetPoses.empty() ) {
    Planning::GoalID closest = IPathPlanner::ComputeClosestGoalPose(_robot.GetDriveCenterPose(), targetPoses);
    SelectPlanner(targetPoses[closest]);
  }
}

Result PathComponent::StartDrivingToPose(const Pose3d& targetPose,
                                         const PathMotionProfile& motionProfile,
                                         bool useManualSpeed)
{
  _usingManualPathSpeed = useManualSpeed;

  Pose3d targetPoseWrtOrigin;
  if(targetPose.GetWithRespectTo(*_robot.GetWorldOrigin(), targetPoseWrtOrigin) == false) {
    PRINT_NAMED_WARNING("PathComponent.StartDrivingToPose.OriginMisMatch",
                        "Could not get target pose w.r.t. robot %d's origin.", _robot.GetID());
    _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
    return RESULT_FAIL;
  }

  SelectPlanner(targetPoseWrtOrigin);

  // Compute drive center pose of given target robot pose
  Pose3d targetDriveCenterPose;
  _robot.ComputeDriveCenterPose(targetPoseWrtOrigin, targetDriveCenterPose);

  const bool somePlannerSucceeded = StartPlanner(_robot.GetDriveCenterPose(), {{targetDriveCenterPose}});
  if( !somePlannerSucceeded ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
    return RESULT_FAIL;
  }

  if( IsTraversingPath() ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
  }
  else {
    _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
  }

  _numPlansStarted++;
      
  *_pathMotionProfile = motionProfile;

  return RESULT_OK;

  // TODO:(bn) re-use the vector version below instead of copying the code here
}

Result PathComponent::StartDrivingToPose(const std::vector<Pose3d>& poses,
                                         const PathMotionProfile& motionProfile,
                                         Planning::GoalID* selectedPoseIndexPtr,
                                         bool useManualSpeed)
{
  _usingManualPathSpeed = useManualSpeed;
  _plannerSelectedPoseIndexPtr = selectedPoseIndexPtr;

  SelectPlanner(poses);

  // Compute drive center pose for start pose and goal poses
  std::vector<Pose3d> targetDriveCenterPoses(poses.size());
  for (int i=0; i< poses.size(); ++i) {
    _robot.ComputeDriveCenterPose(poses[i], targetDriveCenterPoses[i]);
  }

  const bool somePlannerSucceeded = StartPlanner(_robot.GetDriveCenterPose(), targetDriveCenterPoses);
  if( !somePlannerSucceeded ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
    return RESULT_FAIL;
  }

  if( IsTraversingPath() ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
  }
  else {
    _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
  }

  _numPlansStarted++;

  *_pathMotionProfile = motionProfile;
      
  return RESULT_OK;
}

bool PathComponent::StartPlannerWithFallbackPoses()
{
  if( !_fallbackPoseInfo->targetPoses.empty() ) {
    return StartPlanner(_fallbackPoseInfo->driveCenterPose, _fallbackPoseInfo->targetPoses);
  } else { // treat it as an error
    PRINT_NAMED_ERROR("Robot.Update.Planner.Error", "Could not restart planner, missing fallback target poses");
    return false;
  }
}

bool PathComponent::StartPlanner(const Pose3d& driveCenterPose, const std::vector<Pose3d>& targetDriveCenterPoses)
{
  // save start and target poses in case this run fails and we need to try again
  _fallbackPoseInfo->driveCenterPose = driveCenterPose;
  _fallbackPoseInfo->targetPoses = targetDriveCenterPoses;
  
  EComputePathStatus status = _selectedPathPlanner->ComputePath(driveCenterPose, targetDriveCenterPoses);
  if( status == EComputePathStatus::Error ) {
    // try again with the fallback, if it exists
    if( nullptr != _fallbackPathPlanner ) {
      _selectedPathPlanner = _fallbackPathPlanner;
      _fallbackPathPlanner.reset();
      if( EComputePathStatus::Error != _selectedPathPlanner->ComputePath(driveCenterPose, targetDriveCenterPoses) ) {
        return true;
      }
    }
    return false;
  }
  return true;
}

void PathComponent::RestartPlannerIfNeeded(bool forceReplan)
{
  assert(_selectedPathPlanner);
  assert(_numPlansStarted == _numPlansFinished);
  
  switch( _selectedPathPlanner->ComputeNewPathIfNeeded( _robot.GetDriveCenterPose(), forceReplan ) ) {
    case EComputePathStatus::Error:
      _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
      Abort();
      PRINT_CH_INFO("Planner", "PathComponent.Replan.Fail", "ComputeNewPathIfNeeded returned failure!");
      break;
      
    case EComputePathStatus::Running:
      _numPlansStarted++;
      PRINT_CH_DEBUG("Planner", "PathComponent.Replan.Running", "ComputeNewPathIfNeeded running");
      _driveToPoseStatus = ERobotDriveToPoseStatus::Replanning;
      break;
      
    case EComputePathStatus::NoPlanNeeded:
      // leave status as following, don't update plan attempts since no new planning is needed
      break;
  }
}

// Clears the path that the robot is executing which also stops the robot
Result PathComponent::ClearPath()
{
  _robot.GetContext()->GetVizManager()->ErasePath(_robot.GetID());
  if(_pdo) {
    _pdo->ClearPath();
  }
  
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::ClearPath(0)));
}

// Sends a path to the robot to be immediately executed
Result PathComponent::ExecutePath(const Planning::Path& path, const bool useManualSpeed)
{
  Result lastResult = RESULT_FAIL;
      
  if (path.GetNumSegments() == 0) {
    PRINT_NAMED_WARNING("PathComponent.ExecutePath.EmptyPath", "");
    lastResult = RESULT_OK;
  } else {
        
    // TODO: Clear currently executing path or write to buffered path?
    lastResult = ClearPath();
    if(lastResult == RESULT_OK) {
      ++_lastSentPathID;
      if( _pdo ) {
        _pdo->SetPath(path);
      }
      _usingManualPathSpeed = useManualSpeed;

      PRINT_CH_INFO("Planner", "PathComponent.SendExecutePath",
                    "sending start execution message (pathID = %d, manualSpeed == %d)",
                    _lastSentPathID, useManualSpeed);
      lastResult = _robot.SendMessage(RobotInterface::EngineToRobot(
                                        RobotInterface::ExecutePath(_lastSentPathID, useManualSpeed)));
    }
        
    // Visualize path if robot has just started traversing it.
    _robot.GetContext()->GetVizManager()->DrawPath(_robot.GetID(), path, NamedColors::EXECUTED_PATH);
        
  }
      
  return lastResult;
}

void PathComponent::ExecuteTestPath(const PathMotionProfile& motionProfile)
{
  Planning::Path p;
  _longPathPlanner->GetTestPath(_robot.GetPose(), p, &motionProfile);
  ExecutePath(p, false);
}

}
}

