/**
 * File: behaviorSelfTestInitChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Performs basic checks for the self test
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestInitChecks.h"

#include "engine/actions/basicActions.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestInitChecks::BehaviorSelfTestInitChecks(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::INIT_CHECKS_TIMEOUT)
{

}

Result BehaviorSelfTestInitChecks::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot,
                   {"Test Running"});

  // Drive backwards on the charger to help align cliff sensors so they are not
  // on the white stripe
  DriveStraightAction* drive = new DriveStraightAction(-SelfTestConfig::kDriveBackwardsDist_mm,
                                                       SelfTestConfig::kDriveBackwardsSpeed_mmps);
  drive->SetCanMoveOnCharger(true);

  // Driving backwards on the charger does not update the robot's position so the drive action
  // will run forever. Add in some wait actions to cancel the drive after some amount of time
  CompoundActionParallel* action = new CompoundActionParallel();
  const bool ignoreFailure = true;
  std::weak_ptr<IActionRunner> drivePtr = action->AddAction(drive, ignoreFailure);

  WaitAction* wait = new WaitAction(SelfTestConfig::kDriveBackwardsTime_sec);
  WaitForLambdaAction* cancel = new WaitForLambdaAction([drivePtr](Robot& robot)
    {
      if(auto drive = drivePtr.lock())
      {
        drive->Cancel();
      }
      return true;
    });
  CompoundActionSequential* seq = new CompoundActionSequential({wait, cancel});
  action->AddAction(seq);
  
  DelegateIfInControl(action, [this](){ TransitionToChecks(); });

  return RESULT_OK;
}

void BehaviorSelfTestInitChecks::TransitionToChecks()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Should not be seeing any cliffs
  if(robot.GetCliffSensorComponent().IsCliffDetected())
  {
    SELFTEST_SET_RESULT(SelfTestResultCode::CLIFF_UNEXPECTED);
  }

  // Check that raw touch values are in expected range (the range assumes no touch)
  const u16 rawTouchValue = robot.GetTouchSensorComponent().GetLatestRawTouchValue();
  if(!Util::InRange(rawTouchValue,
      SelfTestConfig::kMinExpectedTouchValue,
      SelfTestConfig::kMaxExpectedTouchValue))
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.TouchOOR", 
                        "Min %u < Val %u < Max %u",
                        SelfTestConfig::kMinExpectedTouchValue,
                        rawTouchValue,
                        SelfTestConfig::kMaxExpectedTouchValue);
    SELFTEST_SET_RESULT(SelfTestResultCode::TOUCH_VALUES_OOR);
  }

  const auto& battComp = robot.GetBatteryComponent();
  const bool onCharger = battComp.IsOnChargerContacts();
  const bool isCharging = battComp.IsCharging();
  const bool isDisconnected = battComp.IsBatteryDisconnectedFromCharger();

  // Make sure we are considered on the charger and charging (or not charging because battery is disconnected)
  if(onCharger)
  {
    if(isCharging && isDisconnected)
    {
      SELFTEST_SET_RESULT(SelfTestResultCode::CHARGING_BUT_DISCONNECTED);
    }
    else if(!isCharging && !isDisconnected)
    {
      SELFTEST_SET_RESULT(SelfTestResultCode::ON_CHARGER_NOT_CHARGING_OR_DISCONNECTED);
    }

    const bool battVoltGood = (battComp.GetBatteryVoltsRaw() > SelfTestConfig::kMinBatteryVoltage);
    if(!isDisconnected && !battVoltGood)
    {
      SELFTEST_SET_RESULT(SelfTestResultCode::BATTERY_TOO_LOW);
    }
  }
  else
  {
    SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_UNDETECTED);
  }
  
  const bool chargerVoltGood = (battComp.GetChargerVoltsRaw() > SelfTestConfig::kMinChargerVoltage);
  if(!chargerVoltGood)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.BatteryTooLow",
                        "%fv",
                        robot.GetBatteryComponent().GetChargerVoltsRaw());
    SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_VOLTAGE_TOO_LOW);
  }
  
  // Force delocalize the robot to ensure consistent starting pose
  robot.Delocalize(false);

  SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
}

}
}


