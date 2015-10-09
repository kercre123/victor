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
#include "anki/cozmo/basestation/charger.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "util/random/randomGenerator.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include <iomanip>

namespace Anki {
  
  namespace Cozmo {
    
    // TODO: Define this as a constant parameter elsewhere
    const Radians DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE(DEG_TO_RAD(7.5));
    
    // Right before docking, the dock object must have been visually verified
    // no more than this many milliseconds ago or it will not even attempt to dock.
    const u32 DOCK_OBJECT_LAST_OBSERVED_TIME_THRESH_MS = 1000;
 
    // Helper function for computing the distance-to-preActionPose threshold,
    // given how far robot is from actionObject
    f32 ComputePreActionPoseDistThreshold(const Pose3d& preActionPose,
                                          const ActionableObject* actionObject,
                                          const Radians& preActionPoseAngleTolerance)
    {
      assert(actionObject != nullptr);
      
      if(preActionPoseAngleTolerance > 0.f) {
        // Compute distance threshold to preaction pose based on distance to the
        // object: the further away, the more slop we're allowed.
        Pose3d objectWrtRobot;
        if(false == actionObject->GetPose().GetWithRespectTo(preActionPose, objectWrtRobot)) {
          PRINT_NAMED_ERROR("IDockAction.Init.ObjectPoseOriginProblem",
                            "Could not get object %d's pose w.r.t. robot.",
                            actionObject->GetID().GetValue());
          return -1.f;
        }
        
        const f32 objectDistance = objectWrtRobot.GetTranslation().Length();
        const f32 preActionPoseDistThresh = objectDistance * std::sin(preActionPoseAngleTolerance.ToFloat());
        
        PRINT_NAMED_INFO("IDockAction.Init.DistThresh",
                         "At a distance of %.1fmm, will use pre-dock pose distance threshold of %.1fmm",
                         objectDistance, preActionPoseDistThresh);
        
        return preActionPoseDistThresh;
      } else {
        return -1.f;
      }
    }
    
#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction(const bool forceHeadDown,
                                         const bool useManualSpeed) //, const Pose3d& pose)
    : _isGoalSet(false)
    , _driveWithHeadDown(forceHeadDown)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    , _useManualSpeed(useManualSpeed)
    , _maxPlanningTime(DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S)
    , _maxReplanPlanningTime(DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S)
    , _timeToAbortPlanning(-1.0f)
    {
      
    }
    
    void DriveToPoseAction::Reset()
    {
      IAction::Reset();
      _timeToAbortPlanning = -1.0f;
    }
    
    DriveToPoseAction::DriveToPoseAction(const Pose3d& pose,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
      : DriveToPoseAction(forceHeadDown, useManualSpeed)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;

      SetGoal(pose, distThreshold, angleThreshold);
    }
    
    DriveToPoseAction::DriveToPoseAction(const std::vector<Pose3d>& poses,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
      : DriveToPoseAction(forceHeadDown, useManualSpeed)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;

      SetGoals(poses, distThreshold, angleThreshold);
    }
    
    Result DriveToPoseAction::SetGoal(const Anki::Pose3d& pose)
    {
      _goalPoses = {pose};
      
      PRINT_NAMED_INFO("DriveToPoseAction.SetGoal",
                       "Setting pose goal to (%.1f,%.1f,%.1f) @ %.1fdeg",
                       _goalPoses.back().GetTranslation().x(),
                       _goalPoses.back().GetTranslation().y(),
                       _goalPoses.back().GetTranslation().z(),
                       RAD_TO_DEG(_goalPoses.back().GetRotationAngle<'Z'>().ToFloat()));
      
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
    
    Result DriveToPoseAction::SetGoals(const std::vector<Pose3d>& poses,
                                       const Point3f& distThreshold,
                                       const Radians& angleThreshold)
    {
      _goalDistanceThreshold = distThreshold;
      _goalAngleThreshold    = angleThreshold;
      
      return SetGoals(poses);
    }
    
    Result DriveToPoseAction::SetGoals(const std::vector<Pose3d>& poses)
    {
      _goalPoses = poses;

      PRINT_NAMED_INFO("DriveToPoseAction.SetGoal",
                       "Setting %lu possible goal options.",
                       _goalPoses.size());
      
      _isGoalSet = true;
      
      return RESULT_OK;
    }
    
    const std::string& DriveToPoseAction::GetName() const
    {
      static const std::string name("DriveToPoseAction");
      return name;
    }
    
    ActionResult DriveToPoseAction::Init(Robot& robot)
    {
      ActionResult result = ActionResult::SUCCESS;

      _timeToAbortPlanning = -1.0f;
            
      if(!_isGoalSet) {
        PRINT_NAMED_ERROR("DriveToPoseAction.Init.NoGoalSet",
                          "Goal must be set before running this action.");
        result = ActionResult::FAILURE_ABORT;
      }
      else {
        
        // Make the poses w.r.t. robot:
        for(auto & pose : _goalPoses) {
          if(pose.GetWithRespectTo(*robot.GetWorldOrigin(), pose) == false) {
            PRINT_NAMED_ERROR("DriveToPoseAction.Init",
                              "Could not get goal pose w.r.t. to robot origin.");
            return ActionResult::FAILURE_ABORT;
          }
        }
        
        Result planningResult = RESULT_OK;
        
        _selectedGoalIndex = 0;

        if(_goalPoses.size() == 1) {
          planningResult = robot.StartDrivingToPose(_goalPoses.back(), _useManualSpeed);
        } else {
          planningResult = robot.StartDrivingToPose(_goalPoses, &_selectedGoalIndex, _useManualSpeed);
        }
        
        if(planningResult != RESULT_OK) {
          PRINT_NAMED_ERROR("DriveToPoseAction.Init", "Failed to get path to goal pose.");
          result = ActionResult::FAILURE_ABORT;
        }
        
        if(result == ActionResult::SUCCESS) {
          // So far so good.
          
          if(_driveWithHeadDown) {
            // Now put the head at the right angle for following paths
            // TODO: Make it possible to set the speed/accel somewhere?
            if(robot.MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 2.f, 5.f) != RESULT_OK) {
              PRINT_NAMED_ERROR("DriveToPoseAction.Init", "Failed to move head to path-following angle.");
              result = ActionResult::FAILURE_ABORT;
            }
          }
          
          // Create a callback to respond to a robot world origin change that resets
          // the action since the goal pose is likely now invalid.
          // NOTE: I'm not passing the robot reference in because it will get create
          //  a copy of the robot inside the lambda. I believe using the pointer
          //  is safe because this lambda can't outlive this action which can't
          //  outlive the robot whose queue it exists in.
          Robot* robotPtr = &robot;
          auto cbRobotOriginChanged = [this,robotPtr](RobotID_t robotID) {
            if(robotID == robotPtr->GetID()) {
              PRINT_NAMED_INFO("DriveToPoseAction",
                               "Received signal that robot %d's origin changed. Resetting action.",
                               robotID);
              this->Reset();
              robotPtr->AbortDrivingToPose();
            }
          };
          _signalHandle = robot.OnRobotWorldOriginChanged().ScopedSubscribe(cbRobotOriginChanged);
        }
        
      } // if/else isGoalSet
    
      return result;
    } // Init()
    
    ActionResult DriveToPoseAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = ActionResult::RUNNING;
      
