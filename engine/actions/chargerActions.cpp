/**
 * File: chargerActions.cpp
 *
 * Author: Matt Michini
 * Date:   8/10/2017
 *
 * Description: Implements charger-related actions, e.g. docking with the charger.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/actions/chargerActions.h"

#include "engine/actions/animActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/batteryComponent.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
#pragma mark ---- MountChargerAction ----
  
MountChargerAction::MountChargerAction(ObjectID chargerID,
                                       const bool useCliffSensorCorrection,
                                       const bool shouldPlayDrivingAnimation)
  : IAction("MountCharger",
            RobotActionType::MOUNT_CHARGER,
            (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::LIFT_TRACK)
  , _chargerID(chargerID)
  , _useCliffSensorCorrection(useCliffSensorCorrection)
  , _playDrivingAnimation(shouldPlayDrivingAnimation)
{
  
}


MountChargerAction::~MountChargerAction()
{
  if (HasRobot() && _playDrivingAnimation) {
    GetRobot().GetDrivingAnimationHandler().ActionIsBeingDestroyed();
  }
}

  
ActionResult MountChargerAction::Init()
{
  if (_playDrivingAnimation) {
    // Init the driving animation handler
    GetRobot().GetDrivingAnimationHandler().Init(GetTracksToLock(), GetTag(), IsSuppressingTrackLocking(), true);
  }
  
  // Reset the compound actions to ensure they get re-configured:
  _mountAction.reset();
  _driveForRetryAction.reset();
  
  // Verify that we have a charger in the world that matches _chargerID
  const auto* charger = GetRobot().GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
  if ((charger == nullptr) ||
      (charger->GetType() != ObjectType::Charger_Basic)) {
    PRINT_NAMED_WARNING("MountChargerAction.Init.InvalidCharger",
                        "No charger object with ID %d in block world!",
                        _chargerID.GetValue());
    return ActionResult::BAD_OBJECT;
  }
  
  // Tell robot which charger it will be using
  GetRobot().SetCharger(_chargerID);

  // Set up the turnAndMount compound action
  ActionResult result = ConfigureMountAction();
  
  return result;
}


ActionResult MountChargerAction::CheckIfDone()
{ 
  auto result = ActionResult::RUNNING;
  
  // Tick the turnAndMount action (if needed):
  if (_mountAction != nullptr) {
    result = _mountAction->Update();
    // If the action fails and the robot has already turned toward
    // the charger, then position for a retry
    if ((result != ActionResult::SUCCESS) &&
        (result != ActionResult::RUNNING)) {
      bool isFacingAwayFromCharger = true;
      const auto* charger = GetRobot().GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
      if (charger != nullptr) {
        const auto& chargerAngle = charger->GetPose().GetRotation().GetAngleAroundZaxis();
        const auto& robotAngle = GetRobot().GetPose().GetRotation().GetAngleAroundZaxis();
        isFacingAwayFromCharger = (chargerAngle - robotAngle).getAbsoluteVal().ToFloat() > M_PI_2_F;
      }

      if (isFacingAwayFromCharger) {
        PRINT_NAMED_WARNING("MountChargerAction.CheckIfDone.PositionForRetry",
                            "Turning and mounting the charger failed (action result = %s). Driving forward to position for a retry.",
                            EnumToString(result));
        // Finished with turnAndMountAction
        _mountAction.reset();
        // We need to add the driveForRetryAction:
        result = ConfigureDriveForRetryAction();
        if (result != ActionResult::SUCCESS) {
          return result;
        }
      }
    }
  }
  
  // Tick the _driveForRetryAction action (if needed):
  if (_driveForRetryAction != nullptr) {
    result = _driveForRetryAction->Update();
    
    // If the action finished successfully, this parent action
    // should return a NOT_ON_CHARGER to cause a retry.
    if (result == ActionResult::SUCCESS) {
      return ActionResult::NOT_ON_CHARGER;
    }
  }
  
  return result;
}


ActionResult MountChargerAction::ConfigureMountAction()
{
  DEV_ASSERT(_mountAction == nullptr, "MountChargerAction.ConfigureMountAction.AlreadyConfigured");
  _mountAction.reset(new CompoundActionSequential());
  _mountAction->ShouldSuppressTrackLocking(true);
  _mountAction->SetRobot(&GetRobot());
  
  // Raise lift slightly so it doesn't drag against the ground (if necessary)
  const float backingUpLiftHeight_mm = 45.f;
  if (GetRobot().GetLiftHeight() < backingUpLiftHeight_mm) {
    _mountAction->AddAction(new MoveLiftToHeightAction(backingUpLiftHeight_mm));
  }
  
  // Play the driving Start anim if necessary
  if (_playDrivingAnimation) {
    _mountAction->AddAction(new WaitForLambdaAction([](Robot& robot) {
      robot.GetDrivingAnimationHandler().PlayStartAnim();
      return true;
    }));
  }
  
  // Back up into the charger
  _mountAction->AddAction(new BackupOntoChargerAction(_chargerID,
                                                      _useCliffSensorCorrection));
  
  // Play the driving End anim if necessary
  if (_playDrivingAnimation) {
    _mountAction->AddAction(new WaitForLambdaAction([](Robot& robot) {
      if (robot.GetDrivingAnimationHandler().HasFinishedEndAnim()) {
        return true;
      }
      if (!robot.GetDrivingAnimationHandler().IsPlayingEndAnim()) {
        robot.GetDrivingAnimationHandler().PlayEndAnim();
      }
      return false;
    }));
  }
  
  return ActionResult::SUCCESS;
}
  
ActionResult MountChargerAction::ConfigureDriveForRetryAction()
{
  DEV_ASSERT(_driveForRetryAction == nullptr, "MountChargerAction.ConfigureDriveForRetryAction.AlreadyConfigured");
  const float distanceToDriveForward_mm = 120.f;
  const float driveForwardSpeed_mmps = 100.f;
  _driveForRetryAction.reset(new DriveStraightAction(distanceToDriveForward_mm,
                                                     driveForwardSpeed_mmps,
                                                     false));
  _driveForRetryAction->ShouldSuppressTrackLocking(true);
  _driveForRetryAction->SetRobot(&GetRobot());

  return ActionResult::SUCCESS;
}

#pragma mark ---- TurnToAlignWithChargerAction ----
  
TurnToAlignWithChargerAction::TurnToAlignWithChargerAction(ObjectID chargerID,
                                                           AnimationTrigger leftTurnAnimTrigger,
                                                           AnimationTrigger rightTurnAnimTrigger)
  : IAction("TurnToAlignWithCharger",
            RobotActionType::TURN_TO_ALIGN_WITH_CHARGER,
            (u8)AnimTrackFlag::BODY_TRACK)
  , _chargerID(chargerID)
  , _leftTurnAnimTrigger(leftTurnAnimTrigger)
  , _rightTurnAnimTrigger(rightTurnAnimTrigger)
{
}

void TurnToAlignWithChargerAction::GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const
{
  requests.insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low });
}

ActionResult TurnToAlignWithChargerAction::Init()
{
  _compoundAction.reset(new CompoundActionParallel());
  _compoundAction->ShouldSuppressTrackLocking(true);
  _compoundAction->SetRobot(&GetRobot());
  
  const auto* charger = GetRobot().GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
  if ((charger == nullptr) ||
      (charger->GetType() != ObjectType::Charger_Basic)) {
    PRINT_NAMED_WARNING("TurnToAlignWithChargerAction.Init.InvalidCharger",
                        "No charger object with ID %d in block world!",
                        _chargerID.GetValue());
    return ActionResult::BAD_OBJECT;
  }
  // Compute the angle to turn.
  //
  // The charger's origin is at the 'front' edge of the ramp (furthest from the marker).
  // This value is the distance from the origin into the charger of the point that the
  // robot should angle towards. Setting this distance to 0 means the robot will angle
  // itself toward the charger origin.
  const float distanceIntoChargerToAimFor_mm = 30.f;
  Pose3d poseToAngleToward(0.f, Z_AXIS_3D(),
                           {distanceIntoChargerToAimFor_mm, 0.f, 0.f});
  poseToAngleToward.PreComposeWith(charger->GetPose());
  poseToAngleToward.SetParent(GetRobot().GetWorldOrigin());
  
  const auto targetToRobotVec = ComputeVectorBetween(GetRobot().GetDriveCenterPose(), poseToAngleToward);
  const float angleToTurnTo = atan2f(targetToRobotVec.y(), targetToRobotVec.x());
  
  auto* turnAction = new TurnInPlaceAction(angleToTurnTo, true);
  turnAction->SetMaxSpeed(DEG_TO_RAD(100.f));
  turnAction->SetAccel(DEG_TO_RAD(300.f));
  
  _compoundAction->AddAction(turnAction);
  
  // Play the left/right turn animation depending on the anticipated direction of the turn
  const auto& robotAngle = GetRobot().GetPose().GetRotation().GetAngleAroundZaxis();
  const bool clockwise = (angleToTurnTo - robotAngle).ToFloat() < 0.f;
  
  const auto animationTrigger = clockwise ? _rightTurnAnimTrigger : _leftTurnAnimTrigger;
  
  if (animationTrigger != AnimationTrigger::Count) {
    _compoundAction->AddAction(new TriggerAnimationAction(animationTrigger));
  }
  
  // Go ahead and do the first Update for the compound action so we don't
  // "waste" the first CheckIfDone call doing so. Proceed so long as this
  // first update doesn't _fail_
  const auto& compoundResult = _compoundAction->Update();
  if((ActionResult::SUCCESS == compoundResult) ||
     (ActionResult::RUNNING == compoundResult)) {
    return ActionResult::SUCCESS;
  } else {
    return compoundResult;
  }
}
  
  
ActionResult TurnToAlignWithChargerAction::CheckIfDone()
{
  DEV_ASSERT(_compoundAction != nullptr, "TurnToAlignWithChargerAction.CheckIfDone.NullCompoundAction");
  return _compoundAction->Update();
}


#pragma mark ---- BackupOntoChargerAction ----

BackupOntoChargerAction::BackupOntoChargerAction(ObjectID chargerID,
                                                 bool useCliffSensorCorrection)
  : IDockAction(chargerID,
                "BackupOntoCharger",
                RobotActionType::BACKUP_ONTO_CHARGER)
  , _useCliffSensorCorrection(useCliffSensorCorrection)
{
  // We don't expect to be near the pre-action pose of the charger when we
  // begin backing up onto it, so don't check for it. We aren't even seeing
  // the marker at this point anyway.
  SetDoNearPredockPoseCheck(false);
  
  // Don't turn toward the object since we're expected to be facing away from it
  SetShouldFirstTurnTowardsObject(false);
  SetShouldCheckForObjectOnTopOf(false);
}

  
ActionResult BackupOntoChargerAction::SelectDockAction(ActionableObject* object)
{
  auto objType = object->GetType();
  if (objType != ObjectType::Charger_Basic) {
    PRINT_NAMED_ERROR("BackupOntoChargerAction.SelectDockAction.NotChargerObject",
                      "Object is not a charger! It's a %s.", EnumToString(objType));
    return ActionResult::BAD_OBJECT;
  }
  
  _dockAction = _useCliffSensorCorrection ?
                  DockAction::DA_BACKUP_ONTO_CHARGER_USE_CLIFF :
                  DockAction::DA_BACKUP_ONTO_CHARGER;
  
  
  // Tell robot which charger it will be using
  GetRobot().SetCharger(_dockObjectID);

  return ActionResult::SUCCESS;
}
  
  
ActionResult BackupOntoChargerAction::Verify()
{
  // Verify that robot is on charger
  if (GetRobot().GetBatteryComponent().IsOnChargerContacts()) {
    PRINT_CH_INFO("Actions", "BackupOntoChargerAction.Verify.MountingChargerComplete",
                "Robot has mounted charger.");
    return ActionResult::SUCCESS;
  }
  
  return ActionResult::ABORT;
}
  
  
#pragma mark ---- DriveToAndMountChargerAction ----
  
DriveToAndMountChargerAction::DriveToAndMountChargerAction(const ObjectID& objectID,
                                                           const bool useCliffSensorCorrection)
: CompoundActionSequential()
{
  // Get DriveToObjectAction
  auto driveToAction = new DriveToObjectAction(objectID,
                                               PreActionPose::ActionType::DOCKING,
                                               0,
                                               false,
                                               0);
  driveToAction->SetPreActionPoseAngleTolerance(DEG_TO_RAD(15.f));
  AddAction(driveToAction);
  AddAction(new TurnToAlignWithChargerAction(objectID));
  AddAction(new MountChargerAction(objectID, useCliffSensorCorrection));
}
  
  

} // namespace Cozmo
} // namespace Anki

