/**
 * File: cozmoActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/cozmoActions.h"

#include "bridge.h"
#include "pathPlanner.h"
#include "anki/cozmo/basestation/ramp.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/basestation/cozmoEngineConfig.h"

namespace Anki {
  
  namespace Cozmo {
    
    // TODO: Define this as a constant parameter elsewhere
    const Radians DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE(DEG_TO_RAD(7.5));
    
    // Right before docking, the dock object must have been visually verified
    // no more than this many milliseconds ago or it will not even attempt to dock.
    const u32 DOCK_OBJECT_LAST_OBSERVED_TIME_THRESH_MS = 1000;
 
    // Helper function for computing the distance-to-preActionPose threshold,
    // given how far robot is from actionObject
    f32 ComputePreActionPoseDistThreshold(const Robot& robot,
                                          const ActionableObject* actionObject,
                                          const Radians& preActionPoseAngleTolerance)
    {
      assert(actionObject != nullptr);
      
      if(preActionPoseAngleTolerance > 0.f) {
        // Compute distance threshold to preaction pose based on distance to the
        // object: the further away, the more slop we're allowed.
        Pose3d objectWrtRobot;
        if(false == actionObject->GetPose().GetWithRespectTo(robot.GetPose(), objectWrtRobot)) {
          PRINT_NAMED_ERROR("IDockAction.Init.ObjectPoseOriginProblem",
                            "Could not get object %d's pose w.r.t. robot.\n",
                            actionObject->GetID().GetValue());
          return -1.f;
        }
        
        const f32 objectDistance = objectWrtRobot.GetTranslation().Length();
        const f32 preActionPoseDistThresh = objectDistance * std::sin(preActionPoseAngleTolerance.ToFloat());
        
        PRINT_NAMED_INFO("IDockAction.Init.DistThresh",
                         "At a distance of %.1fmm, will use pre-dock pose distance threshold of %.1fmm\n",
                         objectDistance, preActionPoseDistThresh);
        
        return preActionPoseDistThresh;
      } else {
        return -1.f;
      }
    }
    
#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction(const bool useManualSpeed) //, const Pose3d& pose)
    : _isGoalSet(false)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    , _useManualSpeed(useManualSpeed)
    , _startedTraversingPath(false)
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(const Pose3d& pose, const bool useManualSpeed)
    : DriveToPoseAction(useManualSpeed)
    {
      SetGoal(pose);
    }
    
    Result DriveToPoseAction::SetGoal(const Anki::Pose3d& pose)
    {
      _goalPose = pose;
      
      PRINT_NAMED_INFO("DriveToPoseAction.SetGoal",
                       "Setting pose goal to (%.1f,%.1f,%.1f) @ %.1fdeg\n",
                       _goalPose.GetTranslation().x(),
                       _goalPose.GetTranslation().y(),
                       _goalPose.GetTranslation().z(),
                       RAD_TO_DEG(_goalPose.GetRotationAngle<'Z'>().ToFloat()));
      
      _isGoalSet = true;
      
      return RESULT_OK;
    }
    
    Result DriveToPoseAction::SetGoal(const Pose3d& pose,
                                      const Point3f& distThreshold,
                                      const Radians& angleThreshold)
    {
      _goalDistanceThreshold = distThreshold;
      _goalAngleThreshold = angleThreshold;
      
      return SetGoal(pose);
    }
    
    const std::string& DriveToPoseAction::GetName() const
    {
      static const std::string name("DriveToPoseAction");
      return name;
    }
    
    IAction::ActionResult DriveToPoseAction::Init(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      _startedTraversingPath = false;
      
      if(!_isGoalSet) {
        PRINT_NAMED_ERROR("DriveToPoseAction.CheckPreconditions.NoGoalSet",
                          "Goal must be set before running this action.\n");
        result = FAILURE_ABORT;
      }
      else {
        Planning::Path p;
        
        // TODO: Make it possible to set the speed/accel somewhere?
        if(robot.MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 2.f, 5.f) != RESULT_OK) {
          PRINT_NAMED_ERROR("DriveToPoseAction.Init", "Failed to move head to path-following angle.\n");
          result = FAILURE_ABORT;
        }
        else if(robot.GetPathToPose(_goalPose, p) != RESULT_OK) {
          PRINT_NAMED_ERROR("DriveToPoseAction.Init", "Failed to get path to goal pose.\n");
          result = FAILURE_ABORT;
        }
        else if(robot.ExecutePath(p, _useManualSpeed) != RESULT_OK) {
          PRINT_NAMED_ERROR("DriveToPoseAction.Init", "Failed calling execute path.\n");
          result = FAILURE_ABORT;
        }
      }
      
      return result;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckIfDone(Robot& robot)
    {
      IAction::ActionResult result = IAction::RUNNING;
      
      if(!_startedTraversingPath) {
        // Wait until robot reports it has started traversing the path
        _startedTraversingPath = robot.IsTraversingPath();
        
      } else {
        // Wait until robot reports it is no longer traversing a path
        if(robot.IsTraversingPath())
        {
          // If the robot is traversing a path, consider replanning it
          if(robot.GetBlockWorld().DidObjectsChange())
          {
            Planning::Path newPath;
            
            switch(robot.GetPathPlanner()->GetPlan(newPath, robot.GetDriveCenterPose(), _forceReplanOnNextWorldChange))
            {
              case IPathPlanner::DID_PLAN:
              {
                // clear path, but flag that we are replanning
              robot.ClearPath();
              _forceReplanOnNextWorldChange = false;
              
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.UpdatePath", "sending new path to robot\n");
              robot.ExecutePath(newPath, _useManualSpeed);
              break;
            } // case DID_PLAN:
              
            case IPathPlanner::PLAN_NEEDED_BUT_GOAL_FAILURE:
            {
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.NewGoalForReplanNeeded",
                               "Replan failed due to bad goal. Aborting path.\n");
              
              robot.ClearPath();
              break;
            } // PLAN_NEEDED_BUT_GOAL_FAILURE:
              
            case IPathPlanner::PLAN_NEEDED_BUT_START_FAILURE:
            {
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.NewStartForReplanNeeded",
                               "Replan failed during docking due to bad start. Will try again, and hope robot moves.\n");
              break;
            }
              
            case IPathPlanner::PLAN_NEEDED_BUT_PLAN_FAILURE:
            {
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.NewEnvironmentForReplanNeeded",
                               "Replan failed during docking due to a planner failure. Will try again, and hope environment changes.\n");
              // clear the path, but don't change the state
              robot.ClearPath();
              _forceReplanOnNextWorldChange = true;
              break;
            }
              
            default:
            {
              // Don't do anything just proceed with the current plan...
              break;
            }
                
              
            } // switch(GetPlan())
          } // if blocks changed
          
        } else {
          // No longer traversing the path, so check to see if we ended up in the right place
          Vec3f Tdiff;
          
          // HACK: Loosen z threshold bigtime:
          const Point3f distanceThreshold(_goalDistanceThreshold.x(),
                                          _goalDistanceThreshold.y(),
                                          robot.GetHeight());
          
          if(robot.GetPose().IsSameAs(_goalPose, distanceThreshold, _goalAngleThreshold, Tdiff))
          {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Success",
                             "Robot %d successfully finished following path (Tdiff=%.1fmm).\n",
                             robot.GetID(), Tdiff.Length());
            
            result = IAction::SUCCESS;
          }
          // The last path sent was definitely received by the robot
          // and it is no longer executing it, but we appear to not be in position
          else if (robot.GetLastSentPathID() == robot.GetLastRecvdPathID()) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.DoneNotInPlace",
                             "Robot is done traversing path, but is not in position (dist=%.1fmm). lastPathID=%d\n",
                             Tdiff.Length(), robot.GetLastRecvdPathID());
            result = IAction::FAILURE_RETRY;
          }
          else {
            // Something went wrong: not in place and robot apparently hasn't
            // received all that it should have
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Failure",
                             "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false.\n");
            result = IAction::FAILURE_ABORT;
          }
        }
      }
      
      // If we are not running anymore, for any reason, clear the path and its
      // visualization
      if(result != IAction::RUNNING) {
        robot.ClearPath(); // clear path and indicate that we are not replanning
        VizManager::getInstance()->ErasePath(robot.GetID());
        VizManager::getInstance()->EraseAllPlannerObstacles(true);
        VizManager::getInstance()->EraseAllPlannerObstacles(false);
      }
      
      return result;
    } // CheckIfDone()
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType, const bool useManualSpeed)
    : DriveToPoseAction(useManualSpeed)
    , _objectID(objectID)
    , _actionType(actionType)
    , _alreadyAtGoal(false)
    {
      // NOTE: _goalPose will be set later, when we check preconditions
    }
    
    
    const std::string& DriveToObjectAction::GetName() const
    {
      static const std::string name("DriveToObjectAction");
      return name;
    }
    
    
    IAction::ActionResult DriveToObjectAction::InitHelper(Robot& robot, ActionableObject* object)
    {
      ActionResult result = RUNNING;
      
      std::vector<PreActionPose> possiblePreActionPoses;
      object->GetCurrentPreActionPoses(possiblePreActionPoses, {_actionType}, std::set<Vision::Marker::Code>(), &robot.GetPose());
      
      if(possiblePreActionPoses.empty()) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoPreActionPoses",
                          "ActionableObject %d did not return any pre-action poses with action type %d.\n",
                          _objectID.GetValue(), _actionType);
        
        result = FAILURE_ABORT;
        
      } else {
        size_t selectedIndex = 0;
        
        // Check to see if we already close enough to a pre-action pose that we can
        // just skip path planning. In case multiple pre-action poses are close
        // enough, keep the closest one.
        // Also make a vector of just poses (not preaction poses) for call to
        // Robot::ExecutePathToPose() below
        // TODO: Prettier way to handling making the separate vector of Pose3ds?
        std::vector<Pose3d> possiblePoses;
        const PreActionPose* closestPreActionPose = nullptr;
        f32 closestPoseDist = std::numeric_limits<f32>::max();
        Radians closestPoseAngle = M_PI;
        
        const Point3f preActionPoseDistThresh = ComputePreActionPoseDistThreshold(robot, object,
                                                                              DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE);

        for(auto & preActionPose : possiblePreActionPoses)
        {
          Pose3d possiblePose;
          if(preActionPose.GetPose().GetWithRespectTo(*robot.GetWorldOrigin(), possiblePose) == false) {
            PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.PreActionPoseOriginProblem",
                                "Could not get pre-action pose w.r.t. robot origin.\n");
            
          } else {
            possiblePoses.emplace_back(possiblePose);
            
            if(preActionPoseDistThresh > 0.f) {
              // Keep track of closest possible pose, in case we are already close
              // enough to it to not bother planning a path at all.
              Vec3f Tdiff;
              Radians angleDiff;
              if(possiblePose.IsSameAs(robot.GetPose(), preActionPoseDistThresh,
                                       DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE, Tdiff, angleDiff))
              {
                const f32 currentDist = Tdiff.Length();
                if(currentDist < closestPoseDist &&
                   std::abs(angleDiff.ToFloat()) < std::abs(closestPoseAngle.ToFloat()))
                {
                  closestPoseDist = currentDist;
                  closestPoseAngle = angleDiff;
                  closestPreActionPose = &preActionPose;
                }
              }
            }
            
          }
        }
        
        if(possiblePoses.empty()) {
          PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoPossiblePoses",
                            "No pre-action poses survived as possible docking poses.\n");
          result = FAILURE_ABORT;
        }
        else if (closestPreActionPose != nullptr) {
          PRINT_NAMED_INFO("DriveToObjectAction.InitHelper",
                           "Robot's current pose is close enough to a pre-action pose. "
                           "Just using current pose as the goal.\n");
          _alreadyAtGoal = true;
          result = SUCCESS;
        }
        else {
          Planning::Path p;
          if(robot.GetPathToPose(possiblePoses, selectedIndex, p) != RESULT_OK) {
            result = FAILURE_ABORT;
          }
          else if(robot.ExecutePath(p, IsUsingManualSpeed()) != RESULT_OK) {
            result = FAILURE_ABORT;
          }
          else if(robot.MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 2.f, 6.f) != RESULT_OK) {
            result = FAILURE_ABORT;
          } else {
            //SetGoal(possiblePoses[selectedIndex]);
            SetGoal(possiblePoses[selectedIndex], preActionPoseDistThresh,
                    DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD);
            
            result = SUCCESS;
          }
        }
        
      } // if/else possiblePreActionPoses.empty()
      
      return result;
      
    } // InitHelper()
    
    
    IAction::ActionResult DriveToObjectAction::Init(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                          "Robot %d's block world does not have an ActionableObject with ID=%d.\n",
                          robot.GetID(), _objectID.GetValue());
        
        result = FAILURE_ABORT;
      } else {
      
        result = InitHelper(robot, object);
        
      } // if/else object==nullptr
      
      return result;
    }
    
    
    IAction::ActionResult DriveToObjectAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      if(!_alreadyAtGoal) {
        result = DriveToPoseAction::CheckIfDone(robot);
      }

      return result;
    }
    
            
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose, const bool useManualSpeed)
    : DriveToObjectAction(robot.GetCarryingObject(), PreActionPose::PLACEMENT, useManualSpeed)
    , _placementPose(placementPose)
    {

    }
    
    const std::string& DriveToPlaceCarriedObjectAction::GetName() const
    {
      static const std::string name("DriveToPlaceCarriedObjectAction");
      return name;
    }
    
    IAction::ActionResult DriveToPlaceCarriedObjectAction::Init(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      if(robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d cannot place an object because it is not carrying anything.\n",
                          robot.GetID());
        result = FAILURE_ABORT;
      } else {
        
        _objectID = robot.GetCarryingObject();
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.\n",
                            robot.GetID(), _objectID.GetValue());
          
          result = FAILURE_ABORT;
        } else {
          
          // Temporarily move object to desired pose so we can get placement poses
          // at that position
          const Pose3d origObjectPose(object->GetPose());
          object->SetPose(_placementPose);
          
          // Call parent class's init helper
          result = InitHelper(robot, object);
          
          // Move the object back to where it was (being carried)
          object->SetPose(origObjectPose);
          
        } // if/else object==nullptr
      } // if/else robot is carrying object
      
      return result;
      
    } // CheckPreconditions()
    

#pragma mark ---- TurnInPlaceAction ----
    
    TurnInPlaceAction::TurnInPlaceAction(const Radians& angle)
    : DriveToPoseAction(true)
    , _turnAngle(angle)
    {
      
    }
    
    const std::string& TurnInPlaceAction::GetName() const
    {
      static const std::string name("TurnInPlaceAction");
      return name;
    }
    
    IAction::ActionResult TurnInPlaceAction::Init(Robot &robot)
    {
      // Compute a goal pose rotated by specified angle around robot's
      // _current_ pose
      const Radians heading = robot.GetPose().GetRotationAngle<'Z'>();
      
      Pose3d rotatedPose(heading + _turnAngle, Z_AXIS_3D(),
                         robot.GetPose().GetTranslation(),
                         robot.GetPose().GetParent());
      
      SetGoal(rotatedPose);
      
      // Now that goal is set, call the base class init to send the path, etc.
      return DriveToPoseAction::Init(robot);
    }
    
    
#pragma mark ---- FaceObjectAction ----
    
    FaceObjectAction::FaceObjectAction(ObjectID objectID, Radians turnAngleTol,
                                       Radians maxTurnAngle, bool headTrackWhenDone)
    : FaceObjectAction(objectID, Vision::Marker::ANY_CODE,
                       turnAngleTol, maxTurnAngle, headTrackWhenDone)
    {
      
    }
    
    FaceObjectAction::FaceObjectAction(ObjectID objectID, Vision::Marker::Code whichCode,
                                       Radians turnAngleTol,
                                       Radians maxTurnAngle, bool headTrackWhenDone)
    : _compoundAction{}
    , _objectID(objectID)
    , _whichCode(whichCode)
    , _turnAngleTol(turnAngleTol.getAbsoluteVal())
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    , _waitToVerifyTime(-1.f)
    , _headTrackWhenDone(headTrackWhenDone)
    {

    }
    
    IAction::ActionResult FaceObjectAction::Init(Robot &robot)
    {
      Vision::ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("FaceObjectAction.Init.ObjectNotFound",
                          "Object with ID=%d no longer exists in the world.\n",
                          _objectID.GetValue());
        return FAILURE_ABORT;
      }
      
      Pose3d objectPoseWrtRobot;
      if(_whichCode == Vision::Marker::ANY_CODE) {
        if(false == object->GetPose().GetWithRespectTo(robot.GetPose(), objectPoseWrtRobot)) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.ObjectPoseOriginProblem",
                            "Could not get pose of object %d w.r.t. robot pose.\n",
                            _objectID.GetValue());
          return FAILURE_ABORT;
        }
      } else {
        // Use the closest marker with the specified code:
        std::vector<Vision::KnownMarker*> const& markers = object->GetMarkersWithCode(_whichCode);
        
        if(markers.empty()) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.NoMarkersWithCode",
                            "Object %d does not have any markers with code %d.\n",
                            _objectID.GetValue(), _whichCode);
          return FAILURE_ABORT;
        }
        
        Vision::KnownMarker* closestMarker = nullptr;
        if(markers.size() == 1) {
          closestMarker = markers.front();
          if(false == closestMarker->GetPose().GetWithRespectTo(robot.GetPose(), objectPoseWrtRobot)) {
            PRINT_NAMED_ERROR("FaceObjectAction.Init.MarkerOriginProblem",
                              "Could not get pose of marker with code %d of object %d "
                              "w.r.t. robot pose.\n", _whichCode, _objectID.GetValue() );
            return FAILURE_ABORT;
          }
        } else {
          f32 closestDist = std::numeric_limits<f32>::max();
          Pose3d markerPoseWrtRobot;
          for(auto marker : markers) {
            if(false == marker->GetPose().GetWithRespectTo(robot.GetPose(), markerPoseWrtRobot)) {
              PRINT_NAMED_ERROR("FaceObjectAction.Init.MarkerOriginProblem",
                                "Could not get pose of marker with code %d of object %d "
                                "w.r.t. robot pose.\n", _whichCode, _objectID.GetValue() );
              return FAILURE_ABORT;
            }
            
            const f32 currentDist = markerPoseWrtRobot.GetTranslation().Length();
            if(currentDist < closestDist) {
              closestDist = currentDist;
              closestMarker = marker;
              objectPoseWrtRobot = markerPoseWrtRobot;
            }
          }
        }
        
        if(closestMarker == nullptr) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.NoClosestMarker",
                            "No closest marker found for object %d.\n", _objectID.GetValue());
          return FAILURE_ABORT;
        }
      }
      
      if(_maxTurnAngle > 0)
      {
        // Compute the required angle to face the object
        const Radians turnAngle = std::atan2(objectPoseWrtRobot.GetTranslation().y(),
                                             objectPoseWrtRobot.GetTranslation().x());
        
        PRINT_NAMED_INFO("FaceObjectAction.Init.TurnAngle",
                         "Computed turn angle = %.1fdeg\n", turnAngle.getDegrees());
        
        if(turnAngle.getAbsoluteVal() < _maxTurnAngle) {
          if(turnAngle.getAbsoluteVal() > _turnAngleTol) {
            _compoundAction.AddAction(new TurnInPlaceAction(turnAngle));
          } else {
            PRINT_NAMED_INFO("FaceObjectAction.Init.NoTurnNeeded",
                             "Required turn angle of %.1fdeg is within tolerance of %.1fdeg. Not turning.\n",
                             turnAngle.getDegrees(), _turnAngleTol.getDegrees());
          }
        } else {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.RequiredTurnTooLarge",
                            "Required turn angle of %.1fdeg is larger than max angle of %.1fdeg.\n",
                            turnAngle.getDegrees(), _maxTurnAngle.getDegrees());
          return FAILURE_ABORT;
        }
      } // if(_maxTurnAngle > 0)
    
      // Compute the required head angle to face the object
      // NOTE: It would be more accurate to take head tilt into account, but I'm
      //  just using neck joint height as an approximation for the camera's
      //  current height, since its actual height changes slightly as the head
      //  rotates around the neck.
      const f32 distanceXY = Point2f(objectPoseWrtRobot.GetTranslation()).Length();
      const f32 heightDiff = objectPoseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];
      const Radians headAngle = std::atan2(heightDiff, distanceXY);
      _compoundAction.AddAction(new MoveHeadToAngleAction(headAngle));
      
      // Prevent the compound action from signaling completion
      _compoundAction.SetIsPartOfCompoundAction(true);
      
      // Can't track head to an object and face it
      robot.DisableTrackHeadToObject();
      
      return SUCCESS;
    }
    
    IAction::ActionResult FaceObjectAction::CheckIfDone(Robot& robot)
    {
      // Tick the compound action until it completes
      ActionResult compoundResult = _compoundAction.Update(robot);
      
      if(compoundResult != SUCCESS) {
        return compoundResult;
      }

      const f32 currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_waitToVerifyTime < 0.f) {
        _waitToVerifyTime = currentTime + GetWaitToVerifyTime();
      }

      if(currentTime < _waitToVerifyTime) {
        return RUNNING;
      }
      
      // If we get here, _compoundAction completed returned SUCCESS. So we can
      // can continue with our additional checks:
      
      // Verify that we can see the object we were interested in
      Vision::ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("FaceObjectAction.CheckIfDone.ObjectNotFound",
                          "Object with ID=%d no longer exists in the world.\n",
                          _objectID.GetValue());
        return FAILURE_ABORT;
      }
      
      const TimeStamp_t lastObserved = object->GetLastObservedTime();
      if (robot.GetLastMsgTimestamp() - lastObserved > DOCK_OBJECT_LAST_OBSERVED_TIME_THRESH_MS)
      {
        PRINT_NAMED_WARNING("FaceObjectAction.CheckIfDone.ObjectNotFound",
                            "Object still exists, but not seen since %d (Current time = %d)\n",
                            lastObserved, robot.GetLastMsgTimestamp());
        return FAILURE_ABORT;
      }
      
      if(_whichCode != Vision::Marker::ANY_CODE) {
        std::vector<const Vision::KnownMarker*> observedMarkers;
        object->GetObservedMarkers(observedMarkers, robot.GetLastMsgTimestamp() - DOCK_OBJECT_LAST_OBSERVED_TIME_THRESH_MS);
        
        bool markerWithCodeSeen = false;
        for(auto marker : observedMarkers) {
          if(marker->GetCode() == _whichCode) {
            markerWithCodeSeen = true;
            break;
          }
        }
        
        if(!markerWithCodeSeen) {
          PRINT_NAMED_ERROR("FaceObjectAction.CheckIfDone.MarkerCodeNotSeen",
                            "Object %d observed, but not marker with code %d.\n",
                            _objectID.GetValue(), _whichCode);
          return FAILURE_ABORT;
        }
      }

      if(_headTrackWhenDone) {
        if(robot.EnableTrackHeadToObject(_objectID) == RESULT_OK) {
          return SUCCESS;
        } else {
          PRINT_NAMED_WARNING("FaceObjectAction.CheckIfDone.HeadTracKFail",
                              "Failed to enable head tracking when done.\n");
          return FAILURE_PROCEED;
        }
      }
      
      return SUCCESS;
    } // CheckIfDone()


    const std::string& FaceObjectAction::GetName() const
    {
      static const std::string name("FaceObjectAction");
      return name;
    }
    
    
#pragma mark ---- VisuallyVerifyObjectAction ----
    
    VisuallyVerifyObjectAction::VisuallyVerifyObjectAction(ObjectID objectID,
                                                           Vision::Marker::Code whichCode)
    : FaceObjectAction(objectID, whichCode, 0, 0, false)
    {
      
    }
    
    const std::string& VisuallyVerifyObjectAction::GetName() const
    {
      static const std::string name("VisuallyVerifyObject" + std::to_string(_objectID.GetValue())
                                    + "Action");
      return name;
    }
    
    /*
    IAction::ActionResult VisuallyVerifyObjectAction::Init(Robot& robot)
    {
      
    }
    
    IAction::ActionResult VisuallyVerifyObjectAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      if(!_faceObjectComplete) {
        result = FaceObjectAction::CheckIfDone(robot);
      }
      
      if(result == SUCCESS) {
        // Don't keep running FaceObjectAction::CheckIfDone() above
        _faceObjectComplete = true;
        
        
        
      }
    }
    */
    