      switch( robot.CheckDriveToPoseStatus() ) {
        case ERobotDriveToPoseStatus::Error:
          PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Failure", "Robot driving to pose failed");
          _timeToAbortPlanning = -1.0f;
          result = ActionResult::FAILURE_ABORT;
          break;

        case ERobotDriveToPoseStatus::ComputingPath: {
          float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          
          // handle aborting the plan. If we don't have a timeout set, set one now
          if( _timeToAbortPlanning < 0.0f ) {
            _timeToAbortPlanning = currTime + _maxPlanningTime;
          }
          else if( currTime >= _timeToAbortPlanning ) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.ComputingPathTimeout",
                             "Robot has been planning for more than %f seconds, aborting",
                             _maxPlanningTime);
            robot.AbortDrivingToPose();
            result = ActionResult::FAILURE_ABORT;
            _timeToAbortPlanning = -1.0f;
          }
          break;
        }

        case ERobotDriveToPoseStatus::Replanning: {
          float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          
          // handle aborting the plan. If we don't have a timeout set, set one now
          if( _timeToAbortPlanning < 0.0f ) {
            _timeToAbortPlanning = currTime + _maxReplanPlanningTime;              
          }
          else if( currTime >= _timeToAbortPlanning ) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Replanning.Timeout",
                             "Robot has been planning for more than %f seconds, aborting",
                             _maxReplanPlanningTime);
            robot.AbortDrivingToPose();
            // re-try in this case, since we might be able to succeed once we stop and take more time to plan
            result = ActionResult::FAILURE_RETRY;
            _timeToAbortPlanning = -1.0f;
          }
          break;
        }

        case ERobotDriveToPoseStatus::FollowingPath: {
          // clear abort timing, since we got a path
          _timeToAbortPlanning = -1.0f;

          static int ctr = 0;
          if(ctr++ % 10 == 0) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.WaitingForPathCompletion",
                             "Waiting for robot to complete its path traversal (%d), "
                             "_currPathSegment=%d, _lastSentPathID=%d, _lastRecvdPathID=%d.\n", ctr,
                             robot.GetCurrentPathSegment(), robot.GetLastSentPathID(), robot.GetLastRecvdPathID());
          }
          break;
        }

        case ERobotDriveToPoseStatus::Waiting: {
          // clear abort timing, since we got a path
          _timeToAbortPlanning = -1.0f;

          // No longer traversing the path, so check to see if we ended up in the right place
          Vec3f Tdiff;
          
          // HACK: Loosen z threshold bigtime:
          const Point3f distanceThreshold(_goalDistanceThreshold.x(),
                                          _goalDistanceThreshold.y(),
                                          robot.GetHeight());

          if(robot.GetPose().IsSameAs(_goalPoses[_selectedGoalIndex], distanceThreshold, _goalAngleThreshold, Tdiff))
          {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Success",
                             "Robot %d successfully finished following path (Tdiff=%.1fmm).",
                             robot.GetID(), Tdiff.Length());
            
            result = ActionResult::SUCCESS;
          }
          // The last path sent was definitely received by the robot
          // and it is no longer executing it, but we appear to not be in position
          else if (robot.GetLastSentPathID() == robot.GetLastRecvdPathID()) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.DoneNotInPlace",
                             "Robot is done traversing path, but is not in position (dist=%.1fmm). lastPathID=%d",
                             Tdiff.Length(), robot.GetLastRecvdPathID());
            result = ActionResult::FAILURE_RETRY;
          }
          else {
            // Something went wrong: not in place and robot apparently hasn't
            // received all that it should have
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Failure",
                             "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false.");
            result = ActionResult::FAILURE_ABORT;
          }
          break;
        }
      }
      
      return result;
    } // CheckIfDone()
    
    void DriveToPoseAction::Cleanup(Robot &robot)
    {
      // If we are not running anymore, for any reason, clear the path and its
      // visualization
      robot.AbortDrivingToPose();
      VizManager::getInstance()->ErasePath(robot.GetID());
      VizManager::getInstance()->EraseAllPlannerObstacles(true);
      VizManager::getInstance()->EraseAllPlannerObstacles(false);
    }
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType, const f32 predockOffsetDistX_mm, const bool useManualSpeed)
    : _objectID(objectID)
    , _actionType(actionType)
    , _distance_mm(-1.f)
    , _predockOffsetDistX_mm(predockOffsetDistX_mm)
    , _useManualSpeed(useManualSpeed)
    {
      // NOTE: _goalPose will be set later, when we check preconditions
    }
    
    DriveToObjectAction::DriveToObjectAction(const ObjectID& objectID, const f32 distance, const bool useManualSpeed)
    : _objectID(objectID)
    , _actionType(PreActionPose::ActionType::NONE)
    , _distance_mm(distance)
    , _predockOffsetDistX_mm(0)
    , _useManualSpeed(useManualSpeed)
    {
      // NOTE: _goalPose will be set later, when we check preconditions
    }
    
    void DriveToObjectAction::Reset()
    {
      IAction::Reset();
      _compoundAction.ClearActions();
    }
    
    const std::string& DriveToObjectAction::GetName() const
    {
      static const std::string name("DriveToObjectAction");
      return name;
    }
    
    ActionResult DriveToObjectAction::GetPossiblePoses(const Robot& robot, ActionableObject* object,
                                                       std::vector<Pose3d>& possiblePoses,
                                                       bool& alreadyInPosition)
    {
      ActionResult result = ActionResult::SUCCESS;
      
      alreadyInPosition = false;
      possiblePoses.clear();
      
      std::vector<PreActionPose> possiblePreActionPoses;
      std::vector<std::pair<Quad2f,ObjectID> > obstacles;
      robot.GetBlockWorld().GetObstacles(obstacles);
      object->GetCurrentPreActionPoses(possiblePreActionPoses, {_actionType},
                                       std::set<Vision::Marker::Code>(),
                                       obstacles,
                                       &robot.GetPose(),
                                       _predockOffsetDistX_mm);
      
      if(possiblePreActionPoses.empty()) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoPreActionPoses",
                          "ActionableObject %d did not return any pre-action poses with action type %d.",
                          _objectID.GetValue(), _actionType);
        
        return ActionResult::FAILURE_ABORT;
        
      } else {
        
        // Check to see if we already close enough to a pre-action pose that we can
        // just skip path planning. In case multiple pre-action poses are close
        // enough, keep the closest one.
        // Also make a vector of just poses (not preaction poses) for call to
        // Robot::ExecutePathToPose() below
        // TODO: Prettier way to handling making the separate vector of Pose3ds?
        const PreActionPose* closestPreActionPose = nullptr;
        f32 closestPoseDist = std::numeric_limits<f32>::max();
        Radians closestPoseAngle = M_PI;
        
        Point3f preActionPoseDistThresh = ComputePreActionPoseDistThreshold(robot.GetPose(), object,
                                                                            DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE);
        
        preActionPoseDistThresh.z() = REACHABLE_PREDOCK_POSE_Z_THRESH_MM;
        for(auto & preActionPose : possiblePreActionPoses)
        {
          Pose3d possiblePose;
          if(preActionPose.GetPose().GetWithRespectTo(*robot.GetWorldOrigin(), possiblePose) == false) {
            PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.PreActionPoseOriginProblem",
                                "Could not get pre-action pose w.r.t. robot origin.");
            
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
                            "No pre-action poses survived as possible docking poses.");
          result = ActionResult::FAILURE_ABORT;
        }
        else if (closestPreActionPose != nullptr) {
          PRINT_NAMED_INFO("DriveToObjectAction.InitHelper",
                           "Robot's current pose is close enough to a pre-action pose. "
                           "Just using current pose as the goal.");
          
          alreadyInPosition = true;
          result = ActionResult::SUCCESS;
        }
      }
      
      return result;
    } // GetPossiblePoses()
    
    ActionResult DriveToObjectAction::InitHelper(Robot& robot, ActionableObject* object)
    {
      ActionResult result = ActionResult::RUNNING;
      
      std::vector<Pose3d> possiblePoses;
      bool alreadyInPosition = false;
      
      if(PreActionPose::ActionType::NONE == _actionType) {
        
        if(_distance_mm < 0.f) {
          PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.NoDistanceSet",
                            "ActionType==NONE but no distance set either.");
          result = ActionResult::FAILURE_ABORT;
        } else {
        
          Pose3d objectWrtRobotParent;
          if(false == object->GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), objectWrtRobotParent)) {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            result = ActionResult::FAILURE_ABORT;
          } else {
            Point2f vec(robot.GetPose().GetTranslation());
            vec -= Point2f(objectWrtRobotParent.GetTranslation());
            const f32 currentDistance = vec.MakeUnitLength();
            if(currentDistance < _distance_mm) {
              alreadyInPosition = true;
            } else {
              vec *= _distance_mm;
              const Point3f T(vec.x() + objectWrtRobotParent.GetTranslation().x(),
                              vec.y() + objectWrtRobotParent.GetTranslation().y(),
                              robot.GetPose().GetTranslation().z());
              possiblePoses.push_back(Pose3d(std::atan2f(-vec.y(), -vec.x()), Z_AXIS_3D(), T, objectWrtRobotParent.GetParent()));
            }
            result = ActionResult::SUCCESS;
          }
        }
      } else {
        
        result = GetPossiblePoses(robot, object, possiblePoses, alreadyInPosition);
      }
      
      if(result == ActionResult::SUCCESS) {
        if(!alreadyInPosition) {
          
          f32 preActionPoseDistThresh = ComputePreActionPoseDistThreshold(possiblePoses[0], object, DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE);
          
          _compoundAction.AddAction(new DriveToPoseAction(possiblePoses, true, _useManualSpeed, preActionPoseDistThresh));
        }
      
      
        // Make sure we can see the object, unless we are carrying it (i.e. if we
        // are doing a DriveToPlaceCarriedObject action)
        if(!object->IsBeingCarried()) {
          _compoundAction.AddAction(new FaceObjectAction(_objectID, Radians(0), Radians(0), true, false));
        }
        
        _compoundAction.SetIsPartOfCompoundAction(true);
        
        // Go ahead and do the first Update on the compound action, so we don't
        // "waste" the first CheckIfDone call just initializing it
        result = _compoundAction.Update(robot);
        if(ActionResult::RUNNING == result || ActionResult::SUCCESS == result) {
          result = ActionResult::SUCCESS;
        }
      }
      
      return result;
      
    } // InitHelper()
    
    
    ActionResult DriveToObjectAction::Init(Robot& robot)
    {
      ActionResult result = ActionResult::SUCCESS;
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                          "Robot %d's block world does not have an ActionableObject with ID=%d.",
                          robot.GetID(), _objectID.GetValue());
        
        result = ActionResult::FAILURE_ABORT;
      } else {
      
        // Use a helper here so that it can be shared with DriveToPlaceCarriedObjectAction
        result = InitHelper(robot, object);
        
      } // if/else object==nullptr
      
      return result;
    }
    
    
    ActionResult DriveToObjectAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = _compoundAction.Update(robot);
      
      if(result == ActionResult::SUCCESS) {
        // We completed driving to the pose and visually verifying the object
        // is still there. This could have updated the object's pose (hopefully
        // to a more accurate one), meaning the pre-action pose we selected at
        // Initialization has now moved and we may not be in position, even if
        // we completed the planned path successfully. If that's the case, we
        // want to retry.
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToObjectAction.CheckIfDone.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            robot.GetID(), _objectID.GetValue());
          
          result = ActionResult::FAILURE_ABORT;
        } else if( _actionType == PreActionPose::ActionType::NONE) {
          
          // Check to see if we got close enough
          Pose3d objectPoseWrtRobotParent;
          if(false == object->GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), objectPoseWrtRobotParent)) {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            result = ActionResult::FAILURE_ABORT;
          } else {
            const f32 distanceSq = (Point2f(objectPoseWrtRobotParent.GetTranslation()) - Point2f(robot.GetPose().GetTranslation())).LengthSq();
            if(distanceSq > _distance_mm*_distance_mm) {
              PRINT_NAMED_WARNING("DriveToObjectAction.CheckIfDone", "Robot not close enough, will return FAILURE_RETRY.");
              result = ActionResult::FAILURE_RETRY;
            }
          }
        } else {
          
          std::vector<Pose3d> possiblePoses; // don't really need these
          bool inPosition = false;
          result = GetPossiblePoses(robot, object, possiblePoses, inPosition);
          
          if(!inPosition) {
            PRINT_NAMED_WARNING("DriveToObjectAction.CheckIfDone", "Robot not in position, will return FAILURE_RETRY.");
            result = ActionResult::FAILURE_RETRY;
          }
        }
        
      }

      return result;
    }
    
    void DriveToObjectAction::Cleanup(Robot &robot)
    {
      _compoundAction.Cleanup(robot);
    }
            
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose, const bool useManualSpeed)
    : DriveToObjectAction(robot.GetCarryingObject(), PreActionPose::PLACEMENT, 0, useManualSpeed)
    , _placementPose(placementPose)
    {

    }
    
    const std::string& DriveToPlaceCarriedObjectAction::GetName() const
    {
      static const std::string name("DriveToPlaceCarriedObjectAction");
      return name;
    }
    
    ActionResult DriveToPlaceCarriedObjectAction::Init(Robot& robot)
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if(robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d cannot place an object because it is not carrying anything.",
                          robot.GetID());
        result = ActionResult::FAILURE_ABORT;
      } else {
        
        _objectID = robot.GetCarryingObject();
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            robot.GetID(), _objectID.GetValue());
          
          result = ActionResult::FAILURE_ABORT;
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
      
    } // DriveToPlaceCarriedObjectAction::Init()
    
    
    ActionResult DriveToPlaceCarriedObjectAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = _compoundAction.Update(robot);
      
      // We completed driving to the pose. Unlike driving to an object for
      // pickup, we can't re-verify the accuracy of our final position, so
      // just proceed.
      
      return result;
    } // DriveToPlaceCarriedObjectAction::CheckIfDone()

