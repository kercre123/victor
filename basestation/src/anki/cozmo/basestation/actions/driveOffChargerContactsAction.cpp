/**
 * File: driveOffChargerContactsAction.cpp
 *
 * Author: Andrew Stein
 * Date:   9/13/2016
 *
 * Description: Implements an action to drive forward slightly, just enough to
 *              get off the charger contacts, primarily for use in the SDK, since
 *              motors are locked while on charger to prevent hardware damage.
 *
 *              NOTE: does nothing if robot is not currently on charger.
 *
 *              NOTE: *intentionally* does not lock body track since this is to be used
 *                 when everything is likely locked by SDK mode being on charger
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/actions/driveOffChargerContactsAction.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"

CONSOLE_VAR(f32, kDriveOffChargerContactsDist_mm,    "Actions", 10.f);
CONSOLE_VAR(f32, kDriveOffChargerContactsSpeed_mmps, "Actions", 20.f);

namespace Anki {
namespace Cozmo {
    
DriveOffChargerContactsAction::DriveOffChargerContactsAction(Robot& robot)
: DriveStraightAction(robot, kDriveOffChargerContactsDist_mm, kDriveOffChargerContactsSpeed_mmps, false)
{
  SetName("DriveOffChargerContacts");
  SetType(RobotActionType::DRIVE_OFF_CHARGER_CONTACTS);
  
  if(_robot.GetContext()->IsInSdkMode())
  {
    // Purposefully do not lock the body track even though the base class DriveStraightAction
    // normally would. This action is meant to be used in SDK mode when we are on the charger
    // and all tracks are locked, so it does not require tracks so that we can get off
    // the charger.
    SetTracksToLock((u8)AnimTrackFlag::NO_TRACKS);
  }
}

ActionResult DriveOffChargerContactsAction::Init()
{
  _startedOnCharger = _robot.IsOnCharger();
  if(_startedOnCharger)
  {
    return DriveStraightAction::Init();
  }
  else
  {
    return ActionResult::SUCCESS;
  }
}
  
ActionResult DriveOffChargerContactsAction::CheckIfDone()
{
  if(!_startedOnCharger)
  {
    // We weren't on the charger when we started, so nothing to do
    return ActionResult::SUCCESS;
  }
  
  ActionResult driveResult = DriveStraightAction::CheckIfDone();
  
  if(driveResult == ActionResult::RUNNING)
  {
    return driveResult;
  }
  
  // If drive straight has finished, check that we are no longer on the charger
  if(!_robot.IsOnCharger())
  {
    return ActionResult::SUCCESS;
  }
  else
  {
    PRINT_NAMED_WARNING("DriveOffChargerContactsAction.CheckIfDone.StillOnCharger", "");
    return ActionResult::FAILURE_RETRY;
  }
}
  
} // namespace Cozmo
} // namespace Anki