#pragma mark ---- MoveHeadToAngleAction ----
    
    MoveHeadToAngleAction::MoveHeadToAngleAction(const Radians& headAngle, const f32 tolerance)
    : _headAngle(headAngle)
    , _angleTolerance(tolerance)
    , _name("MoveHeadTo" + std::to_string(std::round(RAD_TO_DEG(_headAngle.ToFloat()))) + "DegAction")
    {
      if(_headAngle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor",
                            "Requested head angle (%.1fdeg) less than min head angle (%.1fdeg). Clipping.\n",
                            _headAngle.getDegrees(), RAD_TO_DEG(MIN_HEAD_ANGLE));
        _headAngle = MIN_HEAD_ANGLE;
      } else if(_headAngle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor",
                            "Requested head angle (%.1fdeg) more than max head angle (%.1fdeg). Clipping.\n",
                            _headAngle.getDegrees(), RAD_TO_DEG(MAX_HEAD_ANGLE));
        _headAngle = MAX_HEAD_ANGLE;
      }
    }
    
    IActionRunner::ActionResult MoveHeadToAngleAction::Init(Robot &robot)
    {
      // TODO: Add ability to specify speed/accel
      if(robot.MoveHeadToAngle(_headAngle.ToFloat(), 5, 10) != RESULT_OK) {
        return FAILURE_ABORT;
      } else {
        return SUCCESS;
      }
    }
    
    IActionRunner::ActionResult MoveHeadToAngleAction::CheckIfDone(Robot &robot)
    {
      ActionResult result = RUNNING;
      
      // Wait to get a state message back from the physical robot saying its head
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(NEAR(robot.GetHeadAngle(), _headAngle.ToFloat(), _angleTolerance.ToFloat())) {
        result = SUCCESS;
      }
      
      return result;
    }
         
    