#pragma mark ---- TurnInPlaceAction ----
    
    TurnInPlaceAction::TurnInPlaceAction(const Radians& angle, const bool isAbsolute, const Radians& variability)
    : DriveToPoseAction(false, false)
    , _turnAngle(angle)
    , _variability(variability)
    , _isAbsoluteAngle(isAbsolute)
    {
      
    }

    const std::string& TurnInPlaceAction::GetName() const
    {
      static const std::string name("TurnInPlaceAction");
      return name;
    }
    
    ActionResult TurnInPlaceAction::Init(Robot &robot)
    {
      // Compute a goal pose rotated by specified angle around robot's
      // _current_ pose, taking into account the current driveCenter offset
      Radians heading = 0;
      if (!_isAbsoluteAngle) {
        heading = robot.GetPose().GetRotationAngle<'Z'>();
      }
      Pose3d rotatedPose;
      Pose3d dcPose = robot.GetDriveCenterPose();
      
      Radians newAngle(heading);
      newAngle += _turnAngle;
      if(_variability != 0) {
        newAngle += _rng.RandDblInRange(-_variability.ToDouble(),
                                         _variability.ToDouble());
      }
      dcPose.SetRotation(newAngle, Z_AXIS_3D());
      robot.ComputeOriginPose(dcPose, rotatedPose);
      
      SetGoal(rotatedPose);
      
      _startedTraversingPath = false;
      
      // Now that goal is set, call the base class init to send the path, etc.
      return DriveToPoseAction::Init(robot);
    }
    
    ActionResult TurnInPlaceAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_startedTraversingPath) {
        // Wait until robot reports it has started traversing the path
        _startedTraversingPath = robot.IsTraversingPath();
        
      } else {
        // Wait until robot reports it is no longer traversing a path
        if(!robot.IsTraversingPath()) {
            result = ActionResult::SUCCESS;
        }
      }
      
      return result;
    } // TurnInPlaceAction::CheckIfDone()

    
