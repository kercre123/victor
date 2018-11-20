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

}

Result BehaviorSelfTestTouch::OnBehaviorActivatedInternal()
{
  PRINT_NAMED_WARNING("","TOUCH TEST");
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot, {"Hold Touch"});
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestTouch::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

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
                 DrawTextOnScreen(robot, {"Release Touch"});
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
    DrawTextOnScreen(robot, {"Hold Touch"});
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