#pragma mark ---- IDockAction ----
    
    IDockAction::IDockAction(ObjectID objectID, const bool useManualSpeed)
    : _dockObjectID(objectID)
    , _dockMarker(nullptr)
    , _preActionPoseAngleTolerance(DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE)
    , _wasPickingOrPlacing(false)
    , _useManualSpeed(useManualSpeed)
    , _visuallyVerifyAction(nullptr)
    {
      
    }
    
    IDockAction::~IDockAction()
    {
      if(_visuallyVerifyAction != nullptr) {
        delete _visuallyVerifyAction;
      }
    }
    
    void IDockAction::SetPreActionPoseAngleTolerance(Radians angleTolerance)
    {
      _preActionPoseAngleTolerance = angleTolerance;
    }
    
    IAction::ActionResult IDockAction::Init(Robot& robot)
    {
      _waitToVerifyTime = -1.f;
    
      // Make sure the object we were docking with still exists in the world
      ActionableObject* dockObject = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_dockObjectID));
      if(dockObject == nullptr) {
        PRINT_NAMED_ERROR("IDockAction.Init.ActionObjectNotFound",
                          "Action object with ID=%d no longer exists in the world.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }
      
      // Verify that we ended up near enough a PreActionPose of the right type
      std::vector<PreActionPose> preActionPoses;
      dockObject->GetCurrentPreActionPoses(preActionPoses, {GetPreActionType()});
      
      if(preActionPoses.empty()) {
        PRINT_NAMED_ERROR("IDockAction.Init.NoPreActionPoses",
                          "Action object with ID=%d returned no pre-action poses of the given type.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }

      const Point2f currentXY(robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y());
      
      //float closestDistSq = std::numeric_limits<float>::max();
      Point2f closestPoint(std::numeric_limits<float>::max());
      size_t closestIndex = preActionPoses.size();
      
      for(size_t index=0; index < preActionPoses.size(); ++index) {
        Pose3d preActionPose;
        if(preActionPoses[index].GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), preActionPose) == false) {
          PRINT_NAMED_WARNING("IDockAction.Init.PreActionPoseOriginProblem",
                              "Could not get pre-action pose w.r.t. robot parent.\n");
        }
        
        const Point2f preActionXY(preActionPose.GetTranslation().x(),
                                  preActionPose.GetTranslation().y());
        //const float distSq = (currentXY - preActionXY).LengthSq();
        const Point2f dist = (currentXY - preActionXY).Abs();
        if(dist < closestPoint) {
          //closestDistSq = distSq;
          closestPoint = dist;
          closestIndex  = index;
        }
      }
      
      //const f32 closestDist = sqrtf(closestDistSq);
      
      f32 preActionPoseDistThresh = ComputePreActionPoseDistThreshold(robot, dockObject,
                                                                      _preActionPoseAngleTolerance);
      
      if(preActionPoseDistThresh > 0.f && closestPoint > preActionPoseDistThresh) {
        PRINT_NAMED_INFO("IDockAction.Init.TooFarFromGoal",
                         "Robot is too far from pre-action pose (%.1fmm, %.1fmm).",
                         closestPoint.x(), closestPoint.y());
        return FAILURE_RETRY;
      }
      else {
        if(SelectDockAction(robot, dockObject) != RESULT_OK) {
          PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.DockActionSelectionFailure",
                            "");
          return FAILURE_ABORT;
        }
        
        PRINT_NAMED_INFO("IDockAction.Init.BeginDocking",
                         "Robot is within (%.1fmm,%.1fmm) of the nearest pre-action pose, "
                         "proceeding with docking.\n", closestPoint.x(), closestPoint.y());
        
        // Set dock markers
        _dockMarker = preActionPoses[closestIndex].GetMarker();
        _dockMarker2 = GetDockMarker2(preActionPoses, closestIndex);
        
        // Set up a visual verification action to make sure we can still see the correct
        // marker of the selected object before proceeding
        // NOTE: This also disables tracking head to object if there was any
        _visuallyVerifyAction = new VisuallyVerifyObjectAction(_dockObjectID,
                                                               _dockMarker->GetCode());

        // Disable the visual verification from issuing a completion signal
        _visuallyVerifyAction->SetIsPartOfCompoundAction(true);
        
        return SUCCESS;
      }
      
    } // Init()
    
    
    IAction::ActionResult IDockAction::CheckIfDone(Robot& robot)
    {
      ActionResult actionResult = RUNNING;
      
      // Wait for visual verification to complete successfully before telling
      // robot to dock and continuing to check for completion
      if(_visuallyVerifyAction != nullptr) {
        actionResult = _visuallyVerifyAction->Update(robot);
        if(actionResult == RUNNING) {
          return actionResult;
        } else {
          if(actionResult == SUCCESS) {
            // Finished with visual verification:
            delete _visuallyVerifyAction;
            _visuallyVerifyAction = nullptr;
            actionResult = RUNNING;
            
            PRINT_NAMED_INFO("IDockAction.DockWithObjectHelper.BeginDocking",
                             "Docking with marker %d (%s) using action %d.\n",
                             _dockMarker->GetCode(),
                             Vision::MarkerTypeStrings[_dockMarker->GetCode()], _dockAction);
            
            if(robot.DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction, _useManualSpeed) == RESULT_OK)
            {
              //NOTE: Any completion (success or failure) after this point should tell
              // the robot to stop tracking and go back to looking for markers!
              _wasPickingOrPlacing = false;
            } else {
              return FAILURE_ABORT;
            }

          } else {
            PRINT_NAMED_ERROR("IDockAction.CheckIfDone.VisualVerifyFailed",
                              "VisualVerification of object failed, stopping IDockAction.\n");
            return actionResult;
          }
        }
      }
      
      if (!_wasPickingOrPlacing) {
        // We have to see the robot went into pick-place mode once before checking
        // to see that it has finished picking or placing below. I.e., we need to
        // know the robot got the DockWithObject command sent in Init().
        _wasPickingOrPlacing = robot.IsPickingOrPlacing();
      }
      else if (!robot.IsPickingOrPlacing() && !robot.IsMoving())
      {
        const f32 currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        
        // Set the verification time if not already set
        if(_waitToVerifyTime < 0.f) {
          _waitToVerifyTime = currentTime + GetVerifyDelayInSeconds();
        }
        
        // Stopped executing docking path, and should have backed out by now,
        // and have head pointed at an angle to see where we just placed or
        // picked up from. So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.
        if(currentTime >= _waitToVerifyTime) {
          PRINT_NAMED_INFO("IDockAction.CheckIfDone",
                           "Robot has stopped moving and picking/placing. Will attempt to verify success.\n");
          
          actionResult = Verify(robot);
        }
      }
      
      return actionResult;
    } // CheckIfDone()
   
    
    void IDockAction::Cleanup(Robot& robot)
    {
      // Make sure we back to looking for markers (and stop tracking) whenever
      // and however this action finishes
      robot.StartLookingForMarkers();
      robot.StopDocking();
      
      // Also return the robot's head to level
      robot.MoveHeadToAngle(0, 2.f, 6.f);
      
      // Abort anything that shouldn't still be running
      if(robot.IsTraversingPath()) {
        robot.ClearPath();
      }
      if(robot.IsPickingOrPlacing()) {
        robot.AbortDocking();
      }
    }
    