#pragma mark ---- FacePoseAction ----
    
    FacePoseAction::FacePoseAction(const Pose3d& pose, Radians turnAngleTol, Radians maxTurnAngle)
    : _compoundAction{}
    , _poseWrtRobot(pose)
    , _isPoseSet(true)
    , _turnAngleTol(turnAngleTol.getAbsoluteVal())
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    {

    }
    
    FacePoseAction::FacePoseAction(Radians turnAngleTol, Radians maxTurnAngle)
    : _compoundAction{}
    , _isPoseSet(false)
    , _turnAngleTol(turnAngleTol.getAbsoluteVal())
    , _maxTurnAngle(maxTurnAngle.getAbsoluteVal())
    {
      
    }
    
    void FacePoseAction::Reset()
    {
      IAction::Reset();
      _compoundAction.ClearActions();
    }
    
    Radians FacePoseAction::GetHeadAngle(f32 heightDiff)
    {
      const f32 distanceXY = Point2f(_poseWrtRobot.GetTranslation()).Length();
      const Radians headAngle = std::atan2(heightDiff, distanceXY);
      return headAngle;
    }
    
    void FacePoseAction::SetPose(const Pose3d& pose)
    {
      _poseWrtRobot = pose;
      _isPoseSet = true;
    }
    
    ActionResult FacePoseAction::Init(Robot &robot)
    {
      if(!_isPoseSet) {
        PRINT_NAMED_ERROR("FacePoseAction.Init.PoseNotSet", "");
        return ActionResult::FAILURE_ABORT;
      }
      
      if(_poseWrtRobot.GetParent() == nullptr) {
        PRINT_NAMED_INFO("FacePoseAction.SetPose.AssumingRobotOriginAsParent", "");
        _poseWrtRobot.SetParent(robot.GetWorldOrigin());
      }
      else if(false == _poseWrtRobot.GetWithRespectTo(robot.GetPose(), _poseWrtRobot))
      {
        PRINT_NAMED_ERROR("FacePoseAction.Init.PoseOriginFailure",
                          "Could not get pose w.r.t. robot pose.");
        return ActionResult::FAILURE_ABORT;
      }
      
      if(_maxTurnAngle > 0)
      {
        // Compute the required angle to face the object
        const Radians turnAngle = std::atan2(_poseWrtRobot.GetTranslation().y(),
                                             _poseWrtRobot.GetTranslation().x());
        
        PRINT_NAMED_INFO("FacePoseAction.Init.TurnAngle",
                         "Computed turn angle = %.1fdeg", turnAngle.getDegrees());
        
        if(turnAngle.getAbsoluteVal() < _maxTurnAngle) {
          if(turnAngle.getAbsoluteVal() > _turnAngleTol) {
            _compoundAction.AddAction(new TurnInPlaceAction(turnAngle, false));
          } else {
            PRINT_NAMED_INFO("FacePoseAction.Init.NoTurnNeeded",
                             "Required turn angle of %.1fdeg is within tolerance of %.1fdeg. Not turning.",
                             turnAngle.getDegrees(), _turnAngleTol.getDegrees());
          }
        } else {
          PRINT_NAMED_ERROR("FacePoseAction.Init.RequiredTurnTooLarge",
                            "Required turn angle of %.1fdeg is larger than max angle of %.1fdeg.",
                            turnAngle.getDegrees(), _maxTurnAngle.getDegrees());
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      // Compute the required head angle to face the object
      // NOTE: It would be more accurate to take head tilt into account, but I'm
      //  just using neck joint height as an approximation for the camera's
      //  current height, since its actual height changes slightly as the head
      //  rotates around the neck.
      const f32 heightDiff = _poseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];
      Radians headAngle = GetHeadAngle(heightDiff);
      
      _compoundAction.AddAction(new MoveHeadToAngleAction(headAngle));
      
      // Prevent the compound action from signaling completion
      _compoundAction.SetIsPartOfCompoundAction(true);
      
      // Go ahead and do the first Update for the compound action so we don't
      // "waste" the first CheckIfDone call doing so. Proceed so long as this
      // first update doesn't _fail_
      ActionResult compoundResult = _compoundAction.Update(robot);
      if(ActionResult::SUCCESS == compoundResult ||
         ActionResult::RUNNING == compoundResult)
      {
        return ActionResult::SUCCESS;
      } else {
        return compoundResult;
      }
      
    } // FacePoseAction::Init()
    
    
    ActionResult FacePoseAction::CheckIfDone(Robot& robot)
    {
      return _compoundAction.Update(robot);
    }
    
    const std::string& FacePoseAction::GetName() const
    {
      static const std::string name("FacePoseAction");
      return name;
    }

#pragma mark ---- FaceObjectAction ----
    
    FaceObjectAction::FaceObjectAction(ObjectID objectID, Radians turnAngleTol,
                                       Radians maxTurnAngle,
                                       bool visuallyVerifyWhenDone,
                                       bool headTrackWhenDone)
    : FaceObjectAction(objectID, Vision::Marker::ANY_CODE,
                       turnAngleTol, maxTurnAngle, visuallyVerifyWhenDone, headTrackWhenDone)
    {
      
    }
    
    FaceObjectAction::FaceObjectAction(ObjectID objectID, Vision::Marker::Code whichCode,
                                       Radians turnAngleTol,
                                       Radians maxTurnAngle,
                                       bool visuallyVerifyWhenDone,
                                       bool headTrackWhenDone)
    : FacePoseAction(turnAngleTol, maxTurnAngle)
    , _facePoseCompoundActionDone(false)
    , _visuallyVerifyAction(objectID, whichCode)
    , _objectID(objectID)
    , _whichCode(whichCode)
    , _visuallyVerifyWhenDone(visuallyVerifyWhenDone)
    , _headTrackWhenDone(headTrackWhenDone)
    {

    }
    
    void FaceObjectAction::Reset()
    {
      FacePoseAction::Reset();
      _facePoseCompoundActionDone = false;
    }
    
    Radians FaceObjectAction::GetHeadAngle(f32 heightDiff)
    {
      // TODO: Just commanding fixed head angle depending on height of object.
      //       Verify this is ok with the wide angle lens. If not, dynamically compute
      //       head angle so that it is at the bottom (for high blocks) or top (for low blocks)
      //       of the image.
      Radians headAngle = DEG_TO_RAD_F32(-15);
      if (heightDiff > 0) {
        headAngle = DEG_TO_RAD_F32(17);
      }
      return headAngle;
    }
    
    ActionResult FaceObjectAction::Init(Robot &robot)
    {
      
      ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("FaceObjectAction.Init.ObjectNotFound",
                          "Object with ID=%d no longer exists in the world.",
                          _objectID.GetValue());
        return ActionResult::FAILURE_ABORT;
      }
      
      Pose3d objectPoseWrtRobot;
      if(_whichCode == Vision::Marker::ANY_CODE) {
        if(false == object->GetPose().GetWithRespectTo(robot.GetPose(), objectPoseWrtRobot)) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.ObjectPoseOriginProblem",
                            "Could not get pose of object %d w.r.t. robot pose.",
                            _objectID.GetValue());
          return ActionResult::FAILURE_ABORT;
        }
      } else {
        // Use the closest marker with the specified code:
        std::vector<Vision::KnownMarker*> const& markers = object->GetMarkersWithCode(_whichCode);
        
        if(markers.empty()) {
          PRINT_NAMED_ERROR("FaceObjectAction.Init.NoMarkersWithCode",
                            "Object %d does not have any markers with code %d.",
                            _objectID.GetValue(), _whichCode);
          return ActionResult::FAILURE_ABORT;
        }
        
        Vision::KnownMarker* closestMarker = nullptr;
        if(markers.size() == 1) {
          closestMarker = markers.front();
          if(false == closestMarker->GetPose().GetWithRespectTo(robot.GetPose(), objectPoseWrtRobot)) {
            PRINT_NAMED_ERROR("FaceObjectAction.Init.MarkerOriginProblem",
                              "Could not get pose of marker with code %d of object %d "
                              "w.r.t. robot pose.", _whichCode, _objectID.GetValue() );
            return ActionResult::FAILURE_ABORT;
          }
        } else {
          f32 closestDist = std::numeric_limits<f32>::max();
          Pose3d markerPoseWrtRobot;
          for(auto marker : markers) {
            if(false == marker->GetPose().GetWithRespectTo(robot.GetPose(), markerPoseWrtRobot)) {
              PRINT_NAMED_ERROR("FaceObjectAction.Init.MarkerOriginProblem",
                                "Could not get pose of marker with code %d of object %d "
                                "w.r.t. robot pose.", _whichCode, _objectID.GetValue() );
              return ActionResult::FAILURE_ABORT;
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
                            "No closest marker found for object %d.", _objectID.GetValue());
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      // Have to set the parent class's pose before calling its Init()
      SetPose(objectPoseWrtRobot);
      
      ActionResult facePoseInitResult = FacePoseAction::Init(robot);
      if(ActionResult::SUCCESS != facePoseInitResult) {
        return facePoseInitResult;
      }
      
      // Can't track head to an object and face it
      robot.DisableTrackToObject();
      
      // Disable completion signals since this is inside another action
      _visuallyVerifyAction.SetIsPartOfCompoundAction(true);
      
      return ActionResult::SUCCESS;
    } // FaceObjectAction::Init()
    
    
    ActionResult FaceObjectAction::CheckIfDone(Robot& robot)
    {
      // Tick the compound action until it completes
      if(!_facePoseCompoundActionDone) {
        ActionResult compoundResult = FacePoseAction::CheckIfDone(robot);
        
        if(compoundResult != ActionResult::SUCCESS) {
          return compoundResult;
        } else {
          _facePoseCompoundActionDone = true;

          // Go ahead and do a first tick of visual verification's Update, to
          // get it initialized
          ActionResult verificationResult = _visuallyVerifyAction.Update(robot);
          if(ActionResult::SUCCESS != verificationResult) {
            return verificationResult;
          }
        }
      }

      // If we get here, _compoundAction completed returned SUCCESS. So we can
      // can continue with our additional checks:
      if (_visuallyVerifyWhenDone) {
        ActionResult verificationResult = _visuallyVerifyAction.Update(robot);
        if (verificationResult != ActionResult::SUCCESS) {
          return verificationResult;
        } else {
          _visuallyVerifyWhenDone = false;
        }
      }

      if(_headTrackWhenDone) {
        if(robot.EnableTrackToObject(_objectID, true) == RESULT_OK) {
          return ActionResult::SUCCESS;
        } else {
          PRINT_NAMED_WARNING("FaceObjectAction.CheckIfDone.HeadTracKFail",
                              "Failed to enable head tracking when done.\n");
          return ActionResult::FAILURE_PROCEED;
        }
      }
      
      return ActionResult::SUCCESS;
    } // FaceObjectAction::CheckIfDone()


    const std::string& FaceObjectAction::GetName() const
    {
      static const std::string name("FaceObjectAction");
      return name;
    }
    
    void FaceObjectAction::GetCompletionStruct(Robot& robot, ActionCompletedStruct& completionInfo) const
    {
      completionInfo.numObjects = 1;
      completionInfo.objectIDs[0] = _objectID;
    }
    
    
#pragma mark ---- VisuallyVerifyObjectAction ----
    
    VisuallyVerifyObjectAction::VisuallyVerifyObjectAction(ObjectID objectID,
                                                           Vision::Marker::Code whichCode)
    : _objectID(objectID)
    , _whichCode(whichCode)
    , _waitToVerifyTime(-1)
    , _moveLiftToHeightAction(MoveLiftToHeightAction::Preset::OUT_OF_FOV)
    , _moveLiftToHeightActionDone(false)
    {
      
    }
    
    const std::string& VisuallyVerifyObjectAction::GetName() const
    {
      static const std::string name("VisuallyVerifyObject" + std::to_string(_objectID.GetValue())
                                    + "Action");
      return name;
    }
    

    ActionResult VisuallyVerifyObjectAction::Init(Robot& robot)
    {
      // Get lift out of the way
      _moveLiftToHeightAction.SetIsPartOfCompoundAction(true);
      _moveLiftToHeightActionDone = false;
      _waitToVerifyTime = -1.f;
      
      // Go ahead and do the first update on moving the lift, so we don't "waste"
      // the first tick of CheckIfDone initializing the sub-action.
      ActionResult moveLiftInitResult = _moveLiftToHeightAction.Update(robot);
      if(ActionResult::SUCCESS == moveLiftInitResult ||
         ActionResult::RUNNING == moveLiftInitResult)
      {
        // Continue to CheckIfDone as long as the first Update didn't _fail_
        return ActionResult::SUCCESS;
      } else {
        return moveLiftInitResult;
      }
    }

    
    ActionResult VisuallyVerifyObjectAction::CheckIfDone(Robot& robot)
    {

      ActionResult actionRes = ActionResult::SUCCESS;
      
      if (!_moveLiftToHeightActionDone) {
        actionRes = _moveLiftToHeightAction.Update(robot);
        if (actionRes != ActionResult::SUCCESS) {
          if (actionRes != ActionResult::RUNNING) {
            PRINT_NAMED_WARNING("VisuallyVerifyObjectAction.CheckIfDone.CompoundActionFailed",
                              "Failed to move lift out of FOV. Action result = %d\n", actionRes);
          }
          return actionRes;
        }
        _moveLiftToHeightActionDone = true;
      }

      
      // While head is moving to verification angle, this shouldn't count towards the waitToVerifyTime
      // TODO: Should this check if it's moving at all?
      if (robot.IsHeadMoving()) {
        _waitToVerifyTime = -1;
      }
      
      const f32 currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_waitToVerifyTime < 0.f) {
        _waitToVerifyTime = currentTime + GetWaitToVerifyTime();
      }
      
      // Verify that we can see the object we were interested in
      ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("VisuallyVerifyObjectAction.CheckIfDone.ObjectNotFound",
                          "Object with ID=%d no longer exists in the world.",
                          _objectID.GetValue());
        return ActionResult::FAILURE_ABORT;
      }
      
      const TimeStamp_t lastObserved = object->GetLastObservedTime();
      if (lastObserved < robot.GetLastImageTimeStamp() - DOCK_OBJECT_LAST_OBSERVED_TIME_THRESH_MS)
      {
        PRINT_NAMED_WARNING("VisuallyVerifyObjectAction.CheckIfDone.ObjectNotFound",
                            "Object still exists, but not seen since %d (Current time = %d)",
                            lastObserved, robot.GetLastImageTimeStamp());
        actionRes = ActionResult::FAILURE_ABORT;
      }
      
      if((actionRes != ActionResult::FAILURE_ABORT) && (_whichCode != Vision::Marker::ANY_CODE)) {
        std::vector<const Vision::KnownMarker*> observedMarkers;
        object->GetObservedMarkers(observedMarkers, robot.GetLastImageTimeStamp() - DOCK_OBJECT_LAST_OBSERVED_TIME_THRESH_MS);
        
        bool markerWithCodeSeen = false;
        for(auto marker : observedMarkers) {
          if(marker->GetCode() == _whichCode) {
            markerWithCodeSeen = true;
            break;
          }
        }
        
        if(!markerWithCodeSeen) {
          // TODO: Find causes of this and turn it back into an error/failure (COZMO-140)
          
          std::string observedMarkerNames;
          for(auto marker : observedMarkers) {
            observedMarkerNames += Vision::MarkerTypeStrings[marker->GetCode()];
            observedMarkerNames += " ";
          }
          
          PRINT_NAMED_WARNING("VisuallyVerifyObjectAction.CheckIfDone.MarkerCodeNotSeen",
                              "Object %d observed, but not expected marker: %s. Instead saw: %s",
                              _objectID.GetValue(), Vision::MarkerTypeStrings[_whichCode], observedMarkerNames.c_str());
          //return ActionResult::FAILURE_ABORT;
        }
      }

      if((currentTime < _waitToVerifyTime) && (actionRes != ActionResult::SUCCESS)) {
        return ActionResult::RUNNING;
      }
      
      return actionRes;
    }

    
