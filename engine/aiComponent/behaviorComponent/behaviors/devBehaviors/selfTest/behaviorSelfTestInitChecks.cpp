/**
 * File: behaviorSelfTestInitChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestInitChecks.h"

#include "engine/actions/basicActions.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

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

  DriveStraightAction* drive = new DriveStraightAction(-SelfTestConfig::kDriveBackwardsDist_mm,
                                                       SelfTestConfig::kDriveBackwardsSpeed_mmps,
                                                       false);

  CompoundActionParallel* action = new CompoundActionParallel();
  const bool ignoreFailure = true;
  std::weak_ptr<IActionRunner> drivePtr = action->AddAction(drive, ignoreFailure);

  WaitAction* wait = new WaitAction(1.f);
  WaitForLambdaAction* cancel = new WaitForLambdaAction([drivePtr](Robot& robot){
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
  if(robot.GetCliffSensorComponent().IsCliffDetectedStatusBitOn())
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

  PRINT_NAMED_WARNING("","%d %d %d", robot.IsOnCharger(), robot.IsCharging(), robot.IsBatteryDisconnected());

  // Make sure we are considered on the charger and charging (or not charging because battery is disconnected)
  if(robot.IsOnCharger())
  {
    if(robot.IsCharging() && robot.IsBatteryDisconnected())
    {
      SELFTEST_SET_RESULT(SelfTestResultCode::ON_CHARGER_NOT_CHARGING_OR_DISCONNECTED);
    }
    else if(!robot.IsCharging() && !robot.IsBatteryDisconnected())
    {
      SELFTEST_SET_RESULT(SelfTestResultCode::BATTERY_TOO_LOW);
    }
  }
  else
  {
    SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_UNDETECTED);
  }

  // Charger voltage should be nice and high
  // Battery voltage will be checked later once we are off the charger in case the battery is currently disconnected
  if(robot.GetChargerVoltage() < SelfTestConfig::kMinChargerVoltage)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.ChargerTooLow",
                        "%fv",
                        robot.GetChargerVoltage());
    SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_VOLTAGE_TOO_LOW);
  }

  // Force delocalize the robot to ensure consistent starting pose
  robot.Delocalize(false);

  SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
}

}
}
