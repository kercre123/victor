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

#include "engine/actions/driveToActions.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"
#include "engine/speedChooser.h"
#include "clad/types/animationTypes.h"
#include "util/console/consoleInterface.h"
#include "util/math/math.h"

namespace Anki {
  
  namespace Cozmo {
    class BlockWorld;

    namespace {
      CONSOLE_VAR(bool, kEnablePredockDistanceCheckFix, "DriveToActions", true);
      CONSOLE_VAR(f32, kDriveToPoseTimeout, "DriveToActions", 30.f);
    }
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(const ObjectID& objectID,
                                             const PreActionPose::ActionType& actionType,
                                             const f32 predockOffsetDistX_mm,
                                             const bool useApproachAngle,
                                             const f32 approachAngle_rad)
    : IAction("DriveToObject",
              RobotActionType::DRIVE_TO_OBJECT,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _objectID(objectID)
    , _actionType(actionType)
    , _distance_mm(-1.f)
    , _predockOffsetDistX_mm(predockOffsetDistX_mm)
    , _compoundAction()
    , _useApproachAngle(useApproachAngle)
    , _approachAngle_rad(approachAngle_rad)
    , _preActionPoseAngleTolerance_rad(DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE)
    {
      SetGetPossiblePosesFunc([this](ActionableObject* object,
                                     std::vector<Pose3d>& possiblePoses,
                                     bool& alreadyInPosition)
                              {
                                return GetPossiblePoses(object, possiblePoses, alreadyInPosition);
                              });
    }
    
    DriveToObjectAction::DriveToObjectAction(const ObjectID& objectID,
                                             const f32 distance)
    : IAction("DriveToObject",
              RobotActionType::DRIVE_TO_OBJECT,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _objectID(objectID)
    , _actionType(PreActionPose::ActionType::NONE)
    , _distance_mm(distance)
    , _predockOffsetDistX_mm(0)
    , _compoundAction()
    , _useApproachAngle(false)
    , _approachAngle_rad(0)
    , _preActionPoseAngleTolerance_rad(DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE)
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
      if(HasRobot() && _lightsSet)
      {
        PRINT_CH_INFO("Actions", "DriveToObjectAction.UnsetInteracting", "%s[%d] Unsetting interacting object to %d",
                      GetName().c_str(), GetTag(),
                      _objectID.GetValue());
        GetRobot().GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::DrivingTo, _objectID);
      }
      _compoundAction.PrepForCompletion();
    }
    
    void DriveToObjectAction::OnRobotSet()
    {
      _compoundAction.SetRobot(&GetRobot());
      OnRobotSetInternalDriveToObj();
    }
    
    void DriveToObjectAction::SetApproachAngle(const f32 angle_rad)
    {
      if( GetState() != ActionResult::NOT_STARTED ) {
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
    
    const bool DriveToObjectAction::GetClosestPreDockPose(ActionableObject* object, Pose3d& closestPose) const
    {
      const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                               _actionType,
                                                               false,
                                                               _predockOffsetDistX_mm,
                                                               _preActionPoseAngleTolerance_rad,
                                                               _useApproachAngle,
                                                               _approachAngle_rad.ToFloat());
      IDockAction::PreActionPoseOutput preActionPoseOutput;
      
      IDockAction::GetPreActionPoses(GetRobot().GetPose(), GetRobot().GetCarryingComponent(), GetRobot().GetBlockWorld(),
                                     preActionPoseInput, preActionPoseOutput);
      
      if(preActionPoseOutput.actionResult == ActionResult::SUCCESS){
        if(preActionPoseOutput.preActionPoses.size() > 0){
          const bool closestIndexValid = preActionPoseOutput.closestIndex < preActionPoseOutput.preActionPoses.size();
          DEV_ASSERT_MSG(closestIndexValid,
                         "DriveToObjectAction.GetClosestPreDockPose.ClosestIndexOutOfRange",
                         "Attempted to access closest index %zu when preactionPoses has a size %zu",
                         preActionPoseOutput.closestIndex, preActionPoseOutput.preActionPoses.size());
          // ensure we don't crash in release
          if (closestIndexValid){
            closestPose = preActionPoseOutput.preActionPoses[preActionPoseOutput.closestIndex].GetPose();
          }
          return closestIndexValid;
        }
      }
      
      return false;
    }
        
    ActionResult DriveToObjectAction::GetPossiblePoses(ActionableObject* object,
                                                       std::vector<Pose3d>& possiblePoses,
                                                       bool& alreadyInPosition)
    {
      const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                               _actionType,
                                                               false,
                                                               _predockOffsetDistX_mm,
                                                               _preActionPoseAngleTolerance_rad,
                                                               _useApproachAngle,
                                                               _approachAngle_rad.ToFloat());
      IDockAction::PreActionPoseOutput preActionPoseOutput;
    
      IDockAction::GetPreActionPoses(GetRobot().GetPose(), GetRobot().GetCarryingComponent(), GetRobot().GetBlockWorld(),
                                     preActionPoseInput, preActionPoseOutput);
      
      if(preActionPoseOutput.actionResult != ActionResult::SUCCESS)
      {
        return preActionPoseOutput.actionResult;
      }
      
      if(preActionPoseOutput.preActionPoses.empty())
      {
        PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.NoPreActionPoses",
                            "ActionableObject %d did not return any pre-action poses with action type %d.",
                            _objectID.GetValue(), _actionType);
        return ActionResult::NO_PREACTION_POSES;
      }
      
      alreadyInPosition = preActionPoseOutput.robotAtClosestPreActionPose;
      possiblePoses.clear();
      
      if(alreadyInPosition)
      {
        Pose3d p = preActionPoseOutput.preActionPoses[preActionPoseOutput.closestIndex].GetPose();
        PRINT_NAMED_INFO("DriveToObjectAction.GetPossiblePoses.UseRobotPose",
                         "Robot's current pose (x:%f y:%f a:%f) is close enough to preAction pose (x:%f y:%f a:%f)"
                         " with threshold (%f,%f), using current robot pose as goal",
                         GetRobot().GetPose().GetTranslation().x(),
                         GetRobot().GetPose().GetTranslation().y(),
                         GetRobot().GetPose().GetRotation().GetAngleAroundZaxis().getDegrees(),
                         p.GetTranslation().x(),
                         p.GetTranslation().y(),
                         p.GetRotation().GetAngleAroundZaxis().getDegrees(),
                         preActionPoseOutput.distThresholdUsed.x(),
                         preActionPoseOutput.distThresholdUsed.y());
      }
      
      for(auto preActionPose : preActionPoseOutput.preActionPoses)
      {
        possiblePoses.push_back(preActionPose.GetPose());
      }
      
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
          result = ActionResult::NO_DISTANCE_SET;
        } else {
          
          Pose3d objectWrtRobotParent;
          if(false == object->GetPose().GetWithRespectTo(GetRobot().GetPose().GetParent(), objectWrtRobotParent)) {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            result = ActionResult::BAD_POSE;
          } else {
            Point2f vec(GetRobot().GetPose().GetTranslation());
            vec -= Point2f(objectWrtRobotParent.GetTranslation());
            const f32 currentDistance = vec.MakeUnitLength();
            if(currentDistance < _distance_mm) {
              alreadyInPosition = true;
            } else {
              vec *= _distance_mm;
              const Point3f T(vec.x() + objectWrtRobotParent.GetTranslation().x(),
                              vec.y() + objectWrtRobotParent.GetTranslation().y(),
                              GetRobot().GetPose().GetTranslation().z());
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
          
          DriveToPoseAction* driveToPoseAction = new DriveToPoseAction(true);
          driveToPoseAction->SetGoals(possiblePoses, object->GetPose());
          _compoundAction.AddAction(driveToPoseAction);
        }
        
        // Make sure we can see the object, unless we are carrying it (i.e. if we
        // are doing a DriveToPlaceCarriedObject action)
        if(!GetRobot().GetCarryingComponent().IsCarryingObject(object->GetID()))
        {
          TurnTowardsObjectAction* turnTowardsObjectAction = new TurnTowardsObjectAction(_objectID, Radians(0), true, false);
          PRINT_NAMED_DEBUG("IActionRunner.CreatedSubAction", "Parent action [%d] %s created a sub action [%d] %s",
                            GetTag(),
                            GetName().c_str(),
                            turnTowardsObjectAction->GetTag(),
                            turnTowardsObjectAction->GetName().c_str());
          _compoundAction.AddAction(turnTowardsObjectAction);
        }

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
      ActionableObject* object = dynamic_cast<ActionableObject*>(GetRobot().GetBlockWorld().GetLocatedObjectByID(_objectID));
      if(object == nullptr)
      {
        PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            GetRobot().GetID(), _objectID.GetValue());
        return ActionResult::BAD_OBJECT;
      }

      // Use a helper here so that it can be shared with DriveToPlaceCarriedObjectAction
      result = InitHelper(object);

      // Only set cube lights if the dock object is a light cube
      _shouldSetCubeLights = IsValidLightCube(object->GetType(), false);
      
      // Mark this object as one we are docking with (e.g. so its lights indicate
      // it is being interacted with)
      // Need to check if we have set the cube lights already in case the action was reset
      if(_shouldSetCubeLights && !_lightsSet)
      {
        PRINT_CH_INFO("Actions", "DriveToObjectAction.SetInteracting", "%s[%d] Setting interacting object to %d",
                      GetName().c_str(), GetTag(),
                      _objectID.GetValue());
        GetRobot().GetCubeLightComponent().PlayLightAnimByTrigger(_objectID, CubeAnimationTrigger::DrivingTo);
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
        ActionableObject* object = dynamic_cast<ActionableObject*>(GetRobot().GetBlockWorld().GetLocatedObjectByID(_objectID));
        if(object == nullptr)
        {
          PRINT_NAMED_WARNING("DriveToObjectAction.CheckIfDone.NoObjectWithID",
                              "Robot %d's block world does not have an ActionableObject with ID=%d.",
                              GetRobot().GetID(), _objectID.GetValue());
          result = ActionResult::BAD_OBJECT;
        }
        else if( _actionType == PreActionPose::ActionType::NONE)
        {
          // Check to see if we got close enough
          Pose3d objectPoseWrtRobotParent;
          if(false == object->GetPose().GetWithRespectTo(GetRobot().GetPose().GetParent(), objectPoseWrtRobotParent))
          {
            PRINT_NAMED_ERROR("DriveToObjectAction.InitHelper.PoseProblem",
                              "Could not get object pose w.r.t. robot parent pose.");
            result = ActionResult::BAD_OBJECT;
          }
          else
          {
            const f32 distanceSq = (Point2f(objectPoseWrtRobotParent.GetTranslation()) - Point2f(GetRobot().GetPose().GetTranslation())).LengthSq();
            if(distanceSq > _distance_mm*_distance_mm) {
              PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone",
                               "[%d] Robot not close enough, will return FAILURE_RETRY.",
                               GetTag());
              result = ActionResult::DID_NOT_REACH_PREACTION_POSE;
            }
          }
        }
        else
        {
          std::vector<Pose3d> possiblePoses; // don't really need these
          bool inPosition = false;
          result = _getPossiblePosesFunc(object, possiblePoses, inPosition);
          
          if(!inPosition) {
            PRINT_NAMED_INFO("DriveToObjectAction.CheckIfDone",
                             "[%d] Robot not in position, will return FAILURE_RETRY.",
                             GetTag());
            result = ActionResult::DID_NOT_REACH_PREACTION_POSE;
          }
        }
      }
      
