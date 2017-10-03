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

#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/charger.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
#pragma mark ---- MountChargerAction ----
  
MountChargerAction::MountChargerAction(Robot& robot,
                                       ObjectID chargerID,
                                       const bool useCliffSensorCorrection,
                                       const bool useManualSpeed)
  : IAction(robot,
            "MountCharger",
            RobotActionType::MOUNT_CHARGER,
            (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::LIFT_TRACK)
  , _chargerID(chargerID)
  , _useCliffSensorCorrection(useCliffSensorCorrection)
  , _useManualSpeed(useManualSpeed)
{
  
}
  
ActionResult MountChargerAction::Init()
{
  // Reset the compound actions to ensure they get re-configured:
  _alignWithChargerAction.reset();
  _turnAndMountAction.reset();
  _driveForRetryAction.reset();
  
  // Verify that we have a charger in the world that matches _chargerID
  const auto* charger = _robot.GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
  if ((charger == nullptr) ||
      (charger->GetType() != ObjectType::Charger_Basic)) {
    PRINT_NAMED_WARNING("MountChargerAction.Init.InvalidCharger",
                        "No charger object with ID %d in block world!",
                        _chargerID.GetValue());
    return ActionResult::BAD_OBJECT;
  }
  
  // Tell robot which charger it will be using
  _robot.SetCharger(_chargerID);

  // Set up the align with charger compound action
  ActionResult result = ConfigureAlignWithChargerAction();
  
  return result;
}
  
ActionResult MountChargerAction::CheckIfDone()
{
  auto result = ActionResult::RUNNING;
  
  // Tick the alignWithCharger action (if needed):
  if (_alignWithChargerAction != nullptr) {
    result = _alignWithChargerAction->Update();
    if (result == ActionResult::SUCCESS) {
      // Finished with alignWithChargerAction.
      // Null the action and keep running.
      _alignWithChargerAction.reset();
      // Configure the turnAndMount action:
      result = ConfigureTurnAndMountAction();
      if (result != ActionResult::SUCCESS) {
        return result;
      }
    }
  }
  
  // Tick the turnAndMount action (if needed):
  if (_turnAndMountAction != nullptr) {
    result = _turnAndMountAction->Update();
    // If the action fails and the robot has already turned toward
    // the charger, then position for a retry
    if ((result != ActionResult::SUCCESS) &&
        (result != ActionResult::RUNNING)) {
      bool isFacingAwayFromCharger = true;
      const auto* charger = _robot.GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
      if (charger != nullptr) {
        const auto& chargerAngle = charger->GetPose().GetRotation().GetAngleAroundZaxis();
        const auto& robotAngle = _robot.GetPose().GetRotation().GetAngleAroundZaxis();
        isFacingAwayFromCharger = (chargerAngle - robotAngle).getAbsoluteVal().ToFloat() > M_PI_2_F;
      }

      if (isFacingAwayFromCharger) {
        PRINT_NAMED_WARNING("MountChargerAction.CheckIfDone.PositionForRetry",
                            "Turning and mounting the charger failed (action result = %s). Driving forward to position for a retry.",
                            EnumToString(result));
        // Finished with turnAndMountAction
        _turnAndMountAction.reset();
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

ActionResult MountChargerAction::ConfigureAlignWithChargerAction()
{
  DEV_ASSERT(_alignWithChargerAction == nullptr, "MountChargerAction.ConfigureAlignWithChargerAction.AlreadyConfigured");
  _alignWithChargerAction.reset(new CompoundActionSequential(_robot));
  _alignWithChargerAction->ShouldSuppressTrackLocking(true);
  
  // Dock action to align with the charger marker
  const float distanceFromMarker_mm = 130.f;
  const float alignSpeed_mmps = 30.f;
  auto alignAction = new AlignWithObjectAction(_robot,
                                               _chargerID,
                                               distanceFromMarker_mm,
                                               AlignmentType::CUSTOM,
                                               _useManualSpeed);
  alignAction->SetSpeed(alignSpeed_mmps);
  _alignWithChargerAction->AddAction(alignAction);
  
  // Look straight to see marker clearly
  _alignWithChargerAction->AddAction(new MoveHeadToAngleAction(_robot, 0.f));
  
  return ActionResult::SUCCESS;
}
  
ActionResult MountChargerAction::ConfigureTurnAndMountAction()
{
  DEV_ASSERT(_turnAndMountAction == nullptr, "MountChargerAction.ConfigureTurnAndMountAction.AlreadyConfigured");
  _turnAndMountAction.reset(new CompoundActionSequential(_robot));
  _turnAndMountAction->ShouldSuppressTrackLocking(true);
  
  const auto* charger = _robot.GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
  if ((charger == nullptr) ||
      (charger->GetType() != ObjectType::Charger_Basic)) {
    PRINT_NAMED_WARNING("MountChargerAction.ConfigureTurnAndMountAction.InvalidCharger",
                        "No charger object with ID %d in block world!",
                        _chargerID.GetValue());
    return ActionResult::BAD_OBJECT;
  }
  
  // The charger's origin is at the 'front' edge of the ramp (furthest from the marker).
  // This value is the distance from the origin into the charger of the point that the
  // robot should angle towards. Setting this distance to 0 means the robot will angle
  // itself toward the charger origin.
  const float distanceIntoChargerToAimFor_mm = 30.f;
  Pose3d poseToAngleToward(0.f, Z_AXIS_3D(),
                           {distanceIntoChargerToAimFor_mm, 0.f, 0.f});
  poseToAngleToward.PreComposeWith(charger->GetPose());
  poseToAngleToward.SetParent(_robot.GetWorldOrigin());
  
  const auto targetToRobotVec = ComputeVectorBetween(_robot.GetDriveCenterPose(), poseToAngleToward);
  const float angleToTurnTo = atan2f(targetToRobotVec.y(), targetToRobotVec.x());

  auto turnAction = new TurnInPlaceAction(_robot,
                                          angleToTurnTo,
                                          true);
  turnAction->SetMaxSpeed(DEG_TO_RAD(100.f));
  turnAction->SetAccel(DEG_TO_RAD(300.f));
  
  _turnAndMountAction->AddAction(turnAction);
  
  // Raise lift slightly so it doesn't drag against the ground (if necessary)
  const float backingUpLiftHeight_mm = 45.f;
  if (_robot.GetLiftHeight() < backingUpLiftHeight_mm) {
    _turnAndMountAction->AddAction(new MoveLiftToHeightAction(_robot, backingUpLiftHeight_mm));
  }
  
  // Finally, actually back up into the charger
  _turnAndMountAction->AddAction(new BackupOntoChargerAction(_robot,
                                                             _chargerID,
                                                             _useCliffSensorCorrection));
  
  // Lower the lift back to the ground
  _turnAndMountAction->AddAction(new MoveLiftToHeightAction(_robot, LIFT_HEIGHT_LOWDOCK));
  
  return ActionResult::SUCCESS;
}
  
ActionResult MountChargerAction::ConfigureDriveForRetryAction()
{
  DEV_ASSERT(_driveForRetryAction == nullptr, "MountChargerAction.ConfigureDriveForRetryAction.AlreadyConfigured");
  const float distanceToDriveForward_mm = 120.f;
  const float driveForwardSpeed_mmps = 100.f;
  _driveForRetryAction.reset(new DriveStraightAction(_robot,
                                                     distanceToDriveForward_mm,
                                                     driveForwardSpeed_mmps,
                                                     false));
  _driveForRetryAction->ShouldSuppressTrackLocking(true);

  return ActionResult::SUCCESS;
}

#pragma mark ---- BackupOntoChargerAction ----

BackupOntoChargerAction::BackupOntoChargerAction(Robot& robot,
                                                 ObjectID chargerID,
                                                 bool useCliffSensorCorrection)
  : IDockAction(robot,
                chargerID,
                "BackupOntoCharger",
                RobotActionType::BACKUP_ONTO_CHARGER,
                false)
  , _useCliffSensorCorrection(useCliffSensorCorrection)
{
  // TODO: Charger marker pose still oscillates so just do your best from where you are
  //       rather than oscillating between jumpy predock poses.
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
  _robot.SetCharger(_dockObjectID);
  
  return ActionResult::SUCCESS;
}
  
  
ActionResult BackupOntoChargerAction::Verify()
{
  // Verify that robot is on charger
  if (_robot.IsOnCharger()) {
    PRINT_CH_INFO("Actions", "BackupOntoChargerAction.Verify.MountingChargerComplete",
                "Robot has mounted charger.");
    return ActionResult::SUCCESS;
  }
  
  return ActionResult::ABORT;
}
  
  
#pragma mark ---- DriveToAndMountChargerAction ----
  
DriveToAndMountChargerAction::DriveToAndMountChargerAction(Robot& robot,
                                                           const ObjectID& objectID,
                                                           const bool useCliffSensorCorrection,
                                                           const bool useManualSpeed)
: CompoundActionSequential(robot)
{
  // Get DriveToObjectAction
  auto driveToAction = new DriveToObjectAction(robot,
                                               objectID,
                                               PreActionPose::ActionType::DOCKING,
                                               0,
                                               false,
                                               0,
                                               useManualSpeed);
  AddAction(driveToAction);
  AddAction(new MountChargerAction(robot, objectID, useCliffSensorCorrection, useManualSpeed));
}
  
  

} // namespace Cozmo
} // namespace Anki