#pragma mark ---- PickAndPlaceObjectAction ----
    
    PickAndPlaceObjectAction::PickAndPlaceObjectAction(ObjectID objectID, const bool useManualSpeed)
    : IDockAction(objectID, useManualSpeed)
    {
      
    }
    
    const std::string& PickAndPlaceObjectAction::GetName() const
    {
      static const std::string name("PickAndPlaceObjectAction");
      return name;
    }
    
    s32 PickAndPlaceObjectAction::GetType() const
    {
      switch(_dockAction)
      {
        case DA_PICKUP_HIGH:
          return ACTION_PICKUP_OBJECT_HIGH;
          
        case DA_PICKUP_LOW:
          return ACTION_PICKUP_OBJECT_LOW;
          
        case DA_PLACE_HIGH:
          return ACTION_PLACE_OBJECT_HIGH;
          
        case DA_PLACE_LOW:
          return ACTION_PLACE_OBJECT_LOW;
          
        default:
          PRINT_NAMED_WARNING("PickAndPlaceObjectAction.GetType",
                              "Dock action not set before determining action type.\n");
          return ACTION_PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void PickAndPlaceObjectAction::EmitCompletionSignal(Robot& robot, bool success) const
    {
      switch(_dockAction)
      {
        case DA_PICKUP_HIGH:
        case DA_PICKUP_LOW:
        {
          if(!robot.IsCarryingObject()) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                              "Expecting robot to think it's carrying object for pickup action.\n");
          } else {
            const s32 carryObject = robot.GetCarryingObject().GetValue();
            const s32 carryObjectOnTop = robot.GetCarryingObjectOnTop().GetValue();
            
            const u8 numObjects = 1 + (carryObjectOnTop >=0 ? 1 : 0);
            
            // TODO: Be able to fill in add'l objects carried in signal
            CozmoEngineSignals::RobotCompletedActionSignal().emit(robot.GetID(), GetType(),
                                                                  carryObject,
                                                                  carryObjectOnTop, -1, -1, -1,
                                                                  numObjects,
                                                                  success);
            return;
          }
          break;
        }
        case DA_PLACE_HIGH:
        case DA_PLACE_LOW:
        {
          // TODO: Be able to fill in more objects in the stack
          Vision::ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_dockObjectID);
          if(object == nullptr) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                              "Docking object %d not found in world after placing.\n",
                              _dockObjectID.GetValue());
          } else {
            std::array<s32,5> objectStack;
            auto objectStackIter = objectStack.begin();
            objectStack.fill(-1);
            uint8_t numObjects = 0;
            while(object != nullptr && numObjects < objectStack.size()) {
              *objectStackIter = object->GetID().GetValue();
              ++objectStackIter;
              ++numObjects;
              object = robot.GetBlockWorld().FindObjectOnTopOf(*object, 15.f);
            }
            CozmoEngineSignals::RobotCompletedActionSignal().emit(robot.GetID(), GetType(),
                                                                  objectStack[0],
                                                                  objectStack[1],
                                                                  objectStack[2],
                                                                  objectStack[3],
                                                                  objectStack[4],
                                                                  numObjects,
                                                                  success);
            return;
          }
          break;
        }
        default:
          PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                            "Dock action not set before filling completion signal.\n");
      }
      
      IDockAction::EmitCompletionSignal(robot, success);
    }
    
    Result PickAndPlaceObjectAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      // Record the object's original pose (before picking it up) so we can
      // verify later whether we succeeded.
      // Make it w.r.t. robot's parent so we can compare heights fairly.
      if(object->GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), _dockObjectOrigPose) == false) {
        PRINT_NAMED_ERROR("PickAndPlaceObjectAction.SelectDockAction.PoseWrtFailed",
                          "Could not get pose of dock object w.r.t. robot parent.\n");
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = _dockObjectOrigPose.GetTranslation().z() - robot.GetPose().GetTranslation().z();
      _dockAction = DA_PICKUP_LOW;
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        if(robot.IsCarryingObject()) {
          PRINT_INFO("Already carrying object. Can't dock to high object. Aborting.\n");
          return RESULT_FAIL;
          
        } else {
          _dockAction = DA_PICKUP_HIGH;
        }
      } else if (robot.IsCarryingObject()) {
        _dockAction = DA_PLACE_HIGH;
        
        // Need to record the object we are currently carrying because it
        // will get unset when the robot unattaches it during placement, and
        // we want to be able to verify that we're seeing what we just placed.
        _carryObjectID     = robot.GetCarryingObject();
        _carryObjectMarker = robot.GetCarryingMarker();
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    // This helper is used both by PickAndPlaceObjectAction and PlaceObjectOnGroundAction
    // to verify whether they succeeded.
    static IActionRunner::ActionResult VerifyObjectPlacementHelper(Robot& robot, ObjectID objectID,
                                                                   const Vision::KnownMarker* objectMarker)
    {
      if(robot.IsCarryingObject() == true) {
        PRINT_NAMED_ERROR("VerifyObjectPlacementHelper.RobotCarryignObject",
                          "Expecting robot to think it's NOT carrying an object at this point.\n");
        return IActionRunner::FAILURE_ABORT;
      }
      
      // TODO: check to see it ended up in the right place?
      Vision::ObservableObject* object = robot.GetBlockWorld().GetObjectByID(objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("VerifyObjectPlacementHelper.CarryObjectNoLongerExists",
                          "Object %d we were carrying no longer exists in the world.\n",
                          robot.GetCarryingObject().GetValue());
        return IActionRunner::FAILURE_ABORT;
      }
      else if(object->GetLastObservedTime() > (robot.GetLastMsgTimestamp()-500))
      {
        // We've seen the object in the last half second (which could
        // not be true if we were still carrying it)
        PRINT_NAMED_INFO("VerifyObjectPlacementHelper.ObjectPlacementSuccess",
                         "Verification of object placement SUCCEEDED!\n");
        return IActionRunner::SUCCESS;
      } else {
        PRINT_NAMED_INFO("VerifyObjectPlacementHelper.ObjectPlacementFailure",
                         "Verification of object placement FAILED!\n");
        // TODO: correct to assume we are still carrying the object? Maybe object fell out of view?
        robot.SetObjectAsAttachedToLift(objectID, objectMarker); // re-pickup object to attach it to the lift again
        return IActionRunner::FAILURE_RETRY;
      }
    } // VerifyObjectPlacementHelper()

    
    IAction::ActionResult PickAndPlaceObjectAction::Verify(Robot& robot) const
    {
      IAction::ActionResult result = FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DA_PICKUP_LOW:
        case DA_PICKUP_HIGH:
        {
          if(robot.IsCarryingObject() == false) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify.RobotNotCarryingObject",
                              "Expecting robot to think it's carrying an object at this point.\n");
            result = FAILURE_RETRY;
            break;
          }
          
          BlockWorld& blockWorld = robot.GetBlockWorld();
          
          // We should _not_ still see a object with the
          // same type as the one we were supposed to pick up in that
          // block's original position because we should now be carrying it.
          Vision::ObservableObject* carryObject = blockWorld.GetObjectByID(robot.GetCarryingObject());
          if(carryObject == nullptr) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify.CarryObjectNoLongerExists",
                              "Object %d we were carrying no longer exists in the world.\n",
                              robot.GetCarryingObject().GetValue());
            result = FAILURE_ABORT;
            break;
          }
          
          const BlockWorld::ObjectsMapByID_t& objectsWithType = blockWorld.GetExistingObjectsByType(carryObject->GetType());
          
          Vec3f Tdiff;
          Radians angleDiff;
          Vision::ObservableObject* objectInOriginalPose = nullptr;
          for(auto object : objectsWithType) {
            // TODO: is it safe to always have useAbsRotation=true here?
            Vec3f Tdiff;
            Radians angleDiff;
            if(object.second->GetPose().IsSameAs_WithAmbiguity(_dockObjectOrigPose,
                                                               carryObject->GetRotationAmbiguities(),
                                                               carryObject->GetSameDistanceTolerance()*0.5f,
                                                               carryObject->GetSameAngleTolerance(), true,
                                                               Tdiff, angleDiff))
            {
              PRINT_INFO("Seeing object %d in original pose. (Tdiff = (%.1f,%.1f,%.1f), AngleDiff=%.1fdeg\n",
                         object.first.GetValue(),
                         Tdiff.x(), Tdiff.y(), Tdiff.z(), angleDiff.getDegrees());
              objectInOriginalPose = object.second;
              break;
            }
          }
          
          if(objectInOriginalPose != nullptr)
          {
            // Must not actually be carrying the object I thought I was!
            // Put the object I thought I was carrying in the position of the
            // object I matched to it above, and then delete that object.
            // (This prevents a new object with different ID being created.)
            if(carryObject->GetID() != objectInOriginalPose->GetID()) {
              PRINT_INFO("Moving carried object to object seen in original pose and clearing that object.\n");
              carryObject->SetPose(objectInOriginalPose->GetPose());
              blockWorld.ClearObject(objectInOriginalPose->GetID());
            }
            robot.UnSetCarryingObject();
            
            PRINT_INFO("Object pick-up FAILED! (Still seeing object in same place.)\n");
            result = FAILURE_RETRY;
          } else {
            //_carryingObjectID = _dockObjectID;  // Already set?
            //_carryingMarker   = _dockMarker;   //   "
            PRINT_INFO("Object pick-up SUCCEEDED!\n");
            result = SUCCESS;
          }
          break;
        } // PICKUP
          
        case DA_PLACE_LOW:
        case DA_PLACE_HIGH:
          if(robot.GetLastPickOrPlaceSucceeded()) {
            // If the physical robot thinks it succeeded, do a verification of the placement:
            result = VerifyObjectPlacementHelper(robot, _carryObjectID, _carryObjectMarker);
            
            if(result != SUCCESS) {
              PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify",
                                "Robot thinks it placed the object, but verification of "
                                "placement failed. Not sure where carry object %d is, so deleting it.\n",
                                robot.GetCarryingObject().GetValue());
              robot.GetBlockWorld().ClearObject(robot.GetCarryingObject());
            }
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track, so we are probably still holding the block
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify",
                              "Robot reported placement failure. Assuming docking failed "
                              "and robot is still holding same block.\n");
            result = FAILURE_RETRY;
          }
          
          break;
          
        default:
          PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify.ReachedDefaultCase",
                            "Don't know how to verify unexpected dockAction %d.\n", _dockAction);
          result = FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
       
    
