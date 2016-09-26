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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/speedChooser.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "util/math/math.h"
#include "driveToActions.h"

namespace Anki {
  
  namespace Cozmo {
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(Robot& robot,
                                             const ObjectID& objectID,
                                             const PreActionPose::ActionType& actionType,
                                             const f32 predockOffsetDistX_mm,
                                             const bool useApproachAngle,
                                             const f32 approachAngle_rad,
                                             const bool useManualSpeed)
    : IAction(robot,
              "DriveToObject",
              RobotActionType::DRIVE_TO_OBJECT,
              (useManualSpeed ? 0 : (u8)AnimTrackFlag::BODY_TRACK))
    , _objectID(objectID)
    , _actionType(actionType)
    , _distance_mm(-1.f)
    , _predockOffsetDistX_mm(predockOffsetDistX_mm)
    , _useManualSpeed(useManualSpeed)
    , _compoundAction(robot)
    , _useApproachAngle(useApproachAngle)
    , _approachAngle_rad(approachAngle_rad)
    {
      SetGetPossiblePosesFunc([this](ActionableObject* object,
                                     std::vector<Pose3d>& possiblePoses,
                                     bool& alreadyInPosition)
                              {
                                return GetPossiblePoses(object, possiblePoses, alreadyInPosition);
                              });
    }
    
    DriveToObjectAction::DriveToObjectAction(Robot& robot,
                                             const ObjectID& objectID,
                                             const f32 distance,
                                             const bool useManualSpeed)
    : IAction(robot,
              "DriveToObject",
              RobotActionType::DRIVE_TO_OBJECT,
              (useManualSpeed ? 0 : (u8)AnimTrackFlag::BODY_TRACK))
    , _objectID(objectID)
    , _actionType(PreActionPose::ActionType::NONE)
    , _distance_mm(distance)
    , _predockOffsetDistX_mm(0)
    , _useManualSpeed(useManualSpeed)
    , _compoundAction(robot)
    , _useApproachAngle(false)
    , _approachAngle_rad(0)
    {
      SetGetPossiblePosesFunc([this](ActionableObject* object,
                                     std::vector<Pose3d>& possiblePoses,
                                     bool& alreadyInPosition)
                                     {
                                       return GetPossiblePoses(object, possiblePoses, alreadyInPosition);
                                     });
    }
    
    DriveToObjectAction::~DriveToObjectAction()
    {
      if(_lightsSet)
      {
        _robot.GetLightsComponent().UnSetInteractionObject(_objectID);
      }
      _compoundAction.PrepForCompletion();
    }
    
    void DriveToObjectAction::SetApproachAngle(const f32 angle_rad)
    {
      if( GetState() != ActionResult::FAILURE_NOT_STARTED ) {
        PRINT_NAMED_WARNING("DriveToObjectAction.SetApproachAngle.Invalid",
                            "Tried to set the approach angle, but action has already started");
        return;
      }

      PRINT_NAMED_INFO("DriveToObjectAction.SetApproachingAngle",
                       "[%d] %f rad",
                       GetTag(),
                       angle_rad);
      _useApproachAngle = true;
      _approachAngle_rad = angle_rad;
    }
    
    const bool DriveToObjectAction::GetUseApproachAngle() const
    {
      return _useApproachAngle;
    }
    
    void DriveToObjectAction::SetMotionProfile(const PathMotionProfile& motionProfile)
    {
      _hasMotionProfile = true;
      _pathMotionProfile = motionProfile;
    }
    
    ActionResult DriveToObjectAction::GetPossiblePoses(ActionableObject* object,
                                                       std::vector<Pose3d>& possiblePoses,
                                                       bool& alreadyInPosition)
    {
      const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                               _actionType,
                                                               false,
                                                               _predockOffsetDistX_mm,
                                                               DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE,
                                                               _useApproachAngle,
                                                               _approachAngle_rad.ToFloat());
      IDockAction::PreActionPoseOutput preActionPoseOutput;
    
      IDockAction::GetPreActionPoses(_robot, preActionPoseInput, preActionPoseOutput);
      
      if(preActionPoseOutput.actionResult != ActionResult::SUCCESS)
      {
        _interactionResult = preActionPoseOutput.interactionResult;
        return preActionPoseOutput.actionResult;
      }
      
      if(preActionPoseOutput.preActionPoses.empty())
      {
        PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.NoPreActionPoses",
                            "ActionableObject %d did not return any pre-action poses with action type %d.",
                            _objectID.GetValue(), _actionType);
        _interactionResult = ObjectInteractionResult::NO_PREACTION_POSES;
        return ActionResult::FAILURE_ABORT;
      }
      
      alreadyInPosition = preActionPoseOutput.robotAtClosestPreActionPose;
      possiblePoses.clear();
      
      if(alreadyInPosition)
      {
        Pose3d p = preActionPoseOutput.preActionPoses[preActionPoseOutput.closestIndex].GetPose();
        PRINT_NAMED_INFO("DriveToObjectAction.GetPossiblePoses.UseRobotPose",
                         "Robot's current pose (x:%f y:%f a:%f) is close enough to preAction pose (x:%f y:%f a:%f)"
                         " with threshold %f, using current robot pose as goal",
                         _robot.GetPose().GetTranslation().x(),
                         _robot.GetPose().GetTranslation().y(),
                         _robot.GetPose().GetRotation().GetAngleAroundZaxis().getDegrees(),
                         p.GetTranslation().x(),
                         p.GetTranslation().y(),
                         p.GetRotation().GetAngleAroundZaxis().getDegrees(),
                         preActionPoseOutput.distThresholdUsed);
      }
      
