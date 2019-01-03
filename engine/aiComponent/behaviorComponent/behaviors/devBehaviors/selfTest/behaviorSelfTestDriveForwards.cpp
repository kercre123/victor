/**
 * File: behaviorSelfTestDriveForwards.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestDriveForwards.h"

#include "engine/actions/basicActions.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestDriveForwards::BehaviorSelfTestDriveForwards(const Json::Value& config)
  : IBehaviorSelfTest(config, SelfTestResultCode::DRIVE_FORWARDS_TIMEOUT)
{

}

Result BehaviorSelfTestDriveForwards::OnBehaviorActivatedInternal()
{
  const bool shouldPlayAnimation = false;
  DriveStraightAction* action = new DriveStraightAction(SelfTestConfig::kDistanceToDriveForwards_mm,
                                                        SelfTestConfig::kDriveSpeed_mmps,
                                                        shouldPlayAnimation);
  action->SetCanMoveOnCharger(true);

  DelegateIfInControl(action, [this](){ TransitionToOffChargerChecks(); });
    
  return RESULT_OK;
}

void BehaviorSelfTestDriveForwards::TransitionToOffChargerChecks()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool onCharger = robot.GetBatteryComponent().IsOnChargerContacts();
  if(onCharger)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDriveForwards.TransitionToOffChargerChecks.StillOnCharger","");
    SELFTEST_SET_RESULT(SelfTestResultCode::STILL_ON_CHARGER);
  }
  
  const float batteryVolts = robot.GetBatteryComponent().GetBatteryVoltsRaw();
  if(batteryVolts < SelfTestConfig::kMinBatteryVoltage)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.BatteryTooLow", "%fv", batteryVolts);
    SELFTEST_SET_RESULT(SelfTestResultCode::BATTERY_TOO_LOW);
  }

  // TODO Maybe check cliff sensors for no cliff here
  // Difficult because don't know what kind of surface we are on, may be a dark table
  
  SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
}

}
}


