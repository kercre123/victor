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

#include "engine/actions/driveOffChargerContactsAction.h"
#include "engine/components/batteryComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "util/console/consoleInterface.h"

CONSOLE_VAR(f32, kDriveOffChargerContactsDist_mm,    "Actions", 10.f);
CONSOLE_VAR(f32, kDriveOffChargerContactsSpeed_mmps, "Actions", 20.f);

namespace Anki {
namespace Cozmo {
    
DriveOffChargerContactsAction::DriveOffChargerContactsAction()
: DriveStraightAction(kDriveOffChargerContactsDist_mm, kDriveOffChargerContactsSpeed_mmps, false)
{
  SetName("DriveOffChargerContacts");
  SetType(RobotActionType::DRIVE_OFF_CHARGER_CONTACTS);
  
  if(GetRobot().GetContext()->IsInSdkMode())
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
  _startedOnCharger = GetRobot().GetBatteryComponent().IsOnChargerContacts();
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
  if(!GetRobot().GetBatteryComponent().IsOnChargerContacts())
  {
    return ActionResult::SUCCESS;
  }
  else
  {
    PRINT_NAMED_WARNING("DriveOffChargerContactsAction.CheckIfDone.StillOnCharger", "");
    return ActionResult::STILL_ON_CHARGER;
  }
}
  
} // namespace Cozmo
} // namespace Anki