      for(auto preActionPose : preActionPoseOutput.preActionPoses)
      {
        possiblePoses.push_back(preActionPose.GetPose());
      }
      
      _interactionResult = ObjectInteractionResult::SUCCESS;
      return ActionResult::SUCCESS;
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
        
        result = _getPossiblePosesFunc(object, possiblePoses, alreadyInPosition);
      }
      
      // In case we are re-running this action, make sure compound actions are cleared.
      // These will do nothing if compoundAction has nothing in it yet (i.e., on first Init)
      _compoundAction.ClearActions();
      _compoundAction.ShouldSuppressTrackLocking(true);

      if(result == ActionResult::SUCCESS) {
        if(!alreadyInPosition) {

          f32 preActionPoseDistThresh = ComputePreActionPoseDistThreshold(possiblePoses[0], object, DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE);
          
          DriveToPoseAction* driveToPoseAction = new DriveToPoseAction(_robot, true, _useManualSpeed);
          driveToPoseAction->SetGoals(possiblePoses, preActionPoseDistThresh);
          if(_hasMotionProfile)
          {
            driveToPoseAction->SetMotionProfile(_pathMotionProfile);
          }
          _compoundAction.AddAction(driveToPoseAction);
        }
        
        // Make sure we can see the object, unless we are carrying it (i.e. if we
        // are doing a DriveToPlaceCarriedObject action)
        if(!object->IsBeingCarried()) {
          TurnTowardsObjectAction* turnTowardsObjectAction = new TurnTowardsObjectAction(_robot, _objectID, Radians(0), true, false);
          PRINT_NAMED_DEBUG("IActionRunner.CreatedSubAction", "Parent action [%d] %s created a sub action [%d] %s",
                            GetTag(),
                            GetName().c_str(),
                            turnTowardsObjectAction->GetTag(),
                            turnTowardsObjectAction->GetName().c_str());
          _compoundAction.AddAction(turnTowardsObjectAction);
        }
        
        _compoundAction.ShouldEmitCompletionSignal(false);
        
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
      if(object == nullptr)
      {
        PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            _robot.GetID(), _objectID.GetValue());
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        result = ActionResult::FAILURE_ABORT;
      }
      else if(PoseState::Unknown == object->GetPoseState())
      {
        PRINT_NAMED_INFO("DriveToObjectAction.CheckPreconditions.ObjectPoseStateUnknown",
                         "Robot %d cannot plan a path to ActionableObject %d, whose pose state is Unknown.",
                         _robot.GetID(), _objectID.GetValue());
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        result = ActionResult::FAILURE_ABORT;
      }
      else
      {
        // Use a helper here so that it can be shared with DriveToPlaceCarriedObjectAction
        result = InitHelper(object);
      } // if/else object==nullptr
      
      // Mark this object as one we are docking with (e.g. so its lights indicate
      // it is being interacted with)
      // Need to check if we have set the cube lights already in case the action was reset
      if(!_lightsSet)
      {
        _robot.GetLightsComponent().SetInteractionObject(_objectID);
        _lightsSet = true;
      }
      
      return result;
    }
    
    ActionResult DriveToObjectAction::CheckIfDone()
    {
      ActionResult result = _compoundAction.Update();
      
      if(result == ActionResult::SUCCESS) {
        
        if (!_doPositionCheckOnPathCompletion) {
          PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone.SkippingPositionCheck", "Action complete");
          return result;
        }
        
        // We completed driving to the pose and visually verifying the object
        // is still there. This could have updated the object's pose (hopefully
        // to a more accurate one), meaning the pre-action pose we selected at
        // Initialization has now moved and we may not be in position, even if
        // we completed the planned path successfully. If that's the case, we
        // want to retry.
        ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr)
        {
          PRINT_NAMED_WARNING("DriveToObjectAction.CheckIfDone.NoObjectWithID",
                              "Robot %d's block world does not have an ActionableObject with ID=%d.",
                              _robot.GetID(), _objectID.GetValue());
          _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
          result = ActionResult::FAILURE_ABORT;
        }
        else if( _actionType == PreActionPose::ActionType::NONE)
        {
          // Check to see if we got close enough
          Pose3d objectPoseWrtRobotParent;
          if(false == object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), objectPoseWrtRobotParent))
          {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
            result = ActionResult::FAILURE_ABORT;
          }
          else
          {
            const f32 distanceSq = (Point2f(objectPoseWrtRobotParent.GetTranslation()) - Point2f(_robot.GetPose().GetTranslation())).LengthSq();
            if(distanceSq > _distance_mm*_distance_mm) {
              PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone",
                               "[%d] Robot not close enough, will return FAILURE_RETRY.",
                               GetTag());
              result = ActionResult::FAILURE_RETRY;
            }
          }
        }
        else
        {
          std::vector<Pose3d> possiblePoses; // don't really need these
          bool inPosition = false;
          result = GetPossiblePoses(object, possiblePoses, inPosition);
          
          if(!inPosition) {
            PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone",
                             "[%d] Robot not in position, will return FAILURE_RETRY.",
                             GetTag());
            _interactionResult = ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE;
            result = ActionResult::FAILURE_RETRY;
          }
        }
      }
      
      return result;
    }
    
    void DriveToObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted interactionCompleted({{_objectID.GetValue(), -1, -1, -1, -1}}, 1,
                                                      _interactionResult);
      completionUnion.Set_objectInteractionCompleted(interactionCompleted);
    }
    
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(Robot& robot,
                                                                     const Pose3d& placementPose,
                                                                     const bool placeOnGround,
                                                                     const bool useExactRotation,
                                                                     const bool useManualSpeed,
                                                                     const bool checkDestinationFree,
                                                                     const float destinationObjectPadding_mm)
    : DriveToObjectAction(robot,
                          robot.GetCarryingObject(),
                          placeOnGround ? PreActionPose::PLACE_ON_GROUND : PreActionPose::PLACE_RELATIVE,
                          0,
                          false,
                          0,
                          useManualSpeed)
    , _placementPose(placementPose)
    , _useExactRotation(useExactRotation)
    , _checkDestinationFree(checkDestinationFree)
    , _destinationObjectPadding_mm(destinationObjectPadding_mm)
    {
      SetName("DriveToPlaceCarriedObject");
      SetType(RobotActionType::DRIVE_TO_PLACE_CARRIED_OBJECT);
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
            if (IDockAction::ComputePlacementApproachAngle(_robot, _placementPose, approachAngle_rad) != RESULT_OK) {
              PRINT_NAMED_WARNING("DriveToPlaceCarriedObjectAction.Init.FailedToComputeApproachAngle", "");
              return ActionResult::FAILURE_ABORT;
            }
            SetApproachAngle(approachAngle_rad);
          }
          
          // Create a temporary object of the same type at the desired pose so we
          // can get placement poses at that position
          ActionableObject* tempObject = dynamic_cast<ActionableObject*>(object->CloneType());
          ASSERT_NAMED(tempObject != nullptr, "DriveToPlaceCarriedObjectAction.Init.DynamicCastFail");
          
          tempObject->InitPose(_placementPose, PoseState::Unknown);
          tempObject->SetBeingCarried(true);
          
          // Call parent class's init helper
          result = DriveToObjectAction::InitHelper(tempObject);
          
          Util::SafeDelete(tempObject);
          
        } // if/else object==nullptr
      } // if/else robot is carrying object
      
      return result;
      
    } // DriveToPlaceCarriedObjectAction::Init()
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ActionResult DriveToPlaceCarriedObjectAction::CheckIfDone()
    {
      ActionResult result = _compoundAction.Update();
      
      // check if the destination is free
      if ( _checkDestinationFree )
      {
        const bool isFree = IsPlacementGoalFree();
        if ( !isFree ) {
          PRINT_NAMED_INFO("DriveToPlaceCarriedObjectAction.PlacementGoalNotFree",
                           "Placement goal is not free to drop the cube, failing with retry.");
          result = ActionResult::FAILURE_RETRY;
        }
      }
      
      // We completed driving to the pose. Unlike driving to an object for
      // pickup, we can't re-verify the accuracy of our final position, so
      // just proceed.
      
      return result;
    } // DriveToPlaceCarriedObjectAction::CheckIfDone()
    
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool DriveToPlaceCarriedObjectAction::IsPlacementGoalFree() const
    {
      ObservableObject* object = _robot.GetBlockWorld().GetObjectByID( _robot.GetCarryingObject() );
      if ( nullptr != object )
      {
        BlockWorldFilter ignoreSelfFilter;
        ignoreSelfFilter.AddIgnoreID( object->GetID() );
      
        // calculate quad at candidate destination
        Quad2f candidateQuad = object->GetBoundingQuadXY(_placementPose);
        
        // TODO rsam: this only checks for other cubes, but not for unknown obstacles since we don't have collision sensor
        std::vector<ObservableObject *> intersectingObjects;
        _robot.GetBlockWorld().FindIntersectingObjects(candidateQuad, intersectingObjects, _destinationObjectPadding_mm, ignoreSelfFilter);
        bool isFree = intersectingObjects.empty();
        return isFree;
      }
      
      // no object :(
      return true;
    }
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed) //, const Pose3d& pose)
    : IAction(robot,
              "DriveToPose",
              RobotActionType::DRIVE_TO_POSE,
              (useManualSpeed ? 0 : (u8)AnimTrackFlag::BODY_TRACK))
    , _isGoalSet(false)
    , _driveWithHeadDown(forceHeadDown)
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
                                         const bool forceHeadDown,
                                         const bool useManualSpeed,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
    : DriveToPoseAction(robot, forceHeadDown, useManualSpeed)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;
      
      SetGoal(pose, distThreshold, angleThreshold);
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot,
                                         const std::vector<Pose3d>& poses,
                                         const bool forceHeadDown,
                                         const bool useManualSpeed,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
    : DriveToPoseAction(robot, forceHeadDown, useManualSpeed)
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
      
      _robot.GetDrivingAnimationHandler().ActionIsBeingDestroyed();
    }
    
    Result DriveToPoseAction::SetGoal(const Pose3d& pose,
                                      const Point3f& distThreshold,
                                      const Radians& angleThreshold)
    {

      if( GetState() != ActionResult::FAILURE_NOT_STARTED ) {
        PRINT_NAMED_WARNING("DriveToObjectAction.SetGoal.Invalid",
                            "[%d] Tried to set goal, but action has started",
                            GetTag());
        return RESULT_FAIL;
      }

      _goalDistanceThreshold = distThreshold;
      _goalAngleThreshold = angleThreshold;
      
      _goalPoses = {pose};
      
      PRINT_NAMED_INFO("DriveToPoseAction.SetGoal",
                       "[%d] Setting pose goal to (%.1f,%.1f,%.1f) @ %.1fdeg",
                       GetTag(),
                       _goalPoses.back().GetTranslation().x(),
                       _goalPoses.back().GetTranslation().y(),
                       _goalPoses.back().GetTranslation().z(),
                       _goalPoses.back().GetRotationAngle<'Z'>().getDegrees());
      
      _isGoalSet = true;
      
      return RESULT_OK;
    }
    
    Result DriveToPoseAction::SetGoals(const std::vector<Pose3d>& poses,
                                       const Point3f& distThreshold,
                                       const Radians& angleThreshold)
    {

      if( GetState() != ActionResult::FAILURE_NOT_STARTED ) {
        PRINT_NAMED_WARNING("DriveToObjectAction.SetGoals.Invalid",
                            "[%d] Tried to set goals, but action has started",
                            GetTag());
        return RESULT_FAIL;
      }
      
      _goalDistanceThreshold = distThreshold;
      _goalAngleThreshold    = angleThreshold;
      
      _goalPoses = poses;
      
      PRINT_NAMED_INFO("DriveToPoseAction.SetGoal",
                       "[%d] Setting %lu possible goal options.",
                       GetTag(),
                       (unsigned long)_goalPoses.size());
      
      _isGoalSet = true;
      
      return RESULT_OK;
    }
    
    void DriveToPoseAction::SetMotionProfile(const PathMotionProfile& motionProfile)
    {
      _hasMotionProfile = true;
      _pathMotionProfile = motionProfile;
    }
    
    ActionResult DriveToPoseAction::Init()
    {
      _robot.GetDrivingAnimationHandler().Init(GetTracksToLock(), GetTag(), IsSuppressingTrackLocking());
    
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
          if(pose.GetWithRespectTo(*(_robot.GetWorldOrigin()), pose) == false) {
            PRINT_NAMED_ERROR("DriveToPoseAction.Init",
                              "Could not get goal pose w.r.t. to robot origin.");
            return ActionResult::FAILURE_ABORT;
          }
        }
        
        Result planningResult = RESULT_OK;
        
        _selectedGoalIndex = 0;
        
        if(!_hasMotionProfile)
        {
          _pathMotionProfile = GetRobot().GetSpeedChooser().GetPathMotionProfile(_goalPoses);
        }
        
        if(_goalPoses.size() == 1) {
          planningResult = _robot.StartDrivingToPose(_goalPoses.back(), _pathMotionProfile, _useManualSpeed);
        } else {
          planningResult = _robot.StartDrivingToPose(_goalPoses,
                                                     _pathMotionProfile,
                                                     &_selectedGoalIndex,
                                                     _useManualSpeed);
          PRINT_NAMED_DEBUG("DriveToPoseAction.SelectedGoal",
                            "[%d] Selected goal %d W.R.T. robot (%f, %f, %f, %fdeg)",
                            GetTag(),
                            (int) _selectedGoalIndex,
                            _goalPoses[_selectedGoalIndex].GetTranslation().x(),
                            _goalPoses[_selectedGoalIndex].GetTranslation().y(),
                            _goalPoses[_selectedGoalIndex].GetTranslation().z(),
                            _goalPoses[_selectedGoalIndex].GetRotationAngle<'Z'>().getDegrees());
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
      
      return result;
    } // Init()
    
    ActionResult DriveToPoseAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      // Still running while the drivingEnd animation is playing
      if(_robot.GetDrivingAnimationHandler().IsPlayingEndAnim())
      {
        return ActionResult::RUNNING;
      }
      
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
        
          // If we are following a path start playing driving animations
          // Won't do anything if DrivingAnimationHandler has already been inited
          _robot.GetDrivingAnimationHandler().PlayStartAnim();
        
          // clear abort timing, since we got a path
          _timeToAbortPlanning = -1.0f;
          
          static int ctr = 0;
          if(ctr++ % 10 == 0) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.WaitingForPathCompletion",
                             "[%d] Waiting for robot to complete its path traversal (%d), "
                             "_currPathSegment=%d, _lastSentPathID=%d, _lastRecvdPathID=%d.",
                             GetTag(),
                             ctr,
                             _robot.GetCurrentPathSegment(),
                             _robot.GetLastSentPathID(),
                             _robot.GetLastRecvdPathID());
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
                             "[%d] Robot %d successfully finished following path (Tdiff=%.1fmm).",
                             GetTag(),
                             _robot.GetID(), Tdiff.Length());
            
            result = ActionResult::SUCCESS;
          }
          // The last path sent was definitely received by the robot
          // and it is no longer executing it, but we appear to not be in position
          else if (_robot.GetLastSentPathID() == _robot.GetLastRecvdPathID()) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.DoneNotInPlace",
                             "[%d] Robot is done traversing path, but is not in position (dist=%.1fmm). lastPathID=%d"
                             " goal %d (%f, %f, %f, %fdeg), actual (%f, %f, %f, %fdeg)",
                             GetTag(),
                             Tdiff.Length(), _robot.GetLastRecvdPathID(),
                             (int) _selectedGoalIndex,
                             _goalPoses[_selectedGoalIndex].GetTranslation().x(),
                             _goalPoses[_selectedGoalIndex].GetTranslation().y(),
                             _goalPoses[_selectedGoalIndex].GetTranslation().z(),
                             _goalPoses[_selectedGoalIndex].GetRotationAngle<'Z'>().getDegrees(),
                             _robot.GetPose().GetTranslation().x(),
                             _robot.GetPose().GetTranslation().y(),
                             _robot.GetPose().GetTranslation().z(),
                             _robot.GetPose().GetRotationAngle<'Z'>().getDegrees());
            
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
      
      // If we are no longer running so start drivingEnd animation and keep this action running
      if(ActionResult::RUNNING != result &&
         _robot.GetDrivingAnimationHandler().PlayEndAnim())
      {
        result = ActionResult::RUNNING;
      }

      return result;
    } // CheckIfDone()
    
