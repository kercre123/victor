/**
 * File: behaviorPlaypenInitChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 08/09/17
 *
 * Description: Quick check of initial robot state for playpen. Checks things like firmware version,
 *              battery voltage, cliff sensors, etc
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenInitChecks.h"

#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenInitChecks::BehaviorPlaypenInitChecks(const Json::Value& config)
: IBehaviorPlaypen(config)
{

}

Result BehaviorPlaypenInitChecks::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Should not be seeing any cliffs
  if(robot.GetCliffSensorComponent().IsCliffDetectedStatusBitOn())
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CLIFF_UNEXPECTED, RESULT_FAIL);
  }

  // Check that raw touch values are in expected range (the range assumes no touch)
  const u16 rawTouchValue = robot.GetTouchSensorComponent().GetLatestRawTouchValue();
  if(!Util::InRange(rawTouchValue,
      PlaypenConfig::kMinExpectedTouchValue,
      PlaypenConfig::kMaxExpectedTouchValue))
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenWaitToStart.OnActivated.TouchOOR", 
                        "Min %u < Val %u < Max %u",
                        PlaypenConfig::kMinExpectedTouchValue,
                        rawTouchValue,
                        PlaypenConfig::kMaxExpectedTouchValue);
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::TOUCH_VALUES_OOR, RESULT_FAIL);
  }
  
  // Battery voltage should be relatively high as we are on the charger
  if(robot.GetBatteryVoltage() < PlaypenConfig::kMinBatteryVoltage)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenInitChecks.OnActivated.BatteryTooLow", "%fv", robot.GetBatteryVoltage());
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::BATTERY_TOO_LOW, RESULT_FAIL);
  }
  
  // Make sure we are considered on the charger and charging
  if(!(robot.IsOnCharger() && robot.IsCharging()))
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNDETECTED, RESULT_FAIL);
  }
  
  // Erase all of playpen/factory related nvstorage
  if(!robot.GetNVStorageComponent().WipeFactory())
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::NVSTORAGE_ERASE_FAILED, RESULT_FAIL);
  }
  
  // Force delocalize the robot to ensure consistent starting pose
  robot.Delocalize(false);

  PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, RESULT_OK);
  
  return RESULT_OK;
}

}
}


