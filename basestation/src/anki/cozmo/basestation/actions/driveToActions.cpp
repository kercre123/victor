/**
 * File: driveToActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements drive-to cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "driveToActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"


namespace Anki {
  
  namespace Cozmo {
    
    // Computes that angle (wrt world) at which the robot would have to approach the given pose
    // such that it places the carried object at the given pose
    Result ComputePlacementApproachAngle(const Robot& robot, const Pose3d& placementPose, f32& approachAngle_rad)
    {
      // Get carried object
      ObjectID objectID = robot.GetCarryingObject();
      if (objectID.GetValue() < 0) {
        PRINT_NAMED_INFO("ComputePlacementApproachAngle.NoCarriedObject", "");
        return RESULT_FAIL;
      }
      
      const ActionableObject* object = dynamic_cast<const ActionableObject*>(robot.GetBlockWorld().GetObjectByID(objectID));
      
      
      // Check that up axis of carried object and the desired placementPose are the same.
      // Otherwise, it's impossible for the robot to place it there!
      AxisName targetUpAxis = placementPose.GetRotationMatrix().GetRotatedParentAxis<'Z'>();
      AxisName currentUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
      if (currentUpAxis != targetUpAxis) {
        PRINT_NAMED_WARNING("ComputePlacementApproachAngle.MismatchedUpAxes",
                            "Carried up axis: %d , target up axis: %d",
                            currentUpAxis, targetUpAxis);
        return RESULT_FAIL;
      }
      
      // Get pose of carried object wrt robot
      Pose3d poseObjectWrtRobot;
      if (!object->GetPose().GetWithRespectTo(robot.GetPose(), poseObjectWrtRobot)) {
        PRINT_NAMED_WARNING("ComputePlacementApproachAngle.FailedToComputeObjectWrtRobotPose", "");
        return RESULT_FAIL;
      }
      
      // Get pose of robot if the carried object were aligned with the placementPose and the robot was still carrying it
      Pose3d poseRobotIfPlacingObject(poseObjectWrtRobot.Invert());
      poseRobotIfPlacingObject.PreComposeWith(placementPose);
      
      // Extra confirmation that the robot is upright in this pose
      assert(poseRobotIfPlacingObject.GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS);
      
      approachAngle_rad = poseRobotIfPlacingObject.GetRotationMatrix().GetAngleAroundParentAxis<'Z'>().ToFloat();
      
      return RESULT_OK;
    }
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(Robot& robot,
                                             const ObjectID& objectID,
                                             const PreActionPose::ActionType& actionType,
                                             const PathMotionProfile motionProfile,
                                             const f32 predockOffsetDistX_mm,
                                             const bool useApproachAngle,
                                             const f32 approachAngle_rad,
                                             const bool useManualSpeed)
    : IAction(robot)
    , _objectID(objectID)
    , _actionType(actionType)
    , _distance_mm(-1.f)
    , _predockOffsetDistX_mm(predockOffsetDistX_mm)
    , _useManualSpeed(useManualSpeed)
    , _compoundAction(robot)
    , _useApproachAngle(useApproachAngle)
    , _approachAngle_rad(approachAngle_rad)
    , _pathMotionProfile(motionProfile)
    {

    }
    
    DriveToObjectAction::DriveToObjectAction(Robot& robot,
                                             const ObjectID& objectID,
                                             const f32 distance,
                                             const PathMotionProfile motionProfile,
                                             const bool useManualSpeed)
    : IAction(robot)
    , _objectID(objectID)
    , _actionType(PreActionPose::ActionType::NONE)
    , _distance_mm(distance)
    , _predockOffsetDistX_mm(0)
    , _useManualSpeed(useManualSpeed)
    , _compoundAction(robot)
    , _useApproachAngle(false)
    , _approachAngle_rad(0)
    , _pathMotionProfile(motionProfile)
    {

    }
    
    const std::string& DriveToObjectAction::GetName() const
    {
      static const std::string name("DriveToObjectAction");
      return name;
    }
    
    void DriveToObjectAction::SetApproachAngle(const f32 angle_rad)
    {
      PRINT_NAMED_INFO("DriveToObjectAction.SetApproachingAngle", "%f rad", angle_rad);
      _useApproachAngle = true;
      _approachAngle_rad = angle_rad;
    }
    
    ActionResult DriveToObjectAction::GetPossiblePoses(ActionableObject* object,
                                                       std::vector<Pose3d>& possiblePoses,
                                                       bool& alreadyInPosition)
    {
      ActionResult result = ActionResult::SUCCESS;
      
      alreadyInPosition = false;
      possiblePoses.clear();
      
      std::vector<PreActionPose> possiblePreActionPoses;
      std::vector<std::pair<Quad2f,ObjectID> > obstacles;
      _robot.GetBlockWorld().GetObstacles(obstacles);
      object->GetCurrentPreActionPoses(possiblePreActionPoses, {_actionType},
                                       std::set<Vision::Marker::Code>(),
                                       obstacles,
                                       &_robot.GetPose(),
                                       _predockOffsetDistX_mm);
      
      // Filter out all but the preActionPose that is closest to the specified approachAngle
      if (_useApproachAngle) {
        bool bestPreActionPoseFound = false;
        for(auto & preActionPose : possiblePreActionPoses)
        {
          Pose3d preActionPoseWrtWorld;
          preActionPose.GetPose().GetWithRespectTo(*_robot.GetWorldOrigin(), preActionPoseWrtWorld);
          
          Radians headingDiff = preActionPoseWrtWorld.GetRotationAngle<'Z'>() - _approachAngle_rad;
          if (std::abs(headingDiff.ToFloat()) < 0.5f * PIDIV2_F) {
            // Found the preAction pose that is most aligned with the desired approach angle
            PreActionPose p(preActionPose);
            possiblePreActionPoses.clear();
            possiblePreActionPoses.push_back(p);
            bestPreActionPoseFound = true;
            break;
          }
        }
        
        if (!bestPreActionPoseFound) {
          PRINT_NAMED_INFO("DriveToObjectAction.GetPossiblePoses.NoPreActionPosesAtApproachAngleExist", "");
          return ActionResult::FAILURE_ABORT;
        }
      }
      
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
        
        Point3f preActionPoseDistThresh = ComputePreActionPoseDistThreshold(_robot.GetPose(), object,
                                                                            DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE);
        
        preActionPoseDistThresh.z() = REACHABLE_PREDOCK_POSE_Z_THRESH_MM;
        for(auto & preActionPose : possiblePreActionPoses)
        {
          Pose3d possiblePose;
          if(preActionPose.GetPose().GetWithRespectTo(*_robot.GetWorldOrigin(), possiblePose) == false) {
            PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.PreActionPoseOriginProblem",
                                "Could not get pre-action pose w.r.t. robot origin.");
            
          } else {
            possiblePoses.emplace_back(possiblePose);
            
            if(preActionPoseDistThresh > 0.f) {
              // Keep track of closest possible pose, in case we are already close
              // enough to it to not bother planning a path at all.
              Vec3f Tdiff;
              Radians angleDiff;
              if(possiblePose.IsSameAs(_robot.GetPose(), preActionPoseDistThresh,
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
    
    ActionResult DriveToObjectAction::InitHelper(ActionableObject* object)
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
          if(false == object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), objectWrtRobotParent)) {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            result = ActionResult::FAILURE_ABORT;
          } else {
            Point2f vec(_robot.GetPose().GetTranslation());
            vec -= Point2f(objectWrtRobotParent.GetTranslation());
            const f32 currentDistance = vec.MakeUnitLength();
            if(currentDistance < _distance_mm) {
              alreadyInPosition = true;
            } else {
              vec *= _distance_mm;
              const Point3f T(vec.x() + objectWrtRobotParent.GetTranslation().x(),
                              vec.y() + objectWrtRobotParent.GetTranslation().y(),
                              _robot.GetPose().GetTranslation().z());
              possiblePoses.push_back(Pose3d(std::atan2f(-vec.y(), -vec.x()), Z_AXIS_3D(), T, objectWrtRobotParent.GetParent()));
            }
            result = ActionResult::SUCCESS;
          }
        }
      } else {
        
        result = GetPossiblePoses(object, possiblePoses, alreadyInPosition);
      }
      
      // In case we are re-running this action, make sure compound actions are cleared.
      // These will do nothing if compoundAction has nothing in it yet (i.e., on first Init)
      _compoundAction.ClearActions();
      _compoundAction.SetSuppressTrackLocking(true);

      if(result == ActionResult::SUCCESS) {
        if(!alreadyInPosition) {

          f32 preActionPoseDistThresh = ComputePreActionPoseDistThreshold(possiblePoses[0], object, DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE);
          
          DriveToPoseAction* driveToPoseAction = new DriveToPoseAction(_robot, _pathMotionProfile, true, _useManualSpeed);
          driveToPoseAction->SetGoals(possiblePoses, preActionPoseDistThresh);
          driveToPoseAction->SetSounds(_startSound, _drivingSound, _stopSound);
          driveToPoseAction->SetDriveSoundSpacing(_drivingSoundSpacingMin_sec, _drivingSoundSpacingMax_sec);
          driveToPoseAction->SetSuppressTrackLocking(true);
          _compoundAction.AddAction(driveToPoseAction);
        }
        
        // Make sure we can see the object, unless we are carrying it (i.e. if we
        // are doing a DriveToPlaceCarriedObject action)
        if(!object->IsBeingCarried()) {
          FaceObjectAction* faceObjectAction = new FaceObjectAction(_robot, _objectID, Radians(0), true, false);
          PRINT_NAMED_DEBUG("IActionRunner.CreatedSubAction", "Parent action [%d] %s created a sub action [%d] %s",
                            GetTag(),
                            GetName().c_str(),
                            faceObjectAction->GetTag(),
                            faceObjectAction->GetName().c_str());
          faceObjectAction->SetSuppressTrackLocking(true);
          _compoundAction.AddAction(faceObjectAction);
        }
        
        _compoundAction.SetEmitCompletionSignal(false);
        
        // Go ahead and do the first Update on the compound action, so we don't
        // "waste" the first CheckIfDone call just initializing it
        result = _compoundAction.Update();
        if(ActionResult::RUNNING == result || ActionResult::SUCCESS == result) {
          result = ActionResult::SUCCESS;
        }
      }
      
      return result;
      
    } // InitHelper()
    
    ActionResult DriveToObjectAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                          "Robot %d's block world does not have an ActionableObject with ID=%d.",
                          _robot.GetID(), _objectID.GetValue());
        
        result = ActionResult::FAILURE_ABORT;
      } else if(ObservableObject::PoseState::Unknown == object->GetPoseState()) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.ObjectPoseStateUnknown",
                          "Robot %d cannot plan a path to ActionableObject %d, whose pose state is Unknown.",
                          _robot.GetID(), _objectID.GetValue());
        result = ActionResult::FAILURE_ABORT;
      } else {
        // Use a helper here so that it can be shared with DriveToPlaceCarriedObjectAction
        result = InitHelper(object);
      } // if/else object==nullptr
      
      return result;
    }
    
    ActionResult DriveToObjectAction::CheckIfDone()
    {
      ActionResult result = _compoundAction.Update();
      
      if(result == ActionResult::SUCCESS) {
        // We completed driving to the pose and visually verifying the object
        // is still there. This could have updated the object's pose (hopefully
        // to a more accurate one), meaning the pre-action pose we selected at
        // Initialization has now moved and we may not be in position, even if
        // we completed the planned path successfully. If that's the case, we
        // want to retry.
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToObjectAction.CheckIfDone.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            _robot.GetID(), _objectID.GetValue());
          
          result = ActionResult::FAILURE_ABORT;
        } else if( _actionType == PreActionPose::ActionType::NONE) {
          
          // Check to see if we got close enough
          Pose3d objectPoseWrtRobotParent;
          if(false == object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), objectPoseWrtRobotParent)) {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            result = ActionResult::FAILURE_ABORT;
          } else {
            const f32 distanceSq = (Point2f(objectPoseWrtRobotParent.GetTranslation()) - Point2f(_robot.GetPose().GetTranslation())).LengthSq();
            if(distanceSq > _distance_mm*_distance_mm) {
              PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone", "Robot not close enough, will return FAILURE_RETRY.");
              result = ActionResult::FAILURE_RETRY;
            }
          }
        } else {
          
          std::vector<Pose3d> possiblePoses; // don't really need these
          bool inPosition = false;
          result = GetPossiblePoses(object, possiblePoses, inPosition);
          
          if(!inPosition) {
            PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone", "Robot not in position, will return FAILURE_RETRY.");
            result = ActionResult::FAILURE_RETRY;
          }
        }
      }
      
      return result;
    }
    
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(Robot& robot,
                                                                     const Pose3d& placementPose,
                                                                     const bool placeOnGround,
                                                                     const PathMotionProfile motionProfile,
                                                                     const bool useExactRotation,
                                                                     const bool useManualSpeed)
    : DriveToObjectAction(robot,
                          robot.GetCarryingObject(),
                          placeOnGround ? PreActionPose::PLACE_ON_GROUND : PreActionPose::PLACE_RELATIVE,
                          motionProfile,
                          0,
                          false,
                          0,
                          useManualSpeed)
    , _placementPose(placementPose)
    , _useExactRotation(useExactRotation)
    {
    }
    
    const std::string& DriveToPlaceCarriedObjectAction::GetName() const
    {
      static const std::string name("DriveToPlaceCarriedObjectAction");
      return name;
    }
    
    ActionResult DriveToPlaceCarriedObjectAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if(_robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d cannot place an object because it is not carrying anything.",
                          _robot.GetID());
        result = ActionResult::FAILURE_ABORT;
      } else {
        
        _objectID = _robot.GetCarryingObject();
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            _robot.GetID(), _objectID.GetValue());
          
          result = ActionResult::FAILURE_ABORT;
        } else {
          
          // Compute the approach angle given the desired placement pose of the carried block
          if (_useExactRotation) {
            f32 approachAngle_rad;
            if (ComputePlacementApproachAngle(_robot, _placementPose, approachAngle_rad) != RESULT_OK) {
              PRINT_NAMED_WARNING("DriveToPlaceCarriedObjectAction.Init.FailedToComputeApproachAngle", "");
              return ActionResult::FAILURE_ABORT;
            }
            SetApproachAngle(approachAngle_rad);
          }
          
          // Temporarily move object to desired pose so we can get placement poses
          // at that position
          const Pose3d origObjectPose(object->GetPose());
          object->SetPose(_placementPose);
          
          // Call parent class's init helper
          result = InitHelper(object);
          
          // Move the object back to where it was (being carried)
          object->SetPose(origObjectPose);
          
        } // if/else object==nullptr
      } // if/else robot is carrying object
      
      return result;
      
    } // DriveToPlaceCarriedObjectAction::Init()
    
    ActionResult DriveToPlaceCarriedObjectAction::CheckIfDone()
    {
      ActionResult result = _compoundAction.Update();
      
      // We completed driving to the pose. Unlike driving to an object for
      // pickup, we can't re-verify the accuracy of our final position, so
      // just proceed.
      
      return result;
    } // DriveToPlaceCarriedObjectAction::CheckIfDone()
    
#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot,
                                         const PathMotionProfile motionProf,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed) //, const Pose3d& pose)
    : IAction(robot)
    , _isGoalSet(false)
    , _driveWithHeadDown(forceHeadDown)
    , _pathMotionProfile(motionProf)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    , _useManualSpeed(useManualSpeed)
    , _maxPlanningTime(DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S)
    , _maxReplanPlanningTime(DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S)
    , _timeToAbortPlanning(-1.0f)
    {

    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot,
                                         const Pose3d& pose,
                                         const PathMotionProfile motionProf,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
    : DriveToPoseAction(robot, motionProf, forceHeadDown, useManualSpeed)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;
      
      SetGoal(pose, distThreshold, angleThreshold);
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot,
                                         const std::vector<Pose3d>& poses,
                                         const PathMotionProfile motionProf,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
    : DriveToPoseAction(robot, motionProf, forceHeadDown, useManualSpeed)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;
      
      SetGoals(poses, distThreshold, angleThreshold);
    }
    
    DriveToPoseAction::~DriveToPoseAction()
    {
      // If we are not running anymore, for any reason, clear the path and its
      // visualization
      _robot.AbortDrivingToPose();
      _robot.GetContext()->GetVizManager()->ErasePath(_robot.GetID());
      _robot.GetContext()->GetVizManager()->EraseAllPlannerObstacles(true);
      _robot.GetContext()->GetVizManager()->EraseAllPlannerObstacles(false);
    }
    
    Result DriveToPoseAction::SetGoal(const Pose3d& pose,
                                      const Point3f& distThreshold,
                                      const Radians& angleThreshold)
    {
      _goalDistanceThreshold = distThreshold;
      _goalAngleThreshold = angleThreshold;
      
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
    
    Result DriveToPoseAction::SetGoals(const std::vector<Pose3d>& poses,
                                       const Point3f& distThreshold,
                                       const Radians& angleThreshold)
    {
      _goalDistanceThreshold = distThreshold;
      _goalAngleThreshold    = angleThreshold;
      
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
    
    ActionResult DriveToPoseAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      _timeToAbortPlanning = -1.0f;
      _nextDrivingSoundTime = 0.f;
      
      if(!_isGoalSet) {
        PRINT_NAMED_ERROR("DriveToPoseAction.Init.NoGoalSet",
                          "Goal must be set before running this action.");
        result = ActionResult::FAILURE_ABORT;
      }
      else {
        
        // Make the poses w.r.t. robot:
        for(auto & pose : _goalPoses) {
          if(pose.GetWithRespectTo(*(_robot.GetWorldOrigin()), pose) == false) {
            PRINT_NAMED_ERROR("DriveToPoseAction.Init",
                              "Could not get goal pose w.r.t. to robot origin.");
            return ActionResult::FAILURE_ABORT;
          }
        }
        
        Result planningResult = RESULT_OK;
        
        _selectedGoalIndex = 0;
        
        if(_goalPoses.size() == 1) {
          planningResult = _robot.StartDrivingToPose(_goalPoses.back(), _pathMotionProfile, _useManualSpeed);
        } else {
          planningResult = _robot.StartDrivingToPose(_goalPoses, _pathMotionProfile, &_selectedGoalIndex, _useManualSpeed);
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
            if(_robot.GetMoveComponent().MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 2.f, 5.f) != RESULT_OK) {
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
          Robot* robotPtr = &_robot;
          auto cbRobotOriginChanged = [this,robotPtr](RobotID_t robotID) {
            if(robotID == robotPtr->GetID()) {
              PRINT_NAMED_INFO("DriveToPoseAction",
                               "Received signal that robot %d's origin changed. Resetting action.",
                               robotID);
              Reset();
              robotPtr->AbortDrivingToPose();
            }
          };
          _originChangedHandle = _robot.OnRobotWorldOriginChanged().ScopedSubscribe(cbRobotOriginChanged);
        }
        
      } // if/else isGoalSet
      
      if(!_drivingSound.empty())
      {
        // If we have a driving sound to play, set up callbacks for when that sound
        // starts/ends so we can choose the time for the next to play between
        // sounds.
        auto soundCompleteLambda = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
        {
          if(_driveSoundActionTag == event.GetData().Get_RobotCompletedAction().idTag)
          {
            // Indicate last drive sound is done
            _driveSoundActionTag = (u32)ActionConstants::INVALID_TAG;
            
            // Choose next play time, relative to current time
            _nextDrivingSoundTime = event.GetCurrentTime() + GetRNG().RandDblInRange(_drivingSoundSpacingMin_sec, _drivingSoundSpacingMax_sec);
          }
        };
        
        _soundCompletedHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotCompletedAction, soundCompleteLambda);
      }
      
      if(ActionResult::SUCCESS == result && !_startSound.empty()) {
        // Play starting sound if there is one (only if nothing else is playing)
        _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, new PlayAnimationAction(_robot, _startSound, 1, false));
      }
      
      return result;
    } // Init()
    
    ActionResult DriveToPoseAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      switch( _robot.CheckDriveToPoseStatus() ) {
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
            _robot.AbortDrivingToPose();
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
            _robot.AbortDrivingToPose();
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
                             "_currPathSegment=%d, _lastSentPathID=%d, _lastRecvdPathID=%d.", ctr,
                             _robot.GetCurrentPathSegment(), _robot.GetLastSentPathID(), _robot.GetLastRecvdPathID());
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
                                          _robot.GetHeight());
          
          if(_robot.GetPose().IsSameAs(_goalPoses[_selectedGoalIndex], distanceThreshold, _goalAngleThreshold, Tdiff))
          {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Success",
                             "Robot %d successfully finished following path (Tdiff=%.1fmm).",
                             _robot.GetID(), Tdiff.Length());
            
            result = ActionResult::SUCCESS;
          }
          // The last path sent was definitely received by the robot
          // and it is no longer executing it, but we appear to not be in position
          else if (_robot.GetLastSentPathID() == _robot.GetLastRecvdPathID()) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.DoneNotInPlace",
                             "Robot is done traversing path, but is not in position (dist=%.1fmm). lastPathID=%d",
                             Tdiff.Length(), _robot.GetLastRecvdPathID());
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
      
      const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      // Play driving or driving sounds if it's time (queue only if nothing else is playing)
      if(ActionResult::RUNNING == result &&                              // we're still runing
         !_drivingSound.empty() &&                                       // we have a drive sound to play
         (u32)ActionConstants::INVALID_TAG == _driveSoundActionTag &&    // we aren't waiting on last drive sound to stop
         currentTime > _nextDrivingSoundTime)                            // it's time to play
      {
        PlayAnimationAction* driveSoundAction = new PlayAnimationAction(_robot, _drivingSound, 1, false);
        _driveSoundActionTag = driveSoundAction->GetTag();
        _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, driveSoundAction);
      }
      else if(ActionResult::SUCCESS == result && !_stopSound.empty())
      {
        _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, new PlayAnimationAction(_robot, _stopSound, 1, false));
      }
      
      return result;
    } // CheckIfDone()
    
#pragma mark ---- IDriveToInteractWithObjectAction ----
    
    IDriveToInteractWithObject::IDriveToInteractWithObject(Robot& robot,
                                                           const ObjectID& objectID,
                                                           const PreActionPose::ActionType& actionType,
                                                           const PathMotionProfile motionProfile,
                                                           const f32 distanceFromMarker_mm,
                                                           const bool useApproachAngle,
                                                           const f32 approachAngle_rad,
                                                           const bool useManualSpeed)
    : CompoundActionSequential(robot)
    {
      _driveToObjectAction = new DriveToObjectAction(robot,
                                                     objectID,
                                                     actionType,
                                                     motionProfile,
                                                     distanceFromMarker_mm,
                                                     useApproachAngle,
                                                     approachAngle_rad,
                                                     useManualSpeed);

      AddAction(_driveToObjectAction);
    }
    
#pragma mark ---- DriveToAlignWithObjectAction ----
    
    DriveToAlignWithObjectAction::DriveToAlignWithObjectAction(Robot& robot,
                                                               const ObjectID& objectID,
                                                               const f32 distanceFromMarker_mm,
                                                               const PathMotionProfile motionProfile,
                                                               const bool useApproachAngle,
                                                               const f32 approachAngle_rad,
                                                               const bool useManualSpeed)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::DOCKING,
                                 motionProfile,
                                 distanceFromMarker_mm,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed)
    {
      AlignWithObjectAction* action = new AlignWithObjectAction(robot, objectID,
                                                                distanceFromMarker_mm, useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToPickupObjectAction ----
    
    DriveToPickupObjectAction::DriveToPickupObjectAction(Robot& robot,
                                                         const ObjectID& objectID,
                                                         const PathMotionProfile motionProfile,
                                                         const bool useApproachAngle,
                                                         const f32 approachAngle_rad,
                                                         const bool useManualSpeed)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::DOCKING,
                                 motionProfile,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed)
    {
      PickupObjectAction* action = new PickupObjectAction(robot, objectID, useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToPlaceOnObjectAction ----
    
    DriveToPlaceOnObjectAction::DriveToPlaceOnObjectAction(Robot& robot,
                                                           const ObjectID& objectID,
                                                           const PathMotionProfile motionProfile,
                                                           const bool useApproachAngle,
                                                           const f32 approachAngle_rad,
                                                           const bool useManualSpeed)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 motionProfile,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed)
    {
      PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                              objectID,
                                                              false,
                                                              0,
                                                              useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToPlaceRelObjectAction ----
    
    DriveToPlaceRelObjectAction::DriveToPlaceRelObjectAction(Robot& robot,
                                                             const ObjectID& objectID,
                                                             const PathMotionProfile motionProfile,
                                                             const f32 placementOffsetX_mm,
                                                             const bool useApproachAngle,
                                                             const f32 approachAngle_rad,
                                                             const bool useManualSpeed)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 motionProfile,
                                 placementOffsetX_mm,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed)
    {
      PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                              objectID,
                                                              true,
                                                              placementOffsetX_mm,
                                                              useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToRollObjectAction ----
    
    DriveToRollObjectAction::DriveToRollObjectAction(Robot& robot,
                                                     const ObjectID& objectID,
                                                     const PathMotionProfile motionProfile,
                                                     const bool useApproachAngle,
                                                     const f32 approachAngle_rad,
                                                     const bool useManualSpeed)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::ROLLING,
                                 motionProfile,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed)
    {
      RollObjectAction* action = new RollObjectAction(robot, objectID, useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToPopAWheelieAction ----
    
    DriveToPopAWheelieAction::DriveToPopAWheelieAction(Robot& robot,
                                                       const ObjectID& objectID,
                                                       const PathMotionProfile motionProfile,
                                                       const bool useApproachAngle,
                                                       const f32 approachAngle_rad,
                                                       const bool useManualSpeed)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::ROLLING,
                                 motionProfile,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed)
    {
      PopAWheelieAction* action = new PopAWheelieAction(robot, objectID, useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToAndTraverseObjectAction ----
    
    DriveToAndTraverseObjectAction::DriveToAndTraverseObjectAction(Robot& robot,
                                                                   const ObjectID& objectID,
                                                                   const PathMotionProfile motionProfile,
                                                                   const bool useManualSpeed)
    : CompoundActionSequential(robot, {
      new DriveToObjectAction(robot,
                              objectID,
                              PreActionPose::ENTRY,
                              motionProfile,
                              0,
                              false,
                              0,
                              useManualSpeed)})
    {
      TraverseObjectAction* action = new TraverseObjectAction(robot, objectID, useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
    
#pragma mark ---- DriveToAndMountChargerAction ----
    
    DriveToAndMountChargerAction::DriveToAndMountChargerAction(Robot& robot,
                                                               const ObjectID& objectID,
                                                               const PathMotionProfile motionProfile,
                                                               const bool useManualSpeed)
    : CompoundActionSequential(robot, {
      new DriveToObjectAction(robot,
                              objectID,
                              PreActionPose::ENTRY,
                              motionProfile,
                              0,
                              false,
                              0,
                              useManualSpeed)})
    {
      MountChargerAction* action = new MountChargerAction(robot, objectID, useManualSpeed);
      action->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2);
      AddAction(action);
    }
  }
}