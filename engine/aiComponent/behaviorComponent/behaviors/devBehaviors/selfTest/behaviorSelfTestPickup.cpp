/**
 * File: behaviorSelfTestPickup.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Prompts user to pickup robot to check cliff sensor while upside down
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestPickup.h"

#include "engine/components/battery/batteryComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace {
  const char* const kTimerName = "upsideDown";
}

namespace Anki {
namespace Vector {

BehaviorSelfTestPickup::BehaviorSelfTestPickup(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::PICKUP_ROBOT_TIMEOUT)
{
  SubscribeToTags({ExternalInterface::MessageEngineToGameTag::ChargerEvent,
                   ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged,
                   ExternalInterface::MessageEngineToGameTag::CliffEvent});
}

Result BehaviorSelfTestPickup::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot,
                   {"Pickup Robot", "Turn Upside Down"});

  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestPickup::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool isBeingHeld = robot.IsBeingHeld();

  if(isBeingHeld)
  {
    const AccelData& accel = robot.GetHeadAccelData();
    if(accel.z <= SelfTestConfig::kUpsideDownZAccel)
    {
      if(!_upsideDownTimerAdded)
      {
        _upsideDownTimerAdded = true;

        // Once turned upside down must complete rest of checks within this time
        AddTimer(SelfTestConfig::kUpsideDownTimeout_ms,
                 [this]() {
                   SELFTEST_SET_RESULT(SelfTestResultCode::UPSIDE_DOWN_TIMEOUT);
                 },
                 kTimerName);
      }

      const auto now = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if(_upsideDownStartTime_ms == 0)
      {
        _upsideDownStartTime_ms = now;

        DrawTextOnScreen(robot,
                         {"Please Wait"},
                         NamedColors::WHITE,
                         NamedColors::BLACK,
                         180);
      }
      // Have been upside down for long enough, check cliff sensors
      else if(now - _upsideDownStartTime_ms > SelfTestConfig::kTimeToBeUpsideDown_ms)
      {
        const auto& cliffSenseComp = robot.GetCliffSensorComponent();
        if(cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_FL) &&
           cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_FR) &&
           cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_BL) &&
           cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_BR))
        {
          // All cliff sensors should be seeing cliffs and have values below a threshold
          const auto& cliffData = cliffSenseComp.GetCliffDataRaw();
          bool allCliffs = true;
          for(int c = 0; c < CliffSensorComponent::kNumCliffSensors; c++)
          {
            allCliffs |= (cliffData[c] < SelfTestConfig::kUpsideDownCliffValThresh); 
          }

          if(allCliffs)
          {
            SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::SUCCESS,
                                                SelfTestStatus::Complete);
          }
        }        
      }      
    }
    else
    {
      // No longer upside down
      if(_upsideDownStartTime_ms != 0)
      {
        DrawTextOnScreen(robot,
                         {"Pickup Robot", "Turn Upside Down"},
                         NamedColors::WHITE,
                         NamedColors::BLACK,
                         0);

      }
      
      _upsideDownStartTime_ms = 0;

      // Remove upside timer
      _upsideDownTimerAdded = false;
      RemoveTimer(kTimerName);
    }
  }
  
  return SelfTestStatus::Running;
}

void BehaviorSelfTestPickup::OnBehaviorDeactivated()
{
  _upsideDownTimerAdded = false;
  _upsideDownStartTime_ms = 0;
}

}
}