#pragma mark ---- IDriveToInteractWithObjectAction ----
    
    IDriveToInteractWithObject::IDriveToInteractWithObject(Robot& robot,
                                                           const ObjectID& objectID,
                                                           const PreActionPose::ActionType& actionType,
                                                           const f32 predockOffsetDistX_mm,
                                                           const bool useApproachAngle,
                                                           const f32 approachAngle_rad,
                                                           const bool useManualSpeed,
                                                           Radians maxTurnTowardsFaceAngle_rad,
                                                           const bool sayName)
    : CompoundActionSequential(robot)
    , _objectID(objectID)
    , _preDockPoseDistOffsetX_mm(predockOffsetDistX_mm)
    {
      if(objectID == robot.GetCarryingObject())
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.Constructor",
                            "Robot is currently carrying action object with ID=%d",
                            objectID.GetValue());
        return;
      }
    
      _driveToObjectAction = new DriveToObjectAction(robot,
                                                     objectID,
                                                     actionType,
                                                     predockOffsetDistX_mm,
                                                     useApproachAngle,
                                                     approachAngle_rad,
                                                     useManualSpeed);

      // TODO: Use the function-based ShouldIgnoreFailure option for AddAction to catch some failures of DriveToObject earlier
      //  (Started to do this but it started to feel messy/dangerous right before ship)
      /*
      // Ignore DriveTo failures iff we simply did not reach preaction pose or
      // failed visual verification (since we'll presumably recheck those at start
      // of object interaction action)
      ICompoundAction::ShouldIgnoreFailureFcn shouldIgnoreFailure = [](ActionResult result, const IActionRunner* action)
      {
        if(nullptr == action)
        {
          PRINT_NAMED_ERROR("IDriveToInteractWithObject.Constructor.NullAction",
                            "ShouldIgnoreFailureFcn cannot check null action");
          return false;
        }
        
        if(ActionResult::SUCCESS == result)
        {
          PRINT_NAMED_WARNING("IDriveToInteractWithObject.Constructor.CheckingIgnoreForSuccessResult",
                              "Not expecting ShouldIgnoreFailure to be called for successful action result");
          return false;
        }
       
        // Note that DriveToObjectActions return ObjectInteractionCompleted unions
        ActionCompletedUnion completionUnion;
        action->GetCompletionUnion(completionUnion);
        const ObjectInteractionCompleted& objInteractionCompleted = completionUnion.Get_objectInteractionCompleted();
        
        switch(objInteractionCompleted.result)
        {
          case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
          case ObjectInteractionResult::VISUAL_VERIFICATION_FAILED:
            return true;
            
          default:
            return false;
        }
      };
      
      AddAction(_driveToObjectAction, shouldIgnoreFailure);
      */
        
      AddAction(_driveToObjectAction, true);
        
      if(maxTurnTowardsFaceAngle_rad > 0.f)
      {
        _turnTowardsLastFacePoseAction = new TurnTowardsLastFacePoseAction(robot, maxTurnTowardsFaceAngle_rad, sayName);
        _turnTowardsObjectAction = new TurnTowardsObjectAction(robot, objectID, maxTurnTowardsFaceAngle_rad);
      
        AddAction(_turnTowardsLastFacePoseAction, true);
        AddAction(_turnTowardsObjectAction, true);
      }
    }
    
    IDriveToInteractWithObject::IDriveToInteractWithObject(Robot& robot,
                                const ObjectID& objectID,
                                const f32 distance,
                                const bool useManualSpeed)
    : CompoundActionSequential(robot)
    , _objectID(objectID)
    {
      if(objectID == robot.GetCarryingObject())
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.Constructor",
                            "Robot is currently carrying action object with ID=%d",
                            objectID.GetValue());
        return;
      }
      
      _driveToObjectAction = new DriveToObjectAction(robot,
                                                     objectID,
                                                     distance,
                                                     useManualSpeed);
      
      AddAction(_driveToObjectAction, true);
    }
    
    void IDriveToInteractWithObject::AddDockAction(IDockAction* dockAction, bool ignoreFailure)
    {
      dockAction->SetPreDockPoseDistOffset(_preDockPoseDistOffsetX_mm);
      AddAction(dockAction, ignoreFailure);
    }

    void IDriveToInteractWithObject::SetSayNameAnimationTrigger(AnimationTrigger trigger)
    {
      if( HasStarted() ) {
        PRINT_NAMED_ERROR("IDriveToInteractWithObject.SetSayNameAnimationTrigger.AfterRunning",
                          "Tried to update the animations after the action started, this isn't supported");
        return;
      }
      if( nullptr != _turnTowardsLastFacePoseAction ) {
        _turnTowardsLastFacePoseAction->SetSayNameAnimationTrigger(trigger);
      }
    }
      
    void IDriveToInteractWithObject::SetNoNameAnimationTrigger(AnimationTrigger trigger)
    {
      if( HasStarted() ) {
        PRINT_NAMED_ERROR("IDriveToInteractWithObject.SetNoNameAnimationTrigger.AfterRunning",
                          "Tried to update the animations after the action started, this isn't supported");
        return;
      }
      if( nullptr != _turnTowardsLastFacePoseAction ) {
        _turnTowardsLastFacePoseAction->SetNoNameAnimationTrigger(trigger);
      }
    }
    
    void IDriveToInteractWithObject::SetMaxTurnTowardsFaceAngle(const Radians angle)
    {
      if(_turnTowardsObjectAction == nullptr ||
         _turnTowardsLastFacePoseAction == nullptr)
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetMaxTurnTowardsFaceAngle",
                            "Can not set angle of null actions (the action were originally constructed with an angle of zero)");
        return;
      }
      PRINT_NAMED_DEBUG("IDriveToInteractWithObject.SetMaxTurnTowardsFaceAngle",
                        "Setting maxTurnTowardsFaceAngle to %f degrees", angle.getDegrees());
      _turnTowardsLastFacePoseAction->SetMaxTurnAngle(angle);
      _turnTowardsObjectAction->SetMaxTurnAngle(angle);
    }
    
    void IDriveToInteractWithObject::SetTiltTolerance(const Radians tol)
    {
      if(_turnTowardsObjectAction == nullptr ||
         _turnTowardsLastFacePoseAction == nullptr)
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetTiltTolerance",
                            "Can not set angle of null actions (the action were originally constructed with an angle of zero)");
        return;
      }
      PRINT_NAMED_DEBUG("IDriveToInteractWithObject.SetTiltTolerance",
                        "Setting tilt tolerance to %f degrees", tol.getDegrees());
      _turnTowardsLastFacePoseAction->SetTiltTolerance(tol);
      _turnTowardsObjectAction->SetTiltTolerance(tol);
    }

    void IDriveToInteractWithObject::SetApproachAngle(const f32 angle_rad)
    {
      if( GetState() != ActionResult::FAILURE_NOT_STARTED ) {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetApproachAngle.Invalid",
                            "Tried to set the approach angle, but action has already started");
        return;
      }

      if(nullptr != _driveToObjectAction) {
        PRINT_CH_INFO("Actions", "IDriveToInteractWithObject.SetApproachingAngle",
                      "[%d] %f rad",
                      GetTag(),
                      angle_rad);
        
        _driveToObjectAction->SetApproachAngle(angle_rad);
      } else {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetApproachAngle.NullDriveToAction", "");
      }
    }
    
    const bool IDriveToInteractWithObject::GetUseApproachAngle() const
    {
      if(nullptr != _driveToObjectAction)
      {
        return _driveToObjectAction->GetUseApproachAngle();
      }
      return false;
    }
    
    void IDriveToInteractWithObject::SetMotionProfile(const PathMotionProfile& motionProfile)
    {
      if(nullptr != _driveToObjectAction) {
        _driveToObjectAction->SetMotionProfile(motionProfile);
      } else {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetMotionProfile.NullDriveToAction", "");
      }
      
      // If any of our children are dockActions (which they likely are) we need to update their
      // speeds/accels to use the ones specified in the motionProfile
      for(auto& action : GetActionList())
      {
        IDockAction* dockAction;
        if((dockAction = dynamic_cast<IDockAction*>(action)) != nullptr)
        {
          dockAction->SetSpeedAndAccel(motionProfile.dockSpeed_mmps, motionProfile.dockAccel_mmps2, motionProfile.dockDecel_mmps2);
        }
      }
    }

    Result IDriveToInteractWithObject::UpdateDerived()
    {
      if(!_lightsSet) {
        _robot.GetLightsComponent().SetInteractionObject(_objectID);
        _lightsSet = true;
      }
      return RESULT_OK;
    }
    
    IDriveToInteractWithObject::~IDriveToInteractWithObject()
    {
      if(_lightsSet) {
        _robot.GetLightsComponent().UnSetInteractionObject(_objectID);
        _lightsSet = false;
      }
    }
    