      return result;
    }
    
    void DriveToObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted interactionCompleted({{_objectID.GetValue(), -1, -1, -1, -1}}, 1, false);
      completionUnion.Set_objectInteractionCompleted(interactionCompleted);
    }
    
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(const Pose3d& placementPose,
                                                                     const bool placeOnGround,
                                                                     const bool useExactRotation,
                                                                     const bool checkDestinationFree,
                                                                     const float destinationObjectPadding_mm)
    : DriveToObjectAction(0,
                          placeOnGround ? PreActionPose::PLACE_ON_GROUND : PreActionPose::PLACE_RELATIVE,
                          0,
                          false,
                          0)
    , _placementPose(placementPose)
    , _useExactRotation(useExactRotation)
    , _checkDestinationFree(checkDestinationFree)
    , _destinationObjectPadding_mm(destinationObjectPadding_mm)
    {
      SetName("DriveToPlaceCarriedObject");
      SetType(RobotActionType::DRIVE_TO_PLACE_CARRIED_OBJECT);
    }

    void DriveToPlaceCarriedObjectAction::OnRobotSetInternalDriveToObj()
    {
      _objectID = GetRobot().GetCarryingComponent().GetCarryingObject();
    }
    
    ActionResult DriveToPlaceCarriedObjectAction::Init()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if(GetRobot().GetCarryingComponent().IsCarryingObject() == false) {
        PRINT_NAMED_WARNING("DriveToPlaceCarriedObjectAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d cannot place an object because it is not carrying anything.",
                          GetRobot().GetID());
        result = ActionResult::NOT_CARRYING_OBJECT_ABORT;
      } else {
        _objectID = GetRobot().GetCarryingComponent().GetCarryingObject();
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(GetRobot().GetBlockWorld().GetLocatedObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.",
                            GetRobot().GetID(), _objectID.GetValue());
          
          result = ActionResult::BAD_OBJECT;
        } else {
          
          // Compute the approach angle given the desired placement pose of the carried block
          if (_useExactRotation) {
            f32 approachAngle_rad;
            ActionResult res = IDockAction::ComputePlacementApproachAngle(GetRobot(), _placementPose, approachAngle_rad);
            if (res != ActionResult::SUCCESS) {
              PRINT_NAMED_WARNING("DriveToPlaceCarriedObjectAction.Init.FailedToComputeApproachAngle", "");
              return res;
            }
            SetApproachAngle(approachAngle_rad);
          }
          
          // Create a temporary object of the same type at the desired pose so we
          // can get placement poses at that position
          ActionableObject* tempObject = dynamic_cast<ActionableObject*>(object->CloneType());
          DEV_ASSERT(tempObject != nullptr, "DriveToPlaceCarriedObjectAction.Init.DynamicCastFail");
          
          tempObject->InitPose(_placementPose, PoseState::Known);
          
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
          result = ActionResult::PLACEMENT_GOAL_NOT_FREE;
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
      ObservableObject* object = GetRobot().GetBlockWorld().GetLocatedObjectByID(GetRobot().GetCarryingComponent().GetCarryingObject());
      if ( nullptr != object )
      {
        BlockWorldFilter ignoreSelfFilter;
        ignoreSelfFilter.AddIgnoreID( object->GetID() );
      
        // calculate quad at candidate destination
        Quad2f candidateQuad = object->GetBoundingQuadXY(_placementPose);
        
        // TODO rsam: this only checks for other cubes, but not for unknown obstacles since we don't have collision sensor
        std::vector<ObservableObject *> intersectingObjects;
        GetRobot().GetBlockWorld().FindLocatedIntersectingObjects(candidateQuad, intersectingObjects, _destinationObjectPadding_mm, ignoreSelfFilter);
        bool isFree = intersectingObjects.empty();
        return isFree;
      }
      
      // no object :(
      return true;
    }
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction(const bool forceHeadDown)
    : IAction("DriveToPose",
              RobotActionType::DRIVE_TO_POSE,
              (u8)AnimTrackFlag::BODY_TRACK)
    , _isGoalSet(false)
    , _driveWithHeadDown(forceHeadDown)
    , _selectedGoalIndex(new Planning::GoalID(0))
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    , _maxPlanningTime(DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S)
    , _maxReplanPlanningTime(DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S)
    , _timeToAbortPlanning(-1.0f)
    {

    }
    
    DriveToPoseAction::DriveToPoseAction(const Pose3d& pose,
                                         const bool forceHeadDown,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
    : DriveToPoseAction(forceHeadDown)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;
      
      SetGoal(pose, distThreshold, angleThreshold);
    }
    
    DriveToPoseAction::DriveToPoseAction(const std::vector<Pose3d>& poses,
                                         const bool forceHeadDown,
                                         const Point3f& distThreshold,
                                         const Radians& angleThreshold,
                                         const float maxPlanningTime,
                                         const float maxReplanPlanningTime)
    : DriveToPoseAction(forceHeadDown)
    {
      _maxPlanningTime = maxPlanningTime;
      _maxReplanPlanningTime = maxReplanPlanningTime;
      
      SetGoals(poses, distThreshold, angleThreshold);
    }
    
    DriveToPoseAction::~DriveToPoseAction()
    {
      if(!HasRobot()){
        return;
      }
      
      auto& pathComponent = GetRobot().GetPathComponent();

      // If we are not running anymore, for any reason, clear the path and its
      // visualization
      if( pathComponent.IsActive() ) {
        pathComponent.Abort();
      }

      GetRobot().GetContext()->GetVizManager()->EraseAllPlannerObstacles(true);
      GetRobot().GetContext()->GetVizManager()->EraseAllPlannerObstacles(false);
      
      GetRobot().GetDrivingAnimationHandler().ActionIsBeingDestroyed();
    }
    
    Result DriveToPoseAction::SetGoal(const Pose3d& pose,
                                      const Point3f& distThreshold,
                                      const Radians& angleThreshold)
    {

      if( GetState() != ActionResult::NOT_STARTED ) {
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

      if( GetState() != ActionResult::NOT_STARTED ) {
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
    
    Result DriveToPoseAction::SetGoals(const std::vector<Pose3d>& poses,
                                       const Pose3d& objectPoseGoalsGeneratedFrom,
                                       const Point3f& distThreshold,
                                       const Radians& angleThreshold)
    {
      _objectPoseGoalsGeneratedFrom = objectPoseGoalsGeneratedFrom;
      _useObjectPose = true;
      return SetGoals(poses, distThreshold, angleThreshold);
    }

    void DriveToPoseAction::GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const
    {
      requests.insert({ VisionMode::DetectingMarkers , EVisionUpdateFrequency::Low });
    }

    f32 DriveToPoseAction::GetTimeoutInSeconds() const { return kDriveToPoseTimeout; }  

    ActionResult DriveToPoseAction::Init()
    {
      GetRobot().GetDrivingAnimationHandler().Init(GetTracksToLock(), GetTag(), IsSuppressingTrackLocking());
    
      ActionResult result = ActionResult::SUCCESS;

      auto& pathComponent = GetRobot().GetPathComponent();
      
      _timeToAbortPlanning = -1.0f;
      
      if(!_isGoalSet) {
        PRINT_NAMED_ERROR("DriveToPoseAction.Init.NoGoalSet",
                          "Goal must be set before running this action.");
        result = ActionResult::NO_GOAL_SET;
      }
      else {
        
        // Make the poses w.r.t. robot:
        for(auto & pose : _goalPoses) {
          if(pose.GetWithRespectTo(GetRobot().GetWorldOrigin(), pose) == false) {
            // this means someone passed in a goal in a different origin than the robot.
            PRINT_NAMED_WARNING("DriveToPoseAction.Init.OriginMisMatch",
                                "Could not get goal pose w.r.t. to robot origin.");
            return ActionResult::BAD_POSE;
          }
        }
        
        Result planningResult = RESULT_OK;
        
        *_selectedGoalIndex = 0;
        
        pathComponent.SetCanReplanningChangeGoal( !_mustUseOriginalGoal );
        
        planningResult = pathComponent.StartDrivingToPose(_goalPoses,
                                                          _selectedGoalIndex);
        
        if(planningResult != RESULT_OK) {
          PRINT_CH_INFO("Actions", "DriveToPoseAction.Init.FailedToFindPath",
                        "[%d] Failed to get path to goal pose.",
                        GetTag());
          result = ActionResult::PATH_PLANNING_FAILED_ABORT;
        }
        
        if(result == ActionResult::SUCCESS) {
          // So far so good.
          
          if(_driveWithHeadDown) {
            // Now put the head at the right angle for following paths
            // TODO: Make it possible to set the speed/accel somewhere?
            if(GetRobot().GetMoveComponent().MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 2.f, 5.f) != RESULT_OK) {
              PRINT_NAMED_ERROR("DriveToPoseAction.Init.MoveHeadFailed", "Failed to move head to path-following angle.");
              result = ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
            }
          }
        }        
      } // if/else isGoalSet
      
      return result;
    } // Init()
    
    ActionResult DriveToPoseAction::CheckIfDone()
    {
      ActionResult result = ActionResult::RUNNING;
      
      // Still running while the drivingEnd animation is playing
      if(GetRobot().GetDrivingAnimationHandler().IsPlayingDrivingEndAnim())
      {
        return ActionResult::RUNNING;
      }
      
      switch( GetRobot().GetPathComponent().GetDriveToPoseStatus() ) {
        case ERobotDriveToPoseStatus::Failed:
        {
          PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Failure", "Robot driving to pose failed");
          _timeToAbortPlanning = -1.0f;
          result = ActionResult::PATH_PLANNING_FAILED_ABORT;
        }
          break;
        
        case ERobotDriveToPoseStatus::ComputingPath:
        {
          const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

          // handle aborting the plan. If we don't have a timeout set, set one now
          if( _timeToAbortPlanning < 0.0f ) {
            _timeToAbortPlanning = currTime + _maxPlanningTime;
          }
          else if( currTime >= _timeToAbortPlanning ) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.ComputingPathTimeout",
                             "Robot has been planning for more than %f seconds, aborting",
                             _maxPlanningTime);
            GetRobot().GetPathComponent().Abort();
            result = ActionResult::PATH_PLANNING_FAILED_ABORT;
            _timeToAbortPlanning = -1.0f;
          }
        }
          break;
          
        case ERobotDriveToPoseStatus::FollowingPath:
        {

          // If we are following a path start playing driving animations
          // Won't do anything if DrivingAnimationHandler has already been inited
          GetRobot().GetDrivingAnimationHandler().StartDrivingAnim();

          // clear abort timing, since we got a path
          _timeToAbortPlanning = -1.0f;

          if(_debugPrintCtr++ % 10 == 0) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.WaitingForPathCompletion",
                             "[%d] Waiting for robot to complete its path traversal (%d), "
                             "_currPathSegment=%d, _lastSentPathID=%d, _lastRecvdPathID=%d.",
                             GetTag(),
                             _debugPrintCtr,
                             GetRobot().GetPathComponent().GetCurrentPathSegment(),
                             GetRobot().GetPathComponent().GetLastSentPathID(),
                             GetRobot().GetPathComponent().GetLastRecvdPathID());
          }
        }
          break;
         
        case ERobotDriveToPoseStatus::Ready:
        {
          // clear abort timing, since we had a path
          _timeToAbortPlanning = -1.0f;
          
          // No longer traversing the path, so check to see if we ended up in the right place
          Vec3f Tdiff;
          
          // HACK: Loosen z threshold bigtime:
          Point3f distanceThreshold(_goalDistanceThreshold.x(),
                                    _goalDistanceThreshold.y(),
                                    GetRobot().GetHeight());
          
          // If the goals were generated from an object then compute the distance threshold using the
          // pose of the goal that was actually selected
          if(_useObjectPose)
          {
            const Point2f thresh = ComputePreActionPoseDistThreshold(_goalPoses[*_selectedGoalIndex],
                                                                     _objectPoseGoalsGeneratedFrom,
                                                                     _goalAngleThreshold);
            
            distanceThreshold.x() = thresh.x();
            distanceThreshold.y() = thresh.y();
          }
          
          if(GetRobot().GetPose().IsSameAs(_goalPoses[*_selectedGoalIndex], distanceThreshold, _goalAngleThreshold, Tdiff))
          {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Success",
                             "[%d] Robot %d successfully finished following path (Tdiff=%.1fmm) robotPose (%.1f, %.1f) goalPose (%.1f %.1f) threshold (%.1f %.1f).",
                             GetTag(),
                             GetRobot().GetID(),
                             Tdiff.Length(),
                             GetRobot().GetPose().GetTranslation().x(),
                             GetRobot().GetPose().GetTranslation().y(),
                             _goalPoses[*_selectedGoalIndex].GetTranslation().x(),
                             _goalPoses[*_selectedGoalIndex].GetTranslation().y(),
                             distanceThreshold.x(),
                             distanceThreshold.y());
            
            result = ActionResult::SUCCESS;
          }
          // The last path sent was definitely received by the robot
          // and it is no longer executing it, but we appear to not be in position
          else if (GetRobot().GetPathComponent().GetLastSentPathID() == GetRobot().GetPathComponent().GetLastRecvdPathID()) {
            PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.DoneNotInPlace",
                             "[%d] Robot is done traversing path, but is not in position (dist=%.1fmm). lastReceivedPathID=%d lastSentPathID=%d"
                             " goal %d (%f, %f, %f, %fdeg), actual (%f, %f, %f, %fdeg), threshold (%f, %f)",
                             GetTag(),
                             Tdiff.Length(), GetRobot().GetPathComponent().GetLastRecvdPathID(),
                             GetRobot().GetPathComponent().GetLastSentPathID(),
                             (int) *_selectedGoalIndex,
                             _goalPoses[*_selectedGoalIndex].GetTranslation().x(),
                             _goalPoses[*_selectedGoalIndex].GetTranslation().y(),
                             _goalPoses[*_selectedGoalIndex].GetTranslation().z(),
                             _goalPoses[*_selectedGoalIndex].GetRotationAngle<'Z'>().getDegrees(),
                             GetRobot().GetPose().GetTranslation().x(),
                             GetRobot().GetPose().GetTranslation().y(),
                             GetRobot().GetPose().GetTranslation().z(),
                             GetRobot().GetPose().GetRotationAngle<'Z'>().getDegrees(),
                             distanceThreshold.x(),
                             distanceThreshold.y());
            
            result = ActionResult::FAILED_TRAVERSING_PATH;
          }
          else {
            // Something went wrong: not in place and robot apparently hasn't
            // received all that it should have
            PRINT_NAMED_ERROR("DriveToPoseAction.CheckIfDone.Failure",
                              "Robot is not at the goal and did not receive the last path");
            result = ActionResult::FOLLOWING_PATH_BUT_NOT_TRAVERSING;
          }
        }
          break;

        case ERobotDriveToPoseStatus::WaitingToBeginPath:         
        case ERobotDriveToPoseStatus::WaitingToCancelPath:
        case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure:
          // nothing to do, just waiting for the robot (path component will timeout on it's own here, if
          // needed)
          break;
      }
      
      // If we are no longer running and have at least started moving (path planning succeeded)
      // then start the drivingEnd animation and keep this action running
      if(result != ActionResult::RUNNING &&
         result != ActionResult::PATH_PLANNING_FAILED_ABORT &&
         result != ActionResult::PATH_PLANNING_FAILED_RETRY &&
         GetRobot().GetDrivingAnimationHandler().EndDrivingAnim())
      {
        result = ActionResult::RUNNING;
      }

      return result;
    } // CheckIfDone()
    