#pragma mark ---- MoveHeadToAngleAction ----
    
    MoveHeadToAngleAction::MoveHeadToAngleAction(const Radians& headAngle, const f32 tolerance, const Radians& variability)
    : _headAngle(headAngle)
    , _angleTolerance(tolerance)
    , _variability(variability)
    , _name("MoveHeadTo" + std::to_string(std::round(RAD_TO_DEG(_headAngle.ToFloat()))) + "DegAction")
    , _inPosition(false)
    {
      if(_headAngle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor",
                            "Requested head angle (%.1fdeg) less than min head angle (%.1fdeg). Clipping.",
                            _headAngle.getDegrees(), RAD_TO_DEG(MIN_HEAD_ANGLE));
        _headAngle = MIN_HEAD_ANGLE;
      } else if(_headAngle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("MoveHeadToAngleAction.Constructor",
                            "Requested head angle (%.1fdeg) more than max head angle (%.1fdeg). Clipping.",
                            _headAngle.getDegrees(), RAD_TO_DEG(MAX_HEAD_ANGLE));
        _headAngle = MAX_HEAD_ANGLE;
      }
      
      if(_variability > 0) {
        _headAngle += _rng.RandDblInRange(-_variability.ToDouble(),
                                                       _variability.ToDouble());
        _headAngle = CLIP(_headAngle, MIN_HEAD_ANGLE, MAX_HEAD_ANGLE);
      }
    }
    
    bool MoveHeadToAngleAction::IsHeadInPosition(const Robot& robot) const
    {
      const bool inPosition = NEAR(robot.GetHeadAngle(), _headAngle.ToFloat(),
                                   _angleTolerance.ToFloat());
      
      return inPosition;
    }
    
    
    ActionResult MoveHeadToAngleAction::Init(Robot &robot)
    {
      ActionResult result = ActionResult::SUCCESS;
      
      _inPosition = IsHeadInPosition(robot);
      
      if(!_inPosition) {
        // TODO: Add ability to specify speed/accel
        if(robot.MoveHeadToAngle(_headAngle.ToFloat(), 15, 20) != RESULT_OK) {
          result = ActionResult::FAILURE_ABORT;
        }
      }
      
      return result;
    }
    
    ActionResult MoveHeadToAngleAction::CheckIfDone(Robot &robot)
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_inPosition) {
        _inPosition = IsHeadInPosition(robot);
      }
      
      // Wait to get a state message back from the physical robot saying its head
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(_inPosition) {
        result = ActionResult::SUCCESS;
      } else {
        PRINT_NAMED_INFO("MoveHeadToAngleAction.CheckIfDone",
                         "Waiting for head to get in position: %.1fdeg vs. %.1fdeg(+/-%.1f)",
                         RAD_TO_DEG(robot.GetHeadAngle()), _headAngle.getDegrees(), _variability.getDegrees());
      }
      
      return result;
    }
         
#pragma mark ---- MoveLiftToHeightAction ----
                                
    MoveLiftToHeightAction::MoveLiftToHeightAction(const f32 height_mm, const f32 tolerance_mm, const f32 variability)
    : _height_mm(height_mm)
    , _heightTolerance(tolerance_mm)
    , _variability(variability)
    , _name("MoveLiftTo" + std::to_string(_height_mm) + "mmAction")
    , _inPosition(false)
    {
      
    }
    
    MoveLiftToHeightAction::MoveLiftToHeightAction(const Preset preset, const f32 tolerance_mm)
    : MoveLiftToHeightAction(GetPresetHeight(preset), tolerance_mm, 0.f)
    {
      _name = "MoveLiftTo";
      _name += GetPresetName(preset);
    }
    
    
    f32 MoveLiftToHeightAction::GetPresetHeight(Preset preset)
    {
      static const std::map<Preset, f32> LUT = {
        {Preset::LOW_DOCK,   LIFT_HEIGHT_LOWDOCK},
        {Preset::HIGH_DOCK,  LIFT_HEIGHT_HIGHDOCK},
        {Preset::CARRY,      LIFT_HEIGHT_CARRY},
        {Preset::OUT_OF_FOV, -1.f},
      };
      
      return LUT.at(preset);
    }
    
    const std::string& MoveLiftToHeightAction::GetPresetName(Preset preset)
    {
      static const std::map<Preset, std::string> LUT = {
        {Preset::LOW_DOCK,   "LowDock"},
        {Preset::HIGH_DOCK,  "HighDock"},
        {Preset::CARRY,      "HeightCarry"},
        {Preset::OUT_OF_FOV, "OutOfFOV"},
      };
      
      static const std::string unknown("UnknownPreset");
      
      auto iter = LUT.find(preset);
      if(iter == LUT.end()) {
        return unknown;
      } else {
        return iter->second;
      }
    }
    
    bool MoveLiftToHeightAction::IsLiftInPosition(const Robot& robot) const
    {
      const bool inPosition = (NEAR(_heightWithVariation, robot.GetLiftHeight(), _heightTolerance) &&
                               !robot.IsLiftMoving());
      
      return inPosition;
    }
    
    ActionResult MoveLiftToHeightAction::Init(Robot& robot)
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if(_height_mm < 0.f) {
        // Choose whatever is closer to current height, LOW or CARRY:
        const f32 currentHeight = robot.GetLiftHeight();
        const f32 low   = GetPresetHeight(Preset::LOW_DOCK);
        const f32 carry = GetPresetHeight(Preset::CARRY);
        // Absolute values here shouldn't be necessary, since these are supposed
        // to be the lowest and highest possible lift settings, but just in case...
        if( std::abs(currentHeight-low) < std::abs(carry-currentHeight)) {
          _heightWithVariation = low;
        } else {
          _heightWithVariation = carry;
        }
      } else {
        _heightWithVariation = _height_mm;
        if(_variability > 0.f) {
          _heightWithVariation += _rng.RandDblInRange(-_variability, _variability);
        }
        _heightWithVariation = CLIP(_heightWithVariation, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
      }
      
      _inPosition = IsLiftInPosition(robot);
      
      if(!_inPosition) {
        // TODO: Add ability to specify speed/accel
        if(robot.MoveLiftToHeight(_heightWithVariation, 10, 20) != RESULT_OK) {
          result = ActionResult::FAILURE_ABORT;
        }
      }
      
      return result;
    }
    
    ActionResult MoveLiftToHeightAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = ActionResult::RUNNING;
      
      if(!_inPosition) {
        _inPosition = IsLiftInPosition(robot);
      }
      
      // TODO: Somehow verify robot got command to move lift before declaring success
      /*
      // Wait for the lift to start moving (meaning robot received command) and
      // then stop moving
      static bool liftStartedMoving = false;
      if(!liftStartedMoving) {
        liftStartedMoving = robot.IsLiftMoving();
      }
      else
       */
      
      if(_inPosition) {
        result = ActionResult::SUCCESS;
      } else {
        PRINT_NAMED_INFO("MoveLiftToHeightAction.CheckIfDone",
                         "Waiting for lift to get in position: %.1fmm vs. %.1fmm(+/-%.1f)",
                         robot.GetLiftHeight(), _height_mm, _variability);
      }
      
      return result;
    }
    
    
