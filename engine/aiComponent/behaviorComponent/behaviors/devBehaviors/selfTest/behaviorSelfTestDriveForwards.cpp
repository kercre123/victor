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
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestDriveForwards::BehaviorSelfTestDriveForwards(const Json::Value& config)
: IBehaviorSelfTest(config)
{

}

Result BehaviorSelfTestDriveForwards::OnBehaviorActivatedInternal()
{
  DriveStraightAction* action = new DriveStraightAction(SelfTestConfig::kDistanceToDriveForwards_mm,
                                                        SelfTestConfig::kDriveSpeed_mmps, false);

  DelegateIfInControl(action, [this](){ TransitionToOffChargerChecks(); });

  return RESULT_OK;
}

void BehaviorSelfTestDriveForwards::OnBehaviorDeactivated()
{

}

void BehaviorSelfTestDriveForwards::TransitionToOffChargerChecks()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(robot.GetBatteryVoltage() < SelfTestConfig::kMinBatteryVoltage)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.BatteryTooLow", "%fv", robot.GetBatteryVoltage());
    SELFTEST_SET_RESULT(FactoryTestResultCode::BATTERY_TOO_LOW);
  }

  // TODO Maybe check cliff sensors for no cliff here
  // Difficult because don't know what kind of surface we are on, may be a dark table

  SELFTEST_SET_RESULT(FactoryTestResultCode::SUCCESS);
}

}
}
