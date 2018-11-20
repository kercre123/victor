/**
 * File: behaviorSelfTestScreenAndBackpack.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestScreenAndBackpack.h"

#include "engine/components/bodyLightComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestScreenAndBackpack::BehaviorSelfTestScreenAndBackpack(const Json::Value& config)
: IBehaviorSelfTest(config)
{

}

Result BehaviorSelfTestScreenAndBackpack::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot,
                   {"Press button if screen","and lights match"},
                   NamedColors::BLACK,
                   NamedColors::CYAN);

  static const BackpackLights lights = {
      .onColors               = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
      .offColors              = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
      .onPeriod_ms            = {{500,500,500}},
      .offPeriod_ms           = {{500,500,500}},
      .transitionOnPeriod_ms  = {{0,0,0}},
      .transitionOffPeriod_ms = {{0,0,0}},
      .offset                 = {{0,0,0}}
  };
    
  robot.GetBodyLightComponent().SetBackpackLights(lights);

  
  AddTimer(30000, [this](){
    SELFTEST_SET_RESULT(FactoryTestResultCode::TEST_TIMED_OUT);
  });
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestScreenAndBackpack::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool buttonPressed = robot.IsPowerButtonPressed();
  const bool buttonReleased = _buttonPressed && !buttonPressed;
  
  if(buttonReleased)
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, SelfTestStatus::Complete);
  }

  _buttonPressed = buttonPressed;

  return SelfTestStatus::Running;
}

void BehaviorSelfTestScreenAndBackpack::OnBehaviorDeactivated()
{
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  robot.GetBodyLightComponent().SetBackpackLights(BodyLightComponent::GetOffBackpackLights());
}

}
}