#pragma mark ---- IDriveToInteractWithObjectAction ----
    
    IDriveToInteractWithObject::IDriveToInteractWithObject(const ObjectID& objectID,
                                                           const PreActionPose::ActionType& actionType,
                                                           const f32 predockOffsetDistX_mm,
                                                           const bool useApproachAngle,
                                                           const f32 approachAngle_rad,
                                                           const Radians& maxTurnTowardsFaceAngle_rad,
                                                           const bool sayName)
    : CompoundActionSequential()
    , _objectID(objectID)
    , _preDockPoseDistOffsetX_mm(predockOffsetDistX_mm)
    {
      IActionRunner* driveToObjectAction = new DriveToObjectAction(_objectID,
                                                                   actionType,
                                                                   predockOffsetDistX_mm,
                                                                   useApproachAngle,
                                                                   approachAngle_rad);

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

      if( kEnablePredockDistanceCheckFix ) {

        // This is a workaround for the bug in COZMO-5880. The problem is that the DriveTo action has a
        // predock pose check which is different from the Dock action. This causes DriveTo to succeed, but
        // Dock to fail. Then, if this whole action is retried, the same thing happens, and the robot fails to
        // dock without ever moving

        // The fix is to _not_ do the check within the dock action, but _only_ if the entire driveTo action
        // succeeds. We need to ignore failures from the drive action because of the way the proxy action
        // works, so we work around this by adding an inner action. The inner action cannot fail, but within
        // the inner action is the drive to action, which _can_ fail. If that action fails, it will _not_ call
        // the Wait action, otherwise it will. This way, we have a lambda that only gets called when the drive
        // to succeeds

        // create an inner action to hold the drive to and the lambda
        CompoundActionSequential* innerAction = new CompoundActionSequential();
        // within innerAction, we do want to consider failures of driving (to prevent the lambda from running
        // if the drive fails)
        _driveToObjectAction = innerAction->AddAction(driveToObjectAction, false);

        auto waitLambda = [this](Robot& robot) {
          if (_shouldSetCubeLights) {
            // Keep the cube lights set while the waitForLambda action is running
            robot.GetCubeLightComponent().PlayLightAnimByTrigger(_objectID, CubeAnimationTrigger::DrivingTo);
          }
          
          // if this lambda gets called, that means the drive to must have succeeded.
          if( !_dockAction.expired() ) {
            PRINT_CH_INFO("Actions", "IDriveToInteractWithObject.DriveToSuccess",
                          "DriveTo action succeeded, telling dock action not to check predock pose distance");
              
            // For debug builds do a dynamic cast for validity checks
            DEV_ASSERT(dynamic_cast<IDockAction*>(_dockAction.lock().get()) != nullptr,
                       "IDriveToInteractWithObjectAction.Constructor.DynamicCastFailed");
                        
            IDockAction* rawDockAction = static_cast<IDockAction*>(_dockAction.lock().get());
            rawDockAction->SetDoNearPredockPoseCheck(false);
          }
          else {
            PRINT_NAMED_ERROR("IDriveToInteractWithObject.InnerAction.WaitLambda.NoDockAction",
                              "Dock action is null! This is a bug!!!");
          }

          // immediately finish the wait action
          return true;
        };

        WaitForLambdaAction* waitAction = new WaitForLambdaAction(waitLambda);
        innerAction->AddAction(waitAction, false);

        // Add the entire inner action, but ignore failures here so that we will always run the dock action
        // even if driving fails (so that the dock action will be the one to fail)
        AddAction(innerAction, true);
      }
      else {
        _driveToObjectAction = AddAction(driveToObjectAction, true);
      }
          
      if(maxTurnTowardsFaceAngle_rad > 0.f)
      {
        _turnTowardsLastFacePoseAction = AddAction(new TurnTowardsLastFacePoseAction(maxTurnTowardsFaceAngle_rad,
                                                                                     sayName),
                                                   true);
          
        _turnTowardsObjectAction = AddAction(new TurnTowardsObjectAction(_objectID,
                                                                         maxTurnTowardsFaceAngle_rad),
                                             true);
      }
    }
    
    IDriveToInteractWithObject::IDriveToInteractWithObject(const ObjectID& objectID,
                                                           const f32 distance)
    : CompoundActionSequential()
    , _objectID(objectID)
    {
      _driveToObjectAction = AddAction(new DriveToObjectAction(_objectID,
                                                               distance),
                                       true);

    }

    void IDriveToInteractWithObject::OnRobotSetInternalCompound()
    {
      if(_objectID == GetRobot().GetCarryingComponent().GetCarryingObject())
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.Constructor",
                            "Robot is currently carrying action object with ID=%d",
                            _objectID.GetValue());
        return;
      }
      
      if(auto driveAction = _driveToObjectAction.lock()){
        driveAction->SetRobot(&GetRobot());
      }
      if(auto turnFaceAction = _turnTowardsLastFacePoseAction.lock()){
        turnFaceAction->SetRobot(&GetRobot());
      }
      if(auto turnObjAction = _turnTowardsObjectAction.lock()){
        turnObjAction->SetRobot(&GetRobot());
      }
      if(auto dockAction = _dockAction.lock()){
        dockAction->SetRobot(&GetRobot());
      }
    } // end OnRobotSetInternalCompound()
    
    std::weak_ptr<IActionRunner> IDriveToInteractWithObject::AddDockAction(IDockAction* dockAction,
                                                                           bool ignoreFailure)
    {
      if(HasRobot()){
        dockAction->SetRobot(&GetRobot());
      }
      
      // right before the dock action, we want to call the PreDock callback (if one was specified). TO achieve
      // this, we use a WaitForLambda action which always completes immediately.
      // TODO:(bn) this could be done much more elegantly as part of CompoundActionSequential
      {
        auto lambdaToWaitFor = [this](Robot& robot) {
          if (_shouldSetCubeLights) {
            // Keep the cube lights set while the waitForLambda action is running
            GetRobot().GetCubeLightComponent().PlayLightAnimByTrigger(_objectID, CubeAnimationTrigger::DrivingTo);
          }
          
          if( _preDockCallback ) {
            _preDockCallback(robot);
          }
          // immediately finish the action
          return true;
        };
        AddAction( new WaitForLambdaAction(lambdaToWaitFor) );
      }
      
      dockAction->SetPreDockPoseDistOffset(_preDockPoseDistOffsetX_mm);
      _dockAction = AddAction(dockAction, ignoreFailure);
      return _dockAction;
    }

    void IDriveToInteractWithObject::SetSayNameAnimationTrigger(AnimationTrigger trigger)
    {
      if( HasStarted() ) {
        PRINT_NAMED_ERROR("IDriveToInteractWithObject.SetSayNameAnimationTrigger.AfterRunning",
                          "Tried to update the animations after the action started, this isn't supported");
        return;
      }
      if( !_turnTowardsLastFacePoseAction.expired() ) {
        static_cast<TurnTowardsLastFacePoseAction*>(_turnTowardsLastFacePoseAction.lock().get())->SetSayNameAnimationTrigger(trigger);
      }
    }
      
    void IDriveToInteractWithObject::SetNoNameAnimationTrigger(AnimationTrigger trigger)
    {
      if( HasStarted() ) {
        PRINT_NAMED_ERROR("IDriveToInteractWithObject.SetNoNameAnimationTrigger.AfterRunning",
                          "Tried to update the animations after the action started, this isn't supported");
        return;
      }
      if(!_turnTowardsLastFacePoseAction.expired()) {
        static_cast<TurnTowardsLastFacePoseAction*>(_turnTowardsLastFacePoseAction.lock().get())->SetNoNameAnimationTrigger(trigger);
      }
    }
    
    void IDriveToInteractWithObject::DontTurnTowardsFace()
    {
      if(!_turnTowardsObjectAction.expired() &&
         !_turnTowardsLastFacePoseAction.expired())
      {
        _turnTowardsLastFacePoseAction.lock()->ForceComplete();
        _turnTowardsObjectAction.lock()->ForceComplete();
      }
    }
    
    void IDriveToInteractWithObject::SetMaxTurnTowardsFaceAngle(const Radians angle)
    {
      if(_turnTowardsObjectAction.expired() ||
         _turnTowardsLastFacePoseAction.expired())
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetMaxTurnTowardsFaceAngle",
                            "Can not set angle of null actions (the action were originally constructed with an angle of zero)");
        return;
      }
      PRINT_NAMED_DEBUG("IDriveToInteractWithObject.SetMaxTurnTowardsFaceAngle",
                        "Setting maxTurnTowardsFaceAngle to %f degrees", angle.getDegrees());
      static_cast<TurnTowardsLastFacePoseAction*>(_turnTowardsLastFacePoseAction.lock().get())->SetMaxTurnAngle(angle);
      static_cast<TurnTowardsObjectAction*>(_turnTowardsObjectAction.lock().get())->SetMaxTurnAngle(angle);
    }
    
    void IDriveToInteractWithObject::SetTiltTolerance(const Radians tol)
    {
      if(_turnTowardsObjectAction.expired() ||
         _turnTowardsLastFacePoseAction.expired())
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetTiltTolerance",
                            "Can not set angle of null actions (the action were originally constructed with an angle of zero)");
        return;
      }
      PRINT_NAMED_DEBUG("IDriveToInteractWithObject.SetTiltTolerance",
                        "Setting tilt tolerance to %f degrees", tol.getDegrees());
      static_cast<TurnTowardsLastFacePoseAction*>(_turnTowardsLastFacePoseAction.lock().get())->SetTiltTolerance(tol);
      static_cast<TurnTowardsObjectAction*>(_turnTowardsObjectAction.lock().get())->SetTiltTolerance(tol);
    }

    void IDriveToInteractWithObject::SetPreActionPoseAngleTolerance(f32 angle_rad)
    {
      if( GetState() != ActionResult::NOT_STARTED ) {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetPreActionPoseAngleTolerance.Invalid",
                            "Tried to set the preaction pose angle tolerance, but action has already started");
        return;
      }

      if(!_driveToObjectAction.expired()) {
        PRINT_CH_INFO("Actions", "IDriveToInteractWithObject.SetPreActionPoseAngleTolerance",
                      "[%d] %f rad",
                      GetTag(),
                      angle_rad);
        
        static_cast<DriveToObjectAction*>(_driveToObjectAction.lock().get())->SetPreActionPoseAngleTolerance(angle_rad);
      } else {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetApproachAngle.NullDriveToAction", "");
      }

    }

    void IDriveToInteractWithObject::SetApproachAngle(const f32 angle_rad)
    {
      if( GetState() != ActionResult::NOT_STARTED ) {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetApproachAngle.Invalid",
                            "Tried to set the approach angle, but action has already started");
        return;
      }

      if(!_driveToObjectAction.expired()) {
        PRINT_CH_INFO("Actions", "IDriveToInteractWithObject.SetApproachingAngle",
                      "[%d] %f rad",
                      GetTag(),
                      angle_rad);
        
        static_cast<DriveToObjectAction*>(_driveToObjectAction.lock().get())->SetApproachAngle(angle_rad);
      } else {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.SetApproachAngle.NullDriveToAction", "");
      }
    }


    const bool IDriveToInteractWithObject::GetUseApproachAngle() const
    {
      if(!_driveToObjectAction.expired())
      {
        return static_cast<DriveToObjectAction*>(_driveToObjectAction.lock().get())->GetUseApproachAngle();
      }
      return false;
    }
        
    void IDriveToInteractWithObject::SetShouldCheckForObjectOnTopOf(const bool b)
    {
      if(!_dockAction.expired())
      {
        static_cast<IDockAction*>(_dockAction.lock().get())->SetShouldCheckForObjectOnTopOf(b);
      }
      else
      {
        PRINT_NAMED_ERROR("IDriveToInteractWithObject.SetShouldCheckForObjectOnTopOf.NoDockAction", "");
      }
    }

    Result IDriveToInteractWithObject::UpdateDerived()
    {      
      if(_shouldSetCubeLights && !_lightsSet) {
        PRINT_CH_INFO("Actions", "IDriveToInteractWithObject.SetInteracting", "%s[%d] Setting interacting object to %d",
                      GetName().c_str(), GetTag(),
                      _objectID.GetValue());
        GetRobot().GetCubeLightComponent().PlayLightAnimByTrigger(_objectID, CubeAnimationTrigger::DrivingTo);
        _lightsSet = true;
      }
      return RESULT_OK;
    }
    
    IDriveToInteractWithObject::~IDriveToInteractWithObject()
    {
      if(_lightsSet) {
        PRINT_CH_INFO("Actions", "IDriveToInteractWithObject.UnsetInteracting", "%s[%d] Unsetting interacting object to %d",
                      GetName().c_str(), GetTag(),
                      _objectID.GetValue());
        GetRobot().GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::DrivingTo, _objectID);
        _lightsSet = false;
      }
    }
    