#pragma mark ---- PlaceObjectOnGroundAction ----
    
    PlaceObjectOnGroundAction::PlaceObjectOnGroundAction()
    {
      
    }
    
    const std::string& PlaceObjectOnGroundAction::GetName() const
    {
      static const std::string name("PlaceObjectOnGroundAction");
      return name;
    }
   
    IAction::ActionResult PlaceObjectOnGroundAction::Init(Robot& robot)
    {
      ActionResult result = RUNNING;
      
      // Robot must be carrying something to put something down!
      if(robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d executing PlaceObjectOnGroundAction but not carrying object.\n", robot.GetID());
        result = FAILURE_ABORT;
      } else {
        
        _carryingObjectID  = robot.GetCarryingObject();
        _carryObjectMarker = robot.GetCarryingMarker();
        
        if(robot.PlaceObjectOnGround() == RESULT_OK)
        {
          result = SUCCESS;
        } else {
          PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckPreconditions.SendPlaceObjectOnGroundFailed",
                            "Robot's SendPlaceObjectOnGround method reported failure.\n");
          result = FAILURE_ABORT;
        }
        
      } // if/else IsCarryingObject()
      
      // If we were moving, stop moving.
      robot.StopAllMotors();
      
      return result;
      
    } // CheckPreconditions()
    
    
    
    IAction::ActionResult PlaceObjectOnGroundAction::CheckIfDone(Robot& robot)
    {
      ActionResult actionResult = RUNNING;
      
      // Wait for robot to report it is done picking/placing and that it's not
      // moving
      if (!robot.IsPickingOrPlacing() && !robot.IsMoving())
      {
        // Stopped executing docking path, and should have placed carried block
        // and backed out by now, and have head pointed at an angle to see
        // where we just placed or picked up from.
        // So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.

        actionResult = VerifyObjectPlacementHelper(robot, _carryingObjectID, _carryObjectMarker);
        
        if(actionResult != SUCCESS) {
          PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckIfDone",
                            "VerityObjectPlaceHelper reported failure, just deleting object %d.\n",
                            _carryingObjectID.GetValue());
          robot.GetBlockWorld().ClearObject(_carryingObjectID);
        }
        
      } // if robot is not picking/placing or moving
      
      return actionResult;
      
    } // CheckIfDone()
    

    
