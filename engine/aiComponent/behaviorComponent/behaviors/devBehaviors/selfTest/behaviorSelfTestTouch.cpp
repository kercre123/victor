/**
 * File: behaviorSelfTestTouch.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Waits for the touch sensor to be held for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestTouch.h"

#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Vector {

namespace {
const std::string kCountdownTimerName = "countdown";
}

BehaviorSelfTestTouch::BehaviorSelfTestTouch(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::TOUCH_PRESS_TIMEOUT)
{
  SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>
                  {ExternalInterface::MessageEngineToGameTag::ChargerEvent,
                   ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged});
}

Result BehaviorSelfTestTouch::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // If touch sensor is already calibrated
  _calibrated = (robot.GetTouchSensorComponent().IsCalibrated() &&
                 !robot.GetTouchSensorComponent().IsChargerModeCheckRunning());

  if(_calibrated)
  {
    DrawTextOnScreen(robot, {"Hold Touch Sensor"});
  }
  else
  {
    DrawTextOnScreen(robot, {"Please Wait"});
  }
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestTouch::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  
  const bool isCalibrated = (robot.GetTouchSensorComponent().IsCalibrated() &&
                             !robot.GetTouchSensorComponent().IsChargerModeCheckRunning());
  if(!isCalibrated && _calibrated)
  {
    DrawTextOnScreen(robot, {"Please Wait"});
  }
  else if(isCalibrated && !_calibrated)
  {
    // If the touch sensor just became calibrated then increase the base behavior timeout
    // by some amount of time in order to account for the time spent calibrating
    IncreaseTimeoutTimer(5000);
    DrawTextOnScreen(robot, {"Hold Touch Sensor"});
  }
  _calibrated = isCalibrated;

  if(!_calibrated)
  {
    return SelfTestStatus::Running;
  }

  const bool buttonPressed = robot.GetTouchSensorComponent().GetIsPressed();
  const bool buttonReleasedEvent = _buttonPressed && !buttonPressed;
  const bool buttonPressedEvent = !_buttonPressed && buttonPressed;

  // If it is time to add a countdown timer
  if(_addTimer)
  {
    _addTimer = false;

    // Draw updated countdown to screen
    DrawTextOnScreen(robot, {std::to_string(_heldCountDown)});
    
    AddTimer(1000,
             [this, &robot](){
               _heldCountDown--;

               // Touch sensor has been held long enough
               if(_heldCountDown <= 0)
               {
                 DrawTextOnScreen(robot, {"Release Touch Sensor"});
               }
               else
               {
                 _addTimer = true;
               }
             },
      kCountdownTimerName);
  }

  // Touch sensor has been held long enough
  if(_heldCountDown <= 0)
  {
    // Remove the countdown timer
    RemoveTimers(kCountdownTimerName);

    // Touch sensor has been released, test passed
    if(buttonReleasedEvent)
    {
      // Clear screen
      DrawTextOnScreen(robot, {""});
      
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::SUCCESS, SelfTestStatus::Complete);
    }
  }

  // Touch sensor has been held, start the countdown
  if(buttonPressedEvent)
  {
    DrawTextOnScreen(robot, {std::to_string(_heldCountDown)});
    
    AddTimer(1000,
             [this](){
               _heldCountDown--;
               _addTimer = true;
             },
             kCountdownTimerName);
  }
  // Touch sensor was released, hasn't been held long enough
  else if(buttonReleasedEvent)
  {
    // Remove countdown timers
    RemoveTimers(kCountdownTimerName);
    
    DrawTextOnScreen(robot, {"Hold Touch Sensor"});

    _addTimer = false;

    // Reset countdown count
    _heldCountDown = SelfTestConfig::kTouchSensorDuration_sec;
  }

  _buttonPressed = buttonPressed;
  
  return SelfTestStatus::Running;
}

void BehaviorSelfTestTouch::OnBehaviorDeactivated()
{
  _buttonPressed = false;
  _addTimer = false;
  _heldCountDown = SelfTestConfig::kTouchSensorDuration_sec;
}

}
}