#pragma mark ---- DriveToAlignWithObjectAction ----
    
    DriveToAlignWithObjectAction::DriveToAlignWithObjectAction(const ObjectID& objectID,
                                                               const f32 distanceFromMarker_mm,
                                                               const bool useApproachAngle,
                                                               const f32 approachAngle_rad,
                                                               const AlignmentType alignmentType,
                                                               Radians maxTurnTowardsFaceAngle_rad,
                                                               const bool sayName)
    : IDriveToInteractWithObject(objectID,
                                 AlignWithObjectAction::GetPreActionTypeFromAlignmentType(alignmentType),
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      AlignWithObjectAction* action = new AlignWithObjectAction(
                                               objectID,
                                               distanceFromMarker_mm,
                                               alignmentType);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
#pragma mark ---- DriveToPickupObjectAction ----
    
    DriveToPickupObjectAction::DriveToPickupObjectAction(const ObjectID& objectID,
                                                         const bool useApproachAngle,
                                                         const f32 approachAngle_rad,
                                                         Radians maxTurnTowardsFaceAngle_rad,
                                                         const bool sayName,
                                                         AnimationTrigger animBeforeDock)
    : IDriveToInteractWithObject(objectID,
                                 PreActionPose::DOCKING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      if(animBeforeDock != AnimationTrigger::Count){
        AddAction(new TriggerAnimationAction(animBeforeDock));
      }
      
      PickupObjectAction* rawPickup = new PickupObjectAction(objectID);
      const u32 pickUpTag = rawPickup->GetTag();
      _pickupAction = AddDockAction(rawPickup);
      SetProxyTag(pickUpTag);
    }
    
    void DriveToPickupObjectAction::SetDockingMethod(DockingMethod dockingMethod)
    {
      if(!_pickupAction.expired()) {
        static_cast<IDockAction*>(_pickupAction.lock().get())->SetDockingMethod(dockingMethod);
      } else {
        PRINT_NAMED_WARNING("DriveToPickupObjectAction.SetDockingMethod.NullPickupAction", "");
      }
    }
    
    void DriveToPickupObjectAction::SetPostDockLiftMovingAnimation(Anki::Cozmo::AnimationTrigger trigger)
    {
      if(!_pickupAction.expired()) {
        static_cast<IDockAction*>(_pickupAction.lock().get())->SetPostDockLiftMovingAnimation(trigger);
      } else {
        PRINT_NAMED_WARNING("DriveToPickupObjectAction.SetPostDockLiftMovingAnimation.NullPickupAction", "");
      }
    }
    

    
#pragma mark ---- DriveToPlaceOnObjectAction ----
    
    DriveToPlaceOnObjectAction::DriveToPlaceOnObjectAction(const ObjectID& objectID,
                                                           const bool useApproachAngle,
                                                           const f32 approachAngle_rad,
                                                           Radians maxTurnTowardsFaceAngle_rad,
                                                           const bool sayName)
    : IDriveToInteractWithObject(objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      PlaceRelObjectAction* action = new PlaceRelObjectAction(objectID,
                                                              false,
                                                              0,
                                                              0);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
#pragma mark ---- DriveToPlaceRelObjectAction ----

    
    DriveToPlaceRelObjectAction::DriveToPlaceRelObjectAction(const ObjectID& objectID,
                                                             const bool placingOnGround,
                                                             const f32 placementOffsetX_mm,
                                                             const f32 placementOffsetY_mm,
                                                             const bool useApproachAngle,
                                                             const f32 approachAngle_rad,
                                                             Radians maxTurnTowardsFaceAngle_rad,
                                                             const bool sayName,
                                                             const bool relativeCurrentMarker)
    : IDriveToInteractWithObject(objectID,
                                 PreActionPose::PLACE_RELATIVE,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      PlaceRelObjectAction* action = new PlaceRelObjectAction(objectID,
                                                              placingOnGround,
                                                              placementOffsetX_mm,
                                                              placementOffsetY_mm,
                                                              relativeCurrentMarker);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
      
      // when relative current marker all pre-dock poses are valid
      // otherwise, one pre-doc pose may be impossible to place at certain offsets
      if(!relativeCurrentMarker)
      {
        DriveToObjectAction* driveToAction = GetDriveToObjectAction();
        if(driveToAction != nullptr)
        {
          driveToAction->SetGetPossiblePosesFunc(
            [this, placementOffsetX_mm, placementOffsetY_mm](ActionableObject* object,
                                                               std::vector<Pose3d>& possiblePoses,
                                                               bool& alreadyInPosition)
            {
              return PlaceRelObjectAction::ComputePlaceRelObjectOffsetPoses(object,
                                                                            placementOffsetX_mm,
                                                                            placementOffsetY_mm,
                                                                            GetRobot().GetPose(),
                                                                            GetRobot().GetWorldOrigin(),
                                                                            GetRobot().GetCarryingComponent(),
                                                                            GetRobot().GetBlockWorld(),
                                                                            GetRobot().GetVisionComponent(),
                                                                            possiblePoses,
                                                                            alreadyInPosition);

            });
        }
        else
        {
          PRINT_CH_INFO("Actions",
                        "DriveToPlaceRelObjectAction.PossiblePosesFunction.NoDriveToAction",
                        "DriveToAction not set, possible invalid poses");
        }
      }
    }
 
    
#pragma mark ---- DriveToRollObjectAction ----
    
    DriveToRollObjectAction::DriveToRollObjectAction(const ObjectID& objectID,
                                                     const bool useApproachAngle,
                                                     const f32 approachAngle_rad,
                                                     Radians maxTurnTowardsFaceAngle_rad,
                                                     const bool sayName)
    : IDriveToInteractWithObject(objectID,
                                 PreActionPose::ROLLING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    , _objectID(objectID)
    {
      _rollAction = AddDockAction(new RollObjectAction(objectID));
      SetProxyTag(_rollAction.lock().get()->GetTag());
    }

    void DriveToRollObjectAction::RollToUpright(const BlockWorld& blockWorld, const Pose3d& robotPose)
    {
      if( GetState() != ActionResult::NOT_STARTED ) {
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
      
      f32 approachAngle_rad;
      if(DriveToRollObjectAction::GetRollToUprightApproachAngle(blockWorld,
                                                                robotPose,
                                                                _objectID,
                                                                approachAngle_rad)){
        SetApproachAngle(approachAngle_rad);
      }
      
    }
    
    bool DriveToRollObjectAction::GetRollToUprightApproachAngle(const BlockWorld& blockWorld,
                                                                const Pose3d& robotPose,
                                                                const ObjectID& objID,
                                                                f32& approachAngle_rad)
    {

      if( !objID.IsSet() ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.RollToUprightStatic.NoObject",
                            "invalid object id");
        return false;
      }
      
      std::vector<std::pair<Quad2f, ObjectID> > obstacles;
      blockWorld.GetObstacles(obstacles);

      // Compute approach angle so that rolling rights the block, using docking
      const ObservableObject* observableObject = blockWorld.GetLocatedObjectByID(objID);
      if( nullptr == observableObject ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.RollToUpright.NullObject",
                            "invalid object id %d",
                            objID.GetValue());
        return false;
      }

      if( observableObject->GetFamily() != ObjectFamily::LightCube &&
          observableObject->GetFamily() != ObjectFamily::Block ) {
        PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WrongFamily",
                      "Can only use this function on blocks or light cubes, ignoring call");
        return false;
      }

      // unfortunately this needs to be a dynamic cast because Block inherits from observable object virtually
      const Block* block = dynamic_cast<const Block*>(observableObject);
      if( block == nullptr ) {
        PRINT_NAMED_ERROR("DriveToRollObjectAction.RollToUpright.NotABlock",
                          "object %d exists, but can't be cast to a Block. This is a bug",
                          objID.GetValue());
        return false;
      }

      
      std::vector<PreActionPose> preActionPoses;
      block->GetCurrentPreActionPoses(preActionPoses,
                                      robotPose,
                                      {PreActionPose::ROLLING},
                                      std::set<Vision::Marker::Code>(),
                                      obstacles);

      if( preActionPoses.empty() ) {
        PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WillNotUpright.NoPoses",
                      "No valid pre-dock poses to roll object %d, not restricting pose",
                      objID.GetValue());
        return false;
      }

      // if we have any valid predock poses which approach the bottom face, use those

      const Vision::KnownMarker& bottomMarker = block->GetMarker( Block::FaceName::BOTTOM_FACE );

      for( const auto& preActionPose : preActionPoses ) {
        const Vision::KnownMarker* marker = preActionPose.GetMarker();
        if( nullptr != marker && marker->GetCode() == bottomMarker.GetCode() ) {
          // found at least one valid pre-action pose using the bottom marker, so limit the approach angle so
          // we will roll the block to upright
          Vec3f approachVec = ComputeVectorBetween(block->GetPose(), preActionPose.GetPose());
          approachAngle_rad = atan2f(approachVec.y(), approachVec.x());
          PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WillUpright",
                        "Found a predock pose that should upright cube %d",
                        objID.GetValue());
          return true;
        }
      }

      // if we got here, that means none of the predock poses (if there are any) will roll from the bottom. In
      // this case, don't limit the predock poses at all. This will make it so we *might* get lucky and roll
      // the cube into a state where we can roll it again to upright it, although there is no guarantee. A
      // real solution would need a high-level planner to solve this. By doing nothing here, we don't limit
      // the approach angle at all
      PRINT_CH_INFO("Actions", "DriveToRollObjectAction.RollToUpright.WillNotUpright.NoBottomPose",
                    "none of the %zu actions will upright the cube, allowing any",
                    preActionPoses.size());
      return false;
    }
    
    Result DriveToRollObjectAction::EnableDeepRoll(bool enable)
    {
      if( GetState() != ActionResult::NOT_STARTED ) {
        PRINT_NAMED_WARNING("DriveToRollObjectAction.EnableDeepRoll.Invalid",
                            "[%d] Tried to set deep roll mode, but action has started",
                            GetTag());
        return RESULT_FAIL;
      }
      
      DEV_ASSERT(!_rollAction.expired(), "DriveToRollObjectAction.actionIsNull");
      static_cast<RollObjectAction*>(_rollAction.lock().get())->EnableDeepRoll(enable);
      return RESULT_OK;
    }
    