#pragma mark ---- DriveToAlignWithObjectAction ----
    
    DriveToAlignWithObjectAction::DriveToAlignWithObjectAction(Robot& robot,
                                                               const ObjectID& objectID,
                                                               const f32 distanceFromMarker_mm,
                                                               const bool useApproachAngle,
                                                               const f32 approachAngle_rad,
                                                               const AlignmentType alignmentType,
                                                               const bool useManualSpeed,
                                                               Radians maxTurnTowardsFaceAngle_rad,
                                                               const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      AlignWithObjectAction* action = new AlignWithObjectAction(robot,
                                               objectID,
                                               distanceFromMarker_mm,
                                               alignmentType,
                                               useManualSpeed);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
#pragma mark ---- DriveToPickupObjectAction ----
    
    DriveToPickupObjectAction::DriveToPickupObjectAction(Robot& robot,
                                                         const ObjectID& objectID,
                                                         const bool useApproachAngle,
                                                         const f32 approachAngle_rad,
                                                         const bool useManualSpeed,
                                                         Radians maxTurnTowardsFaceAngle_rad,
                                                         const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::DOCKING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      _pickupAction = new PickupObjectAction(robot, objectID, useManualSpeed);
      AddDockAction(_pickupAction);
      SetProxyTag(_pickupAction->GetTag());
    }
    
    void DriveToPickupObjectAction::SetDockingMethod(DockingMethod dockingMethod)
    {
      if(nullptr != _pickupAction) {
        _pickupAction->SetDockingMethod(dockingMethod);
      } else {
        PRINT_NAMED_WARNING("DriveToPickupObjectAction.SetDockingMethod.NullPickupAction", "");
      }
    }
    
    void DriveToPickupObjectAction::SetPostDockLiftMovingAnimation(Anki::Cozmo::AnimationTrigger trigger)
    {
      if(nullptr != _pickupAction) {
        _pickupAction->SetPostDockLiftMovingAnimation(trigger);
      } else {
        PRINT_NAMED_WARNING("DriveToPickupObjectAction.SetPostDockLiftMovingAnimation.NullPickupAction", "");
      }
    }
    

    
#pragma mark ---- DriveToPlaceOnObjectAction ----
    
    DriveToPlaceOnObjectAction::DriveToPlaceOnObjectAction(Robot& robot,
                                                           const ObjectID& objectID,
                                                           const bool useApproachAngle,
                                                           const f32 approachAngle_rad,
                                                           const bool useManualSpeed,
                                                           Radians maxTurnTowardsFaceAngle_rad,
                                                           const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                              objectID,
                                                              false,
                                                              0,
                                                              0,
                                                              useManualSpeed);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
#pragma mark ---- DriveToPlaceRelObjectAction ----
    
    DriveToPlaceRelObjectAction::DriveToPlaceRelObjectAction(Robot& robot,
                                                             const ObjectID& objectID,
                                                             const bool placingOnGround,
                                                             const f32 placementOffsetX_mm,
                                                             const f32 placementOffsetY_mm,
                                                             const bool useApproachAngle,
                                                             const f32 approachAngle_rad,
                                                             const bool useManualSpeed,
                                                             Radians maxTurnTowardsFaceAngle_rad,
                                                             const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                              objectID,
                                                              placingOnGround,
                                                              placementOffsetX_mm,
                                                              placementOffsetY_mm,
                                                              useManualSpeed);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
#pragma mark ---- DriveToRollObjectAction ----
    
    DriveToRollObjectAction::DriveToRollObjectAction(Robot& robot,
                                                     const ObjectID& objectID,
                                                     const bool useApproachAngle,
                                                     const f32 approachAngle_rad,
                                                     const bool useManualSpeed,
                                                     Radians maxTurnTowardsFaceAngle_rad,
                                                     const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::ROLLING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    , _objectID(objectID)
    {
      _rollAction = new RollObjectAction(robot, objectID, useManualSpeed);
      AddDockAction(_rollAction);
      SetProxyTag(_rollAction->GetTag());
    }

    void DriveToRollObjectAction::RollToUpright()
    {
      if( GetState() != ActionResult::FAILURE_NOT_STARTED ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.RollToUpright.AlreadyRunning",
                            "[%d] Tried to set the approach angle, but action has already started",
                            GetTag());
        return;
      }

      if( ! _objectID.IsSet() ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.RollToUpright.NoObject",
                            "[%d] invalid object id",
                            GetTag());
        return;
      }

      const BlockWorld& blockWorld = _robot.GetBlockWorld();
      std::vector<std::pair<Quad2f, ObjectID> > obstacles;
      blockWorld.GetObstacles(obstacles);

      // Compute approach angle so that rolling rights the block, using docking
      ObservableObject* observableObject = _robot.GetBlockWorld().GetObjectByID(_objectID);
      if( nullptr == observableObject ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.RollToUpright.NullObject",
                            "[%d] invalid object id %d",
                            GetTag(),
                            _objectID.GetValue());
        return;
      }

      if( observableObject->GetFamily() != ObjectFamily::LightCube &&
          observableObject->GetFamily() != ObjectFamily::Block ) {
        PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WrongFamily",
                      "[%d] Can only use this function on blocks or light cubes, ignoring call",
                      GetTag());
        return;
      }

      // unfortunately this needs to be a dynamic cast because Block inherits from observable object virtually
      Block* block = dynamic_cast<Block*>(observableObject);
      if( block == nullptr ) {
        PRINT_NAMED_ERROR("DriveToRollObjectAction.RollToUpright.NotABlock",
                          "[%d] object %d exists, but can't be cast to a Block. This is a bug",
                          GetTag(),
                          _objectID.GetValue());
        return;
      }

      
      std::vector<PreActionPose> preActionPoses;
      block->GetCurrentPreActionPoses(preActionPoses,
                                      {PreActionPose::ROLLING},
                                      std::set<Vision::Marker::Code>(),
                                      obstacles);

      if( preActionPoses.empty() ) {
        PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WillNotUpright.NoPoses",
                      "[%d] No valid pre-dock poses to roll object %d, not restricting pose",
                      GetTag(),
                      _objectID.GetValue());
        return;
      }

      // if we have any valid predock poses which approach the bottom face, use those

      const Vision::KnownMarker& bottomMarker = block->GetMarker( Block::FaceName::BOTTOM_FACE );

      for( const auto& preActionPose : preActionPoses ) {
        const Vision::KnownMarker* marker = preActionPose.GetMarker();
        if( nullptr != marker && marker->GetCode() == bottomMarker.GetCode() ) {
          // found at least one valid pre-action pose using the bottom marker, so limit the approach angle so
          // we will roll the block to upright
          Vec3f approachVec = ComputeVectorBetween(block->GetPose(), preActionPose.GetPose());
          f32 approachAngle_rad = atan2f(approachVec.y(), approachVec.x());
          SetApproachAngle(approachAngle_rad);
          PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WillUpright",
                        "[%d] Found a predock pose that should upright cube %d",
                        GetTag(),
                        _objectID.GetValue());
          return;
        }
      }

      // if we got here, that means none of the predock poses (if there are any) will roll from the bottom. In
      // this case, don't limit the predock poses at all. This will make it so we *might* get lucky and roll
      // the cube into a state where we can roll it again to upright it, although there is no guarantee. A
      // real solution would need a high-level planner to solve this. By doing nothing here, we don't limit
      // the approach angle at all
      PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WillNotUpright.NoBottomPose",
                    "[%d] none of the %zu actions will upright the cube, allowing any",
                    GetTag(),
                    preActionPoses.size());
    }
    
    Result DriveToRollObjectAction::EnableDeepRoll(bool enable)
    {
      if( GetState() != ActionResult::FAILURE_NOT_STARTED ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.EnableDeepRoll.Invalid",
                            "[%d] Tried to set deep roll mode, but action has started",
                            GetTag());
        return RESULT_FAIL;
      }
      
      ASSERT_NAMED(_rollAction != nullptr, "DriveToRollObjectAction.actionIsNull");
      _rollAction->EnableDeepRoll(enable);
      return RESULT_OK;
    }
    
