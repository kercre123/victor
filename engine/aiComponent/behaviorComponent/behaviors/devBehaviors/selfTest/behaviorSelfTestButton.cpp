/**
 * File: behaviorSelfTestButton.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Waits for the backpack button to be pressed
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestButton.h"

#include "engine/components/animationComponent.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestButton::BehaviorSelfTestButton(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::BUTTON_PRESS_TIMEOUT)
{
  // Allow offtreads state to change while this behavior is running
  // We sometimes detect pickup/InAir when the button is pressed while on the charger due to
  // bouncy charge contacts
  SubscribeToTags({EngineToGameTag::RobotOffTreadsStateChanged});
}

Result BehaviorSelfTestButton::OnBehaviorActivatedInternal()
{
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

  const bool buttonReleased = _buttonPressed && !buttonPressed;
  if(buttonReleased && !_buttonStartedPressed)
  {
    const bool onCharger = robot.GetBatteryComponent().IsOnChargerContacts();
    if(onCharger)
    {
      // Once the button has been pressed, wait until we have stabilized
      // and think we are back on treads as the charger contacts can be quite bouncy
      DrawTextOnScreen(robot, {"Please Wait"});

      WaitToBeOnTreads();
    }
    else
    {
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::CHARGER_UNDETECTED, SelfTestStatus::Failure);
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
    SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
  }
  else
  {
    // Repeatedly check for OnTreads every so often until we timeout
    AddTimer(500, [this](){
                     WaitToBeOnTreads();
                   });
  }
}

}
}


