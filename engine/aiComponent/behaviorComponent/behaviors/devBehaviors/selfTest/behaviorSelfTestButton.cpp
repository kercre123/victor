/**
 * File: behaviorSelfTestButton.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestButton.h"

#include "engine/components/animationComponent.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestButton::BehaviorSelfTestButton(const Json::Value& config)
: IBehaviorSelfTest(config)
{

}

Result BehaviorSelfTestButton::OnBehaviorActivatedInternal()
{
  PRINT_NAMED_WARNING("","BUTTON TEST");
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot, {"Press Button"});

  _buttonPressed = robot.IsPowerButtonPressed();
  _buttonStartedPressed = _buttonPressed;
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestButton::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool buttonPressed = robot.IsPowerButtonPressed();

  if(_buttonStartedPressed && !buttonPressed)
  {
    _buttonStartedPressed = false;
  }

  const bool onCharger = (robot.IsOnCharger() || robot.IsCharging());

  const bool buttonReleased = _buttonPressed && !buttonPressed;
  if(buttonReleased && !_buttonStartedPressed)
  {
    if(onCharger)
    {
      DrawTextOnScreen(robot, {"Please Wait"});

      AddTimer(500, [this](){
                       WaitToBeOnTreads();
                     });
    }
    else
    {
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNDETECTED, SelfTestStatus::Failure);
    }
  }

  _buttonPressed = buttonPressed;
  
  return SelfTestStatus::Running;
}

void BehaviorSelfTestButton::OnBehaviorDeactivated()
{
  _buttonPressed = false;
  _buttonStartedPressed = false;
}

void BehaviorSelfTestButton::WaitToBeOnTreads()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(robot.GetOffTreadsState() == OffTreadsState::OnTreads)
  {
    SELFTEST_SET_RESULT(FactoryTestResultCode::SUCCESS);
  }
  else
  {
    AddTimer(500, [this](){
                     WaitToBeOnTreads();
                   });
  }
}

}
}