#pragma mark ---- IDockAction ----
    
    IDockAction::IDockAction(ObjectID objectID,
                             const bool useManualSpeed,
                             const f32 placementOffsetX_mm,
                             const f32 placementOffsetY_mm,
                             const f32 placementOffsetAngle_rad,
                             const f32 placeObjectOnGroundIfCarrying)
    : _dockObjectID(objectID)
    , _dockMarker(nullptr)
    , _preActionPoseAngleTolerance(DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE)
    , _wasPickingOrPlacing(false)
    , _useManualSpeed(useManualSpeed)
    , _faceAndVerifyAction(nullptr)
    , _placementOffsetX_mm(placementOffsetX_mm)
    , _placementOffsetY_mm(placementOffsetY_mm)
    , _placementOffsetAngle_rad(placementOffsetAngle_rad)
    , _placeObjectOnGroundIfCarrying(placeObjectOnGroundIfCarrying)
    {
      
    }
    
    IDockAction::~IDockAction()
    {
      if(_faceAndVerifyAction != nullptr) {
        delete _faceAndVerifyAction;
      }
    }
    
    void IDockAction::Reset()
    {
      IAction::Reset();
      if(_faceAndVerifyAction != nullptr) {
        delete _faceAndVerifyAction;
        _faceAndVerifyAction = nullptr;
      }
    }
    
    void IDockAction::SetPreActionPoseAngleTolerance(Radians angleTolerance)
    {
      _preActionPoseAngleTolerance = angleTolerance;
    }
    
    ActionResult IDockAction::Init(Robot& robot)
    {
      _waitToVerifyTime = -1.f;
    
      // Make sure the object we were docking with still exists in the world
      ActionableObject* dockObject = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_dockObjectID));
      if(dockObject == nullptr) {
        PRINT_NAMED_ERROR("IDockAction.Init.ActionObjectNotFound",
                          "Action object with ID=%d no longer exists in the world.",
                          _dockObjectID.GetValue());
        
        return ActionResult::FAILURE_ABORT;
      }
      
      // Verify that we ended up near enough a PreActionPose of the right type
      std::vector<PreActionPose> preActionPoses;
      std::vector<std::pair<Quad2f, ObjectID> > obstacles;
      robot.GetBlockWorld().GetObstacles(obstacles);
      dockObject->GetCurrentPreActionPoses(preActionPoses, {GetPreActionType()},
                                           std::set<Vision::Marker::Code>(), obstacles, nullptr, _placementOffsetX_mm);
      
      if(preActionPoses.empty()) {
        PRINT_NAMED_ERROR("IDockAction.Init.NoPreActionPoses",
                          "Action object with ID=%d returned no pre-action poses of the given type.",
                          _dockObjectID.GetValue());
        
        return ActionResult::FAILURE_ABORT;
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
                              "Could not get pre-action pose w.r.t. robot parent.");
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
      
      f32 preActionPoseDistThresh = ComputePreActionPoseDistThreshold(robot.GetPose(), dockObject,
                                                                      _preActionPoseAngleTolerance);
      
      if(preActionPoseDistThresh > 0.f && closestPoint > preActionPoseDistThresh) {
        PRINT_NAMED_INFO("IDockAction.Init.TooFarFromGoal",
                         "Robot is too far from pre-action pose (%.1fmm, %.1fmm).",
                         closestPoint.x(), closestPoint.y());
        return ActionResult::FAILURE_RETRY;
      }
      else {
        if(SelectDockAction(robot, dockObject) != RESULT_OK) {
          PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.DockActionSelectionFailure",
                            "");
          return ActionResult::FAILURE_ABORT;
        }
        
        PRINT_NAMED_INFO("IDockAction.Init.BeginDocking",
                         "Robot is within (%.1fmm,%.1fmm) of the nearest pre-action pose, "
                         "proceeding with docking.", closestPoint.x(), closestPoint.y());
        
        // Set dock markers
        _dockMarker = preActionPoses[closestIndex].GetMarker();
        _dockMarker2 = GetDockMarker2(preActionPoses, closestIndex);
        
        // Set up a visual verification action to make sure we can still see the correct
        // marker of the selected object before proceeding
        // NOTE: This also disables tracking head to object if there was any
        _faceAndVerifyAction = new FaceObjectAction(_dockObjectID,
                                                    _dockMarker->GetCode(),
                                                    0, 0, true, false);

        // Disable the visual verification from issuing a completion signal
        _faceAndVerifyAction->SetIsPartOfCompoundAction(true);
        
        // Go ahead and Update the FaceObjectAction once now, so we don't
        // waste a tick doing so in CheckIfDone (since this is the first thing
        // that will be done in CheckIfDone anyway)
        ActionResult faceObjectResult = _faceAndVerifyAction->Update(robot);
        
        if(ActionResult::SUCCESS == faceObjectResult ||
           ActionResult::RUNNING == faceObjectResult)
        {
          return ActionResult::SUCCESS;
        } else {
          return faceObjectResult;
        }

      }
      
    } // Init()
    
    
    ActionResult IDockAction::CheckIfDone(Robot& robot)
    {
      ActionResult actionResult = ActionResult::RUNNING;
      
      // Wait for visual verification to complete successfully before telling
      // robot to dock and continuing to check for completion
      if(_faceAndVerifyAction != nullptr) {
        actionResult = _faceAndVerifyAction->Update(robot);
        if(actionResult == ActionResult::RUNNING) {
          return actionResult;
        } else {
          if(actionResult == ActionResult::SUCCESS) {
            // Finished with visual verification:
            delete _faceAndVerifyAction;
            _faceAndVerifyAction = nullptr;
            actionResult = ActionResult::RUNNING;
            
            PRINT_NAMED_INFO("IDockAction.DockWithObjectHelper.BeginDocking", "Docking with marker %d (%s) using action %s.",
              _dockMarker->GetCode(), Vision::MarkerTypeStrings[_dockMarker->GetCode()], DockActionToString(_dockAction));
            
            if(robot.DockWithObject(_dockObjectID,
                                    _dockMarker, _dockMarker2,
                                    _dockAction,
                                    _placementOffsetX_mm,
                                    _placementOffsetY_mm,
                                    _placementOffsetAngle_rad,
                                    _useManualSpeed) == RESULT_OK)
            {
              //NOTE: Any completion (success or failure) after this point should tell
              // the robot to stop tracking and go back to looking for markers!
              _wasPickingOrPlacing = false;
            } else {
              return ActionResult::FAILURE_ABORT;
            }

          } else {
            PRINT_NAMED_ERROR("IDockAction.CheckIfDone.VisualVerifyFailed",
                              "VisualVerification of object failed, stopping IDockAction.");
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
        
        // While head is moving to verification angle, this shouldn't count towards the waitToVerifyTime
        if (robot.IsHeadMoving()) {
          _waitToVerifyTime = -1;
        }
        
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
          //PRINT_NAMED_INFO("IDockAction.CheckIfDone",
          //                 "Robot has stopped moving and picking/placing. Will attempt to verify success.");
          
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
        robot.AbortDrivingToPose();
      }
      if(robot.IsPickingOrPlacing()) {
        robot.AbortDocking();
      }
    }
    
#pragma mark ---- PickAndPlaceObjectAction ----
    
    PickAndPlaceObjectAction::PickAndPlaceObjectAction(ObjectID objectID,
                                                       const bool useManualSpeed,
                                                       const f32 placementOffsetX_mm,
                                                       const f32 placementOffsetY_mm,
                                                       const f32 placementOffsetAngle_rad,
                                                       const bool placeObjectOnGroundIfCarrying)
    : IDockAction(objectID, useManualSpeed, placementOffsetX_mm, placementOffsetY_mm, placementOffsetAngle_rad, placeObjectOnGroundIfCarrying)
    , _placementVerifyAction(nullptr)
    , _verifyComplete(false)
    {
      
    }
    
    PickAndPlaceObjectAction::~PickAndPlaceObjectAction()
    {
      if(_placementVerifyAction != nullptr) {
        delete _placementVerifyAction;
      }
    }
    
    void PickAndPlaceObjectAction::Reset()
    {
      IDockAction::Reset();
      
      if(_placementVerifyAction != nullptr) {
        delete _placementVerifyAction;
        _placementVerifyAction = nullptr;
      }
    }
    
    const std::string& PickAndPlaceObjectAction::GetName() const
    {
      static const std::string name("PickAndPlaceObjectAction");
      return name;
    }
    
    RobotActionType PickAndPlaceObjectAction::GetType() const
    {
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_HIGH:
          return RobotActionType::PICKUP_OBJECT_HIGH;
          
        case DockAction::DA_PICKUP_LOW:
          return RobotActionType::PICKUP_OBJECT_LOW;
          
        case DockAction::DA_PLACE_HIGH:
          return RobotActionType::PLACE_OBJECT_HIGH;
          
        case DockAction::DA_PLACE_LOW:
          return RobotActionType::PLACE_OBJECT_LOW;
          
        default:
          PRINT_NAMED_WARNING("PickAndPlaceObjectAction.GetType",
                              "Dock action not set before determining action type.");
          return RobotActionType::PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void PickAndPlaceObjectAction::GetCompletionStruct(Robot& robot, ActionCompletedStruct& completionInfo) const
    {
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_HIGH:
        case DockAction::DA_PICKUP_LOW:
        {
          if(!robot.IsCarryingObject()) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                              "Expecting robot to think it's carrying object for pickup action.");
          } else {
            
            const std::set<ObjectID> carriedObjects = robot.GetCarryingObjects();
            completionInfo.numObjects = carriedObjects.size();
            completionInfo.objectIDs.fill(-1);
            u8 objectCnt = 0;
            for (auto& objID : carriedObjects) {
              completionInfo.objectIDs[objectCnt++] = objID.GetValue();
            }
            
            return;
          }
          break;
        }
        case DockAction::DA_PLACE_HIGH:
        case DockAction::DA_PLACE_LOW:
        {
          // TODO: Be able to fill in more objects in the stack
          ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_dockObjectID);
          if(object == nullptr) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                              "Docking object %d not found in world after placing.",
                              _dockObjectID.GetValue());
          } else {
            
            ActionCompletedStruct completionInfo;
            
            auto objectStackIter = completionInfo.objectIDs.begin();
            completionInfo.objectIDs.fill(-1);
            completionInfo.numObjects = 0;
            while(object != nullptr &&
                  completionInfo.numObjects < completionInfo.objectIDs.size())
            {
              *objectStackIter = object->GetID().GetValue();
              ++objectStackIter;
              ++completionInfo.numObjects;
              object = robot.GetBlockWorld().FindObjectOnTopOf(*object, 15.f);
            }
            return;
          }
          break;
        }
        default:
          PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                            "Dock action not set before filling completion signal.");
      }
      
      IDockAction::GetCompletionStruct(robot, completionInfo);
    }
    
    Result PickAndPlaceObjectAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      // Record the object's original pose (before picking it up) so we can
      // verify later whether we succeeded.
      // Make it w.r.t. robot's parent so we can compare heights fairly.
      if(object->GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), _dockObjectOrigPose) == false) {
        PRINT_NAMED_ERROR("PickAndPlaceObjectAction.SelectDockAction.PoseWrtFailed",
                          "Could not get pose of dock object w.r.t. robot parent.");
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = _dockObjectOrigPose.GetTranslation().z() - robot.GetPose().GetTranslation().z();
      _dockAction = DockAction::DA_PICKUP_LOW;
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        if(robot.IsCarryingObject()) {
          PRINT_STREAM_INFO("PickAndPlaceObjectAction.SelectDockAction", "Already carrying object. Can't dock to high object. Aborting.");
          return RESULT_FAIL;
          
        } else {
          _dockAction = DockAction::DA_PICKUP_HIGH;
        }
      } else if (robot.IsCarryingObject()) {
        _dockAction = _placeObjectOnGroundIfCarrying ? DockAction::DA_PLACE_LOW : DockAction::DA_PLACE_HIGH;
        
        // Need to record the object we are currently carrying because it
        // will get unset when the robot unattaches it during placement, and
        // we want to be able to verify that we're seeing what we just placed.
        _carryObjectID     = robot.GetCarryingObject();
        _carryObjectMarker = robot.GetCarryingMarker();
      }
      
      return RESULT_OK;
    } // SelectDockAction()

    
    ActionResult PickAndPlaceObjectAction::Verify(Robot& robot)
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_LOW:
        case DockAction::DA_PICKUP_HIGH:
        {
          if(robot.IsCarryingObject() == false) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify.RobotNotCarryingObject",
                              "Expecting robot to think it's carrying an object at this point.");
            result = ActionResult::FAILURE_RETRY;
            break;
          }
          
          BlockWorld& blockWorld = robot.GetBlockWorld();
          
          // We should _not_ still see a object with the
          // same type as the one we were supposed to pick up in that
          // block's original position because we should now be carrying it.
          ObservableObject* carryObject = blockWorld.GetObjectByID(robot.GetCarryingObject());
          if(carryObject == nullptr) {
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify.CarryObjectNoLongerExists",
                              "Object %d we were carrying no longer exists in the world.",
                              robot.GetCarryingObject().GetValue());
            result = ActionResult::FAILURE_ABORT;
            break;
          }
          
          const BlockWorld::ObjectsMapByID_t& objectsWithType = blockWorld.GetExistingObjectsByType(carryObject->GetType());
          
          Vec3f Tdiff;
          Radians angleDiff;
          ObservableObject* objectInOriginalPose = nullptr;
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
              PRINT_STREAM_INFO("PickAndPlaceObjectAction.Verify", std::setprecision(3) << "Seeing object " << object.first.GetValue() << " in original pose. (Tdiff = (" << Tdiff.x() << "," << Tdiff.y() << "," << Tdiff.z() << "), AngleDiff=" << angleDiff.getDegrees() << "deg");
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
              PRINT_STREAM_INFO("PickAndPlaceObjectAction.Verify", "Moving carried object to object seen in original pose and clearing that object.");
              carryObject->SetPose(objectInOriginalPose->GetPose());
              blockWorld.ClearObject(objectInOriginalPose->GetID());
            }
            robot.UnSetCarryingObjects();
            
            PRINT_STREAM_INFO("PickAndPlaceObjectAction.Verify", "Object pick-up FAILED! (Still seeing object in same place.)");
            result = ActionResult::FAILURE_RETRY;
          } else {
            //_carryingObjectID = _dockObjectID;  // Already set?
            //_carryingMarker   = _dockMarker;   //   "
            PRINT_STREAM_INFO("PickAndPlaceObjectAction.Verify", "Object pick-up SUCCEEDED!");
            result = ActionResult::SUCCESS;
          }
          break;
        } // PICKUP
          
        case DockAction::DA_PLACE_LOW:
        case DockAction::DA_PLACE_HIGH:
        {
          if(robot.GetLastPickOrPlaceSucceeded()) {
            
            if(robot.IsCarryingObject() == true) {
              PRINT_NAMED_ERROR("PickAndPlaceObjectAction::Verify",
                                "Expecting robot to think it's NOT carrying an object at this point.");
              return ActionResult::FAILURE_ABORT;
            }
            
            // If the physical robot thinks it succeeded, move the lift out of the
            // way, and attempt to visually verify
            if(_placementVerifyAction == nullptr) {
              _placementVerifyAction = new FaceObjectAction(_carryObjectID, Radians(0), Radians(0), true, false);
              
              // Disable completion signals since this is inside another action
              _placementVerifyAction->SetIsPartOfCompoundAction(true);
              
              // Go ahead do the first update of the FaceObjectAction to get the
              // init "out of the way" rather than wasting a tick here
              result = _placementVerifyAction->Update(robot);
              if(ActionResult::SUCCESS != result && ActionResult::RUNNING != result) {
                return result;
              }
            }
            
            result = _placementVerifyAction->Update(robot);
            
            if(result != ActionResult::RUNNING) {
              
              // Visual verification is done
              delete _placementVerifyAction;
              _placementVerifyAction = nullptr;
              
              if(result != ActionResult::SUCCESS) {
                if(_dockAction == DockAction::DA_PLACE_LOW) {
                  PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify",
                                    "Robot thinks it placed the object low, but verification of placement "
                                    "failed. Not sure where carry object %d is, so deleting it.",
                                    _carryObjectID.GetValue());
                  
                  robot.GetBlockWorld().ClearObject(_carryObjectID);
                } else {
                  assert(_dockAction == DockAction::DA_PLACE_HIGH);
                  PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify",
                                    "Robot thinks it placed the object high, but verification of placement "
                                    "failed. Assuming we are still carying object %d.",
                                    _carryObjectID.GetValue());
                  
                  robot.SetObjectAsAttachedToLift(_carryObjectID, _carryObjectMarker);
                }

              }
              else if(_dockAction == DockAction::DA_PLACE_HIGH && !_verifyComplete) {
                
                // If we are placing high and verification succeeded, lower the lift
                _verifyComplete = true;
                
                if(result == ActionResult::SUCCESS) {
                  // Visual verification succeeded, drop lift (otherwise, just
                  // leave it up, since we are assuming we are still carrying the object)
                  _placementVerifyAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
                  
                  // Disable completion signals since this is inside another action
                  _placementVerifyAction->SetIsPartOfCompoundAction(true);
                  
                  result = ActionResult::RUNNING;
                }

              }
            } // if(result != ActionResult::RUNNING)
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track, so we are probably still holding the block
            PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify",
                              "Robot reported placement failure. Assuming docking failed "
                              "and robot is still holding same block.");
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // PLACE

        default:
          PRINT_NAMED_ERROR("PickAndPlaceObjectAction.Verify.ReachedDefaultCase",
            "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
       
    
    
#pragma mark ---- RollObjectAction ----
    
    RollObjectAction::RollObjectAction(ObjectID objectID, const bool useManualSpeed)
    : IDockAction(objectID, useManualSpeed)
    , _rollVerifyAction(nullptr)
    {
      
    }
    
    RollObjectAction::~RollObjectAction()
    {
      if(_rollVerifyAction != nullptr) {
        delete _rollVerifyAction;
      }
    }
    
    void RollObjectAction::Reset()
    {
      IDockAction::Reset();
      
      if(_rollVerifyAction != nullptr) {
        delete _rollVerifyAction;
        _rollVerifyAction = nullptr;
      }
    }
    
    const std::string& RollObjectAction::GetName() const
    {
      static const std::string name("RollObjectAction");
      return name;
    }
    
    RobotActionType RollObjectAction::GetType() const
    {
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
          return RobotActionType::ROLL_OBJECT_LOW;
          
        default:
          PRINT_NAMED_WARNING("RollObjectAction.GetType",
                              "Dock action not set before determining action type.");
          return RobotActionType::PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void RollObjectAction::GetCompletionStruct(Robot& robot, ActionCompletedStruct& completionInfo) const
    {
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
        {
          if(robot.IsCarryingObject()) {
            PRINT_NAMED_ERROR("RollObjectAction.EmitCompletionSignal",
                              "Expecting robot to think it's not carrying object for roll action.");
          } else {
            completionInfo.numObjects = 1;
            completionInfo.objectIDs.fill(-1);
            completionInfo.objectIDs[0] = _dockObjectID;
            return;
          }
          break;
        }
        default:
          PRINT_NAMED_ERROR("PickAndPlaceObjectAction.EmitCompletionSignal",
                            "Dock action not set before filling completion signal.");
      }
      
      IDockAction::GetCompletionStruct(robot, completionInfo);
    }
    
    Result RollObjectAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      // Record the object's original pose (before picking it up) so we can
      // verify later whether we succeeded.
      // Make it w.r.t. robot's parent so we can compare heights fairly.
      if(object->GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), _dockObjectOrigPose) == false) {
        PRINT_NAMED_ERROR("RollObjectAction.SelectDockAction.PoseWrtFailed",
                          "Could not get pose of dock object w.r.t. robot parent.");
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = _dockObjectOrigPose.GetTranslation().z() - robot.GetPose().GetTranslation().z();
      _dockAction = DockAction::DA_ROLL_LOW;
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      // TODO: There might be ways to roll high blocks when not carrying object and low blocks when carrying an object.
      //       Do them later.
      if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        PRINT_STREAM_INFO("RollObjectAction.SelectDockAction", "Object is too high to roll. Aborting.");
        return RESULT_FAIL;
      } else if (robot.IsCarryingObject()) {
        PRINT_STREAM_INFO("RollObjectAction.SelectDockAction", "Can't roll while carrying an object.");
        return RESULT_FAIL;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    
    ActionResult RollObjectAction::Verify(Robot& robot)
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
        {
          if(robot.GetLastPickOrPlaceSucceeded()) {
            
            if(robot.IsCarryingObject() == true) {
              PRINT_NAMED_ERROR("RollObjectAction::Verify",
                                "Expecting robot to think it's NOT carrying an object at this point.");
              return ActionResult::FAILURE_ABORT;
            }
            
            // If the physical robot thinks it succeeded, move the lift out of the
            // way, and attempt to visually verify
            if(_rollVerifyAction == nullptr) {
              _rollVerifyAction = new FaceObjectAction(_dockObjectID, Radians(0), Radians(0), true, false);
              
              // Disable completion signals since this is inside another action
              _rollVerifyAction->SetIsPartOfCompoundAction(true);
            }
            
            result = _rollVerifyAction->Update(robot);
            
            if(result != ActionResult::RUNNING) {
              
              // Visual verification is done
              delete _rollVerifyAction;
              _rollVerifyAction = nullptr;
              
              if(result != ActionResult::SUCCESS) {
                PRINT_NAMED_ERROR("RollObjectAction.Verify",
                                  "Robot thinks it rolled the object, but verification failed. "
                                  "Not sure where rolled object %d is, so deleting it.",
                                  _dockObjectID.GetValue());
                
                robot.GetBlockWorld().ClearObject(_dockObjectID);
              } else {
                // TODO: Need to verify whether or not block is actually in the place and orientation
                //       that is expected. Use _dockObjectOrigPose?
                PRINT_NAMED_WARNING("RollObjectAction.Verify.Todo",
                                    "TODO: Need to verify rolled block orientation is correct. Currently just visually verifying existence");
                
              }
            } // if(result != ActionResult::RUNNING)
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track.
            PRINT_NAMED_ERROR("RollObjectAction.Verify",
                              "Robot reported roll failure. Assuming docking failed");
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // ROLL_LOW

          
        default:
          PRINT_NAMED_ERROR("RollObjectAction.Verify.ReachedDefaultCase",
            "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
    
#pragma mark ---- PlaceObjectOnGroundAction ----
    
    PlaceObjectOnGroundAction::PlaceObjectOnGroundAction()
    : _faceAndVerifyAction(nullptr)
    {
      
    }
    
    PlaceObjectOnGroundAction::~PlaceObjectOnGroundAction()
    {
      if(_faceAndVerifyAction != nullptr) {
        delete _faceAndVerifyAction;
      }
    }
    
    void PlaceObjectOnGroundAction::Reset()
    {
      IAction::Reset();
      
      if(_faceAndVerifyAction != nullptr) {
        delete _faceAndVerifyAction;
        _faceAndVerifyAction = nullptr;
      }
    }
    
    const std::string& PlaceObjectOnGroundAction::GetName() const
    {
      static const std::string name("PlaceObjectOnGroundAction");
      return name;
    }
   
    ActionResult PlaceObjectOnGroundAction::Init(Robot& robot)
    {
      ActionResult result = ActionResult::RUNNING;
      
      // Robot must be carrying something to put something down!
      if(robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d executing PlaceObjectOnGroundAction but not carrying object.", robot.GetID());
        result = ActionResult::FAILURE_ABORT;
      } else {
        
        _carryingObjectID  = robot.GetCarryingObject();
        _carryObjectMarker = robot.GetCarryingMarker();
        
        if(robot.PlaceObjectOnGround() == RESULT_OK)
        {
          result = ActionResult::SUCCESS;
        } else {
          PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckPreconditions.SendPlaceObjectOnGroundFailed",
                            "Robot's SendPlaceObjectOnGround method reported failure.");
          result = ActionResult::FAILURE_ABORT;
        }
        
        _faceAndVerifyAction = new FaceObjectAction(_carryingObjectID, _carryObjectMarker->GetCode(), 0, 0, true, false);
        _faceAndVerifyAction->SetIsPartOfCompoundAction(true);
        
      } // if/else IsCarryingObject()
      
      // If we were moving, stop moving.
      robot.StopAllMotors();
      
      return result;
      
    } // CheckPreconditions()
    
    
    
    ActionResult PlaceObjectOnGroundAction::CheckIfDone(Robot& robot)
    {
      ActionResult actionResult = ActionResult::RUNNING;
      
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

        actionResult = _faceAndVerifyAction->Update(robot);

        if(actionResult != ActionResult::RUNNING && actionResult != ActionResult::SUCCESS) {
          PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckIfDone",
                            "VerityObjectPlaceHelper reported failure, just deleting object %d.",
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
      _dockAction = DockAction::DA_CROSS_BRIDGE;
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult CrossBridgeAction::Verify(Robot& robot)
    {
      // TODO: Need some kind of verificaiton here?
      PRINT_NAMED_INFO("CrossBridgeAction.Verify.BridgeCrossingComplete",
                       "Robot has completed crossing a bridge.");
      return ActionResult::SUCCESS;
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
                          "Could not cast generic ActionableObject into Ramp object.");
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      // Choose ascent or descent
      const Ramp::TraversalDirection direction = ramp->WillAscendOrDescend(robot.GetPose());
      switch(direction)
      {
        case Ramp::ASCENDING:
          _dockAction = DockAction::DA_RAMP_ASCEND;
          break;
          
        case Ramp::DESCENDING:
          _dockAction = DockAction::DA_RAMP_DESCEND;
          break;
          
        case Ramp::UNKNOWN:
        default:
          result = RESULT_FAIL;
      }
    
      // Tell robot which ramp it will be using, and in which direction
      robot.SetRamp(_dockObjectID, direction);
            
      return result;
      
    } // SelectDockAction()
    
    
    ActionResult AscendOrDescendRampAction::Verify(Robot& robot)
    {
      // TODO: Need to do some kind of verification here?
      PRINT_NAMED_INFO("AscendOrDescendRampAction.Verify.RampAscentOrDescentComplete",
                       "Robot has completed going up/down ramp.");
      
      return ActionResult::SUCCESS;
    } // Verify()
    
    
#pragma mark ---- MountChargerAction ----
    
    MountChargerAction::MountChargerAction(ObjectID chargerID, const bool useManualSpeed)
    : IDockAction(chargerID, useManualSpeed)
    {
      
    }
    
    const std::string& MountChargerAction::GetName() const
    {
      static const std::string name("MountChargerAction");
      return name;
    }
    
    Result MountChargerAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      Charger* charger = dynamic_cast<Charger*>(object);
      if(charger == nullptr) {
        PRINT_NAMED_ERROR("MountChargerAction.SelectDockAction.NotChargerObject",
                          "Could not cast generic ActionableObject into Charger object.");
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      _dockAction = DockAction::DA_MOUNT_CHARGER;
      
      // Tell robot which ramp it will be using, and in which direction
      //robot.SetRamp(_dockObjectID, direction);
      
      return result;
      
    } // SelectDockAction()
    
    
    ActionResult MountChargerAction::Verify(Robot& robot)
    {
      // TODO: Need to do some kind of verification here?
      PRINT_NAMED_INFO("MountChargerAction.Verify.MountingChargerComplete",
                       "Robot has mounted charger.");
      
      return ActionResult::SUCCESS;
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
    
    ActionResult TraverseObjectAction::UpdateInternal(Robot& robot)
    {
      // Select the chosen action based on the object's type, if we haven't
      // already
      if(_chosenAction == nullptr) {
        ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("TraverseObjectAction.Init.ObjectNotFound",
                            "Could not get actionable object with ID = %d from world.", _objectID.GetValue());
          return ActionResult::FAILURE_ABORT;
        }
        
        if(object->GetType() == ObjectType::Bridge_LONG ||
           object->GetType() == ObjectType::Bridge_SHORT)
        {
          _chosenAction = new CrossBridgeAction(_objectID, _useManualSpeed);
        }
        else if(object->GetType() == ObjectType::Ramp_Basic) {
          _chosenAction = new AscendOrDescendRampAction(_objectID, _useManualSpeed);
        }
        else if(object->GetType() == ObjectType::Charger_Basic) {
          _chosenAction = new MountChargerAction(_objectID, _useManualSpeed);
        }
        else {
          PRINT_NAMED_ERROR("TraverseObjectAction.Init.CannotTraverseObjectType",
                            "Robot %d was asked to traverse object ID=%d of type %s, but "
                            "that traversal is not defined.", robot.GetID(),
                            object->GetID().GetValue(), ObjectTypeToString(object->GetType()));
          
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      // Now just use chosenAction's Update()
      assert(_chosenAction != nullptr);
      return _chosenAction->Update(robot);
      
    } // Update()
    
    
#pragma mark ---- PlayAnimationAction ----
    
    PlayAnimationAction::PlayAnimationAction(const std::string& animName,
                                             const u32 numLoops)
    : _animName(animName)
    , _name("PlayAnimation" + animName + "Action")
    , _numLoops(numLoops)
    {
      
    }
    
    ActionResult PlayAnimationAction::Init(Robot& robot)
    {
      _startedPlaying = false;
      _animTag = robot.PlayAnimation(_animName, _numLoops);
      if(_animTag != 0) {
        return ActionResult::SUCCESS;
      } else {
        return ActionResult::FAILURE_ABORT;
      }
    }
    
    ActionResult PlayAnimationAction::CheckIfDone(Robot& robot)
    {
      // Wait for the current animation tag to match the one corresponding to
      // this action, so we know the robot has actually started playing _this_
      // animation.
      if(!_startedPlaying)
      {
        if(robot.GetCurrentAnimationTag() == _animTag) {
          _startedPlaying = true;
        } else {
          PRINT_NAMED_INFO("PlayAnimationAction.CheckIfDone.WaitForStart",
                           "Waiting for robot to actually start animating '%s' with tag=%d (current=%d).",
                           _animName.c_str(), _animTag, robot.GetCurrentAnimationTag());
          return ActionResult::RUNNING;
        }
      }
      
      // If we've made it this far, we must have started the expected animation,
      // so just wait for the current animation tag to change to know when it is
      // done playing on the robot.
      assert(_startedPlaying);
      if(robot.GetCurrentAnimationTag() == _animTag)
      {
        return ActionResult::RUNNING;
      } else {
        return ActionResult::SUCCESS;
      }
    }
    
    void PlayAnimationAction::Cleanup(Robot& robot)
    {
      // Abort anything that shouldn't still be running
      if(robot.IsAnimating()) {
        robot.AbortAnimation();
      }
    }
    
    void PlayAnimationAction::GetCompletionStruct(Robot& robot, ActionCompletedStruct& completionInfo) const
    {
      completionInfo.animName = _animName;
    }
    
#pragma mark ---- PlaySoundAction ----
    
    PlaySoundAction::PlaySoundAction(const std::string& soundName)
    : _soundName(soundName)
    , _name("PlaySound" + soundName + "Action")
    {
      
    }

    ActionResult PlaySoundAction::CheckIfDone(Robot& robot)
    {
      // TODO: Implement!
      return ActionResult::FAILURE_ABORT;
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
    
    ActionResult WaitAction::Init(Robot& robot)
    {
      _doneTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _waitTimeInSeconds;
      return ActionResult::SUCCESS;
    }
    
    ActionResult WaitAction::CheckIfDone(Robot& robot)
    {
      assert(_doneTimeInSeconds > 0.f);
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _doneTimeInSeconds) {
        return ActionResult::SUCCESS;
      } else {
        return ActionResult::RUNNING;
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