#pragma mark ---- DriveToPopAWheelieAction ----
    
    DriveToPopAWheelieAction::DriveToPopAWheelieAction(const ObjectID& objectID,
                                                       const bool useApproachAngle,
                                                       const f32 approachAngle_rad,
                                                       Radians maxTurnTowardsFaceAngle_rad,
                                                       const bool sayName)
    : IDriveToInteractWithObject(objectID,
                                 PreActionPose::ROLLING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      PopAWheelieAction* action = new PopAWheelieAction(objectID);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
    
#pragma mark ---- DriveToFacePlantAction ----
    
    DriveToFacePlantAction::DriveToFacePlantAction(const ObjectID& objectID,
                                                   const bool useApproachAngle,
                                                   const f32 approachAngle_rad,
                                                   Radians maxTurnTowardsFaceAngle_rad,
                                                   const bool sayName)
    : IDriveToInteractWithObject(objectID,
                                 PreActionPose::DOCKING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    {
      FacePlantAction* action = new FacePlantAction(objectID);
      AddDockAction(action);
      SetProxyTag(action->GetTag());
    }
    
    
#pragma mark ---- DriveToReAlignWithObjectAction ----
    
    DriveToRealignWithObjectAction::DriveToRealignWithObjectAction(ObjectID objectID,
                                                                   float dist_mm)
    : CompoundActionSequential()
    , _objectID(objectID)
    , _dist_mm(dist_mm)
    {

    }


    void DriveToRealignWithObjectAction::OnRobotSetInternalCompound()
    {
      const f32 minTrans = 20.0f;
      const f32 moveBackDist = 35.0f;
      const f32 waitTime = 3.0f;
      

      ObservableObject* observableObject = GetRobot().GetBlockWorld().GetLocatedObjectByID(_objectID);
      if(nullptr == observableObject)
      {
        PRINT_NAMED_WARNING("DriveToRealignWithObjectAction.Constructor.NullObservableObject",
                            "ObjectID=%d. Will not use add MoveHead+DriveStraight+Wait actions.",
                            _objectID.GetValue());
      }
      else
      {
        // if block's state is not known, find it.
        Pose3d p;
        observableObject->GetPose().GetWithRespectTo(GetRobot().GetPose(), p);
        if(!(observableObject->IsPoseStateKnown()) || (p.GetTranslation().y() < minTrans))
        {
          MoveHeadToAngleAction* moveHeadToAngleAction = new MoveHeadToAngleAction(kIdealViewBlockHeadAngle);
          AddAction(moveHeadToAngleAction);
          DriveStraightAction* driveAction = new DriveStraightAction(-moveBackDist);
          driveAction->SetShouldPlayAnimation(false);
          AddAction(driveAction);
          WaitAction* waitAction = new WaitAction(waitTime);
          AddAction(waitAction);
        }
      }
      
      // Drive towards found block and verify it.
      DriveToAlignWithObjectAction* driveToAlignWithObjectAction = new DriveToAlignWithObjectAction(_objectID, _dist_mm);
      driveToAlignWithObjectAction->SetNumRetries(0);
      AddAction(driveToAlignWithObjectAction);
      SetNumRetries(0);
    }

  }
}


