/**
 * File: behaviorSelfTestPickup.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestPickup.h"

#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestPickup::BehaviorSelfTestPickup(const Json::Value& config)
: IBehaviorSelfTest(config)
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

  const bool isPickedUp = robot.IsPickedUp();
  //const bool isBeingHeld = robot.IsBeingHeld();

  if(isPickedUp/* && isBeingHeld*/)
  {
    const AccelData& accel = robot.GetHeadAccelData();
    if(accel.z <= -7000)
    {
      PRINT_NAMED_WARNING("","Robot is upside down");

      if(_upsideDownStartTime_ms == 0)
      {
        DrawTextOnScreen(robot,
                         {"Please Wait"},
                         NamedColors::WHITE,
                         NamedColors::BLACK,
                         180);
      }
      
      if(!_upsideDownTimerAdded)
      {
        _upsideDownTimerAdded = true;
        AddTimer(5000, [this]() {
                         SELFTEST_SET_RESULT(FactoryTestResultCode::TEST_TIMED_OUT);
                       });
      }

      const auto now = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if(_upsideDownStartTime_ms == 0)
      {
        _upsideDownStartTime_ms = now;
      }
      else if(now - _upsideDownStartTime_ms > SelfTestConfig::kTimeToBeUpsideDown_ms)
      {
        const auto& cliffSenseComp = robot.GetCliffSensorComponent();
        if(cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_FL) &&
           cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_FR) &&
           cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_BL) &&
           cliffSenseComp.IsCliffDetected(CliffSensor::CLIFF_BR))
        {
          const auto& cliffData = cliffSenseComp.GetCliffDataRaw();
          bool allCliffs = true;
          for(int c = 0; c < CliffSensorComponent::kNumCliffSensors; c++)
          {
            allCliffs |= (cliffData[c] < 50);
          }

          if(allCliffs)
          {
            SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS,
                                                SelfTestStatus::Complete);
          }
        }
      }
    }
    else
    {
      if(_upsideDownStartTime_ms != 0)
      {
        DrawTextOnScreen(robot,
                         {"Pickup Robot", "Turn Upside Down"},
                         NamedColors::WHITE,
                         NamedColors::BLACK,
                         0);

      }
      
      _upsideDownStartTime_ms = 0;
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