#pragma mark ---- CrossBridgeAction ----
    
    CrossBridgeAction::CrossBridgeAction(ObjectID bridgeID, const bool useManualSpeed)
    : IDockAction(bridgeID, useManualSpeed)
    {
      
    }
    
    const std::string& CrossBridgeAction::GetName() const
    {
      static const std::string name("CrossBridgeAction");
      return name;
    }
    
    const Vision::KnownMarker* CrossBridgeAction::GetDockMarker2(const std::vector<PreActionPose> &preActionPoses, const size_t closestIndex)
    {
      // Use the unchosen pre-crossing pose marker (the one at the other end of
      // the bridge) as dockMarker2
      assert(preActionPoses.size() == 2);
      size_t indexForOtherEnd = 1 - closestIndex;
      assert(indexForOtherEnd == 0 || indexForOtherEnd == 1);
      return preActionPoses[indexForOtherEnd].GetMarker();
    }
    
    Result CrossBridgeAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      _dockAction = DA_CROSS_BRIDGE;
      return RESULT_OK;
    } // SelectDockAction()
    
    IAction::ActionResult CrossBridgeAction::Verify(Robot& robot) const
    {
      // TODO: Need some kind of verificaiton here?
      PRINT_NAMED_INFO("CrossBridgeAction.Verify.BridgeCrossingComplete",
                       "Robot has completed crossing a bridge.\n");
      return SUCCESS;
    } // Verify()
    
    
