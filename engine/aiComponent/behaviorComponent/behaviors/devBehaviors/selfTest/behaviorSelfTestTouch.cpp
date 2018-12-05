/**
 * File: behaviorSelfTestTouch.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestTouch.h"

#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestTouch::BehaviorSelfTestTouch(const Json::Value& config)
: IBehaviorSelfTest(config)
{
  SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>
                  {ExternalInterface::MessageEngineToGameTag::ChargerEvent,
                   ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged});
}

Result BehaviorSelfTestTouch::OnBehaviorActivatedInternal()
{
  PRINT_NAMED_WARNING("","TOUCH TEST");
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  _calibrated = (robot.GetTouchSensorComponent().IsCalibrated());/* &&
                 !robot.GetTouchSensorComponent().IsChargerModeCheckRunning());*/

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

  const bool isCalibrated = (robot.GetTouchSensorComponent().IsCalibrated());/* &&
                             !robot.GetTouchSensorComponent().IsChargerModeCheckRunning());*/
  if(!isCalibrated && _calibrated)
  {
    DrawTextOnScreen(robot, {"Please Wait"});
  }
  else if(isCalibrated && !_calibrated)
  {
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

  if(_addTimer)
  {
    _addTimer = false;

    DrawTextOnScreen(robot, {std::to_string(_heldCountDown)});

    AddTimer(1000,
             [this, &robot](){
               _heldCountDown--;

               if(_heldCountDown <= 0)
               {
                 DrawTextOnScreen(robot, {"Release Touch Sensor"});
               }
               else
               {
                 _addTimer = true;
               }
             },
      "countdown");
  }

  if(_heldCountDown <= 0)
  {
    RemoveTimers("countdown");

    if(buttonReleasedEvent)
    {
      DrawTextOnScreen(robot, {""});

      SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, SelfTestStatus::Complete);
    }
  }

  if(buttonPressedEvent)
  {
    DrawTextOnScreen(robot, {std::to_string(_heldCountDown)});

    AddTimer(1000,
             [this](){
               _heldCountDown--;
               _addTimer = true;
             },
      "countdown");
  }
  else if(buttonReleasedEvent)
  {
    RemoveTimers("countdown");
    DrawTextOnScreen(robot, {"Hold Touch Sensor"});
    _addTimer = false;
    _heldCountDown = 5;
  }

  _buttonPressed = buttonPressed;

  return SelfTestStatus::Running;
}

void BehaviorSelfTestTouch::OnBehaviorDeactivated()
{
  _buttonPressed = false;
  _addTimer = false;
  _heldCountDown = 5;
}

}
}
