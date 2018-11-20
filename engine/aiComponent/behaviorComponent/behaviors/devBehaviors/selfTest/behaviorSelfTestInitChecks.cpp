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

#include "engine/components/bodyLightComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestInitChecks::BehaviorSelfTestInitChecks(const Json::Value& config)
: IBehaviorSelfTest(config)
{

}

Result BehaviorSelfTestInitChecks::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot,
                   {"Beginning automated test"});

  // Should not be seeing any cliffs
  if(robot.GetCliffSensorComponent().IsCliffDetectedStatusBitOn())
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CLIFF_UNEXPECTED, RESULT_FAIL);
  }

  // Check that raw touch values are in expected range (the range assumes no touch)
  const u16 rawTouchValue = robot.GetTouchSensorComponent().GetLatestRawTouchValue();
  if(!Util::InRange(rawTouchValue,
      PlaypenConfig::kMinExpectedTouchValue,
      PlaypenConfig::kMaxExpectedTouchValue))
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.TouchOOR", 
                        "Min %u < Val %u < Max %u",
                        PlaypenConfig::kMinExpectedTouchValue,
                        rawTouchValue,
                        PlaypenConfig::kMaxExpectedTouchValue);
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::TOUCH_VALUES_OOR, RESULT_FAIL);
  }

  PRINT_NAMED_WARNING("","%d %d %d", robot.IsOnCharger(), robot.IsCharging(), robot.IsBatteryDisconnected());

  // Make sure we are considered on the charger and charging (or not charging because battery is disconnected)
  if(robot.IsOnCharger())
  {
    if(robot.IsCharging() && robot.IsBatteryDisconnected())
    {
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNDETECTED, RESULT_FAIL);
    }
    else if(!robot.IsCharging() && !robot.IsBatteryDisconnected())
    {
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNDETECTED, RESULT_FAIL);
    }
  }
  else
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNDETECTED, RESULT_FAIL);
  }
  
  // Charger voltage should be nice and high
  // Battery voltage will be checked later once we are off the charger in case the battery is currently disconnected
  if(robot.GetChargerVoltage() < 4.0)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.BatteryTooLow", "%fv", robot.GetBatteryVoltage());
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::BATTERY_TOO_LOW, RESULT_FAIL);
  }
  
  // Force delocalize the robot to ensure consistent starting pose
  robot.Delocalize(false);

  SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, RESULT_OK);
  
  return RESULT_OK;
}

}
}