#pragma mark ---- AscendOrDescendRampAction ----
    
    AscendOrDescendRampAction::AscendOrDescendRampAction(ObjectID rampID, const bool useManualSpeed)
    : IDockAction(rampID, useManualSpeed)
    {

    }
    
    const std::string& AscendOrDescendRampAction::GetName() const
    {
      static const std::string name("AscendOrDescendRampAction");
      return name;
    }
    
    Result AscendOrDescendRampAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      Ramp* ramp = dynamic_cast<Ramp*>(object);
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("AscendOrDescendRampAction.SelectDockAction.NotRampObject",
                          "Could not cast generic ActionableObject into Ramp object.\n");
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      // Choose ascent or descent
      const Ramp::TraversalDirection direction = ramp->WillAscendOrDescend(robot.GetPose());
      switch(direction)
      {
        case Ramp::ASCENDING:
          _dockAction = DA_RAMP_ASCEND;
          break;
          
        case Ramp::DESCENDING:
          _dockAction = DA_RAMP_DESCEND;
          break;
          
        case Ramp::UNKNOWN:
        default:
          result = RESULT_FAIL;
      }
    
      // Tell robot which ramp it will be using, and in which direction
      robot.SetRamp(_dockObjectID, direction);
            
      return result;
      
    } // SelectDockAction()
    
    
    IAction::ActionResult AscendOrDescendRampAction::Verify(Robot& robot) const
    {
      // TODO: Need to do some kind of verification here?
      PRINT_NAMED_INFO("AscendOrDescendRampAction.Verify.RampAscentOrDescentComplete",
                       "Robot has completed going up/down ramp.\n");
      
      return SUCCESS;
    } // Verify()
    
    