#pragma mark ---- DriveToPopAWheelieAction ----
    
    DriveToPopAWheelieAction::DriveToPopAWheelieAction(Robot& robot,
                                                       const ObjectID& objectID,
                                                       const bool useApproachAngle,
                                                       const f32 approachAngle_rad,
                                                       const bool useManualSpeed,
                                                       Radians maxTurnTowardsFaceAngle_rad,
                                                       const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::ROLLING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      PopAWheelieAction* action = new PopAWheelieAction(robot, objectID, useManualSpeed);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
#pragma mark ---- DriveToAndTraverseObjectAction ----
    
    DriveToAndTraverseObjectAction::DriveToAndTraverseObjectAction(Robot& robot,
                                                                   const ObjectID& objectID,
                                                                   const bool useManualSpeed,
                                                                   Radians maxTurnTowardsFaceAngle_rad,
                                                                   const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::ENTRY,
                                 0,
                                 false,
                                 0,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      TraverseObjectAction* action = new TraverseObjectAction(robot, objectID, useManualSpeed);
      SetProxyTag(action->GetTag());
      AddAction(action);
    }
    
#pragma mark ---- DriveToAndMountChargerAction ----
    
    DriveToAndMountChargerAction::DriveToAndMountChargerAction(Robot& robot,
                                                               const ObjectID& objectID,
                                                               const bool useManualSpeed,
                                                               Radians maxTurnTowardsFaceAngle_rad,
                                                               const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::ENTRY,
                                 0,
                                 false,
                                 0,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      // Get DriveToObjectAction
      DriveToObjectAction* driveAction = GetDriveToObjectAction();
      ASSERT_NAMED(driveAction != nullptr, "DriveToAndMountChargerAction.DriveToObjectSubActionNotFound");
      driveAction->DoPositionCheckOnPathCompletion(false);
      
      MountChargerAction* action = new MountChargerAction(robot, objectID, useManualSpeed);
      SetProxyTag(action->GetTag());
      AddDockAction(action);
    }
    
    DriveToRealignWithObjectAction::DriveToRealignWithObjectAction(Robot& robot,
                                    ObjectID objectID,
                                    float dist_mm)
    : CompoundActionSequential(robot)
    {
      const f32 minTrans = 20.0f;
      const f32 moveBackDist = 35.0f;
      const f32 waitTime = 3.0f;
      

      ObservableObject* observableObject = robot.GetBlockWorld().GetObjectByID(objectID);
      if(nullptr == observableObject)
      {
        PRINT_NAMED_WARNING("DriveToRealignWithObjectAction.Constructor.NullObservableObject",
                            "ObjectID=%d. Will not use add MoveHead+DriveStraight+Wait actions.",
                            objectID.GetValue());
      }
      else
      {
        // if block's state is not known, find it.
        Pose3d p;
        observableObject->GetPose().GetWithRespectTo(robot.GetPose(), p);
        if(!(observableObject->IsPoseStateKnown()) || (p.GetTranslation().y() < minTrans))
        {
          MoveHeadToAngleAction* moveHeadToAngleAction = new MoveHeadToAngleAction(robot, kIdealViewBlockHeadAngle);
          AddAction(moveHeadToAngleAction);
          DriveStraightAction* driveAction = new DriveStraightAction(robot, -moveBackDist, DEFAULT_PATH_MOTION_PROFILE.reverseSpeed_mmps, false);
          AddAction(driveAction);
          WaitAction* waitAction = new WaitAction(robot, waitTime);
          AddAction(waitAction);
        }
      }
      
      // Drive towards found block and verify it.
      DriveToAlignWithObjectAction* driveToAlignWithObjectAction = new DriveToAlignWithObjectAction(robot, objectID, dist_mm);
      driveToAlignWithObjectAction->SetNumRetries(0);
      AddAction(driveToAlignWithObjectAction);
      SetNumRetries(0);
    }
  }
}