#pragma mark ---- TraverseObjectAction ----
    
    TraverseObjectAction::TraverseObjectAction(ObjectID objectID, const bool useManualSpeed)
    : _objectID(objectID)
    , _chosenAction(nullptr)
    , _useManualSpeed(useManualSpeed)
    {
      
    }
    
    TraverseObjectAction::~TraverseObjectAction()
    {
      if(_chosenAction != nullptr) {
        delete _chosenAction;
        _chosenAction = nullptr;
      }
    }
    
    const std::string& TraverseObjectAction::GetName() const
    {
      static const std::string name("TraverseObjectAction");
      return name;
    }
    
    void TraverseObjectAction::Reset()
    {
      if(_chosenAction != nullptr) {
        _chosenAction->Reset();
      }
    }
    
    IActionRunner::ActionResult TraverseObjectAction::UpdateInternal(Robot& robot)
    {
      // Select the chosen action based on the object's type, if we haven't
      // already
      if(_chosenAction == nullptr) {
        ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("TraverseObjectAction.Init.ObjectNotFound",
                            "Could not get actionable object with ID = %d from world.\n", _objectID.GetValue());
          return FAILURE_ABORT;
        }
        
        if(object->GetType() == Bridge::Type::LONG_BRIDGE ||
           object->GetType() == Bridge::Type::SHORT_BRIDGE)
        {
          _chosenAction = new CrossBridgeAction(_objectID, _useManualSpeed);
        }
        else if(object->GetType() == Ramp::Type::BASIC_RAMP) {
          _chosenAction = new AscendOrDescendRampAction(_objectID, _useManualSpeed);
        }
        else {
          PRINT_NAMED_ERROR("TraverseObjectAction.Init.CannotTraverseObjectType",
                            "Robot %d was asked to traverse object ID=%d of type %s, but "
                            "that traversal is not defined.\n", robot.GetID(),
                            object->GetID().GetValue(), object->GetType().GetName().c_str());
          
          return FAILURE_ABORT;
        }
      }
      
      // Now just use chosenAction's Update()
      assert(_chosenAction != nullptr);
      return _chosenAction->Update(robot);
      
    } // Update()
    
    
#pragma mark ---- PlayAnimationAction ----
    
    PlayAnimationAction::PlayAnimationAction(const std::string& animName)
    : _animName(animName)
    , _name("PlayAnimation" + animName + "Action")
    {
      
    }
    
    IAction::ActionResult PlayAnimationAction::Init(Robot& robot)
    {
      if(robot.PlayAnimation(_animName.c_str()) == RESULT_OK) {
        return SUCCESS;
      } else {
        return FAILURE_ABORT;
      }
    }
    
    IAction::ActionResult PlayAnimationAction::CheckIfDone(Robot& robot)
    {
      if(robot.IsAnimating()) {
        return RUNNING;
      } else {
        return SUCCESS;
      }
    }
    
    void PlayAnimationAction::Cleanup(Robot& robot)
    {
      // Abort anything that shouldn't still be running
      if(robot.IsAnimating()) {
        robot.AbortAnimation();
      }
    }
    
#pragma mark ---- PlaySoundAction ----
    
    PlaySoundAction::PlaySoundAction(const std::string& soundName)
    : _soundName(soundName)
    , _name("PlaySound" + soundName + "Action")
    {
      
    }

    IAction::ActionResult PlaySoundAction::CheckIfDone(Robot& robot)
    {
      // TODO: Implement!
      return FAILURE_ABORT;
    }
    
#pragma mark ---- WaitAction ----
    
    WaitAction::WaitAction(f32 waitTimeInSeconds)
    : _waitTimeInSeconds(waitTimeInSeconds)
    , _doneTimeInSeconds(-1.f)
    {
      // Put the wait time with two decimals of precision in the action's name
      char tempBuffer[32];
      snprintf(tempBuffer, 32, "Wait%.2fSecondsAction", _waitTimeInSeconds);
      _name = tempBuffer;
    }
    
    IActionRunner::ActionResult WaitAction::Init(Robot& robot)
    {
      _doneTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _waitTimeInSeconds;
      return SUCCESS;
    }
    
    IActionRunner::ActionResult WaitAction::CheckIfDone(Robot& robot)
    {
      assert(_doneTimeInSeconds > 0.f);
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _doneTimeInSeconds) {
        return SUCCESS;
      } else {
        return RUNNING;
      }
    }
    
    /*
    static void TestInstantiation(void)
    {
      Robot robot(7, nullptr);
      ObjectID id;
      id.Set();
      
      PickAndPlaceObjectAction action(robot, id);
      AscendOrDescendRampAction action2(robot, id);
      CrossBridgeAction action3(robot, id);
    }
    */
  } // namespace Cozmo
} // namespace Anki
