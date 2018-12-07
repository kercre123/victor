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
#include "engine/actions/basicActions.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestScreenAndBackpack::BehaviorSelfTestScreenAndBackpack(const Json::Value& config)
  : IBehaviorSelfTest(config, SelfTestResultCode::SCREEN_BACKPACK_TIMEOUT)
{
  SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>
                  {ExternalInterface::MessageEngineToGameTag::ChargerEvent});
}

Result BehaviorSelfTestScreenAndBackpack::OnBehaviorActivatedInternal()
{
  DriveStraightAction* action = new DriveStraightAction(SelfTestConfig::kDistToDriveOffCharger_mm,
                                                        SelfTestConfig::kDriveOffChargerSpeed_mmps, false);

  DelegateIfInControl(action, [this](){ TransitionToButtonCheck(); });

  return RESULT_OK;
}

void BehaviorSelfTestScreenAndBackpack::TransitionToButtonCheck()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool onCharger = robot.IsOnChargerPlatform();
  if(onCharger)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestScreenAndBackpack.TransitionToButtonCheck.StillOnCharger","");
    SELFTEST_SET_RESULT(SelfTestResultCode::STILL_ON_CHARGER);
  }

  DrawTextOnScreen(robot,
                   {"Press button if screen","and lights match"},
                   SelfTestConfig::kTextColor,
                   SelfTestConfig::kColorCheck);

  static const BackpackLights lights = {
      .onColors               = {{SelfTestConfig::kColorCheck, SelfTestConfig::kColorCheck, SelfTestConfig::kColorCheck}},
      .offColors              = {{SelfTestConfig::kColorCheck, SelfTestConfig::kColorCheck, SelfTestConfig::kColorCheck}},
      .onPeriod_ms            = {{500,500,500}},
      .offPeriod_ms           = {{500,500,500}},
      .transitionOnPeriod_ms  = {{0,0,0}},
      .transitionOffPeriod_ms = {{0,0,0}},
      .offset                 = {{0,0,0}}
  };

  robot.GetBodyLightComponent().SetBackpackLights(lights);

  _buttonPressed = robot.IsPowerButtonPressed();
  _buttonStartedPressed = _buttonPressed;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestScreenAndBackpack::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool onCharger = robot.IsOnChargerPlatform();
  if(onCharger || IsControlDelegated())
  {
    return SelfTestStatus::Running;
  }

  const bool buttonPressed = robot.IsPowerButtonPressed();
  const bool buttonReleased = _buttonPressed && !buttonPressed;

  if(_buttonStartedPressed && !buttonPressed)
  {
    _buttonStartedPressed = false;
  }

  if(buttonReleased && !_buttonStartedPressed)
  {
    DriveStraightAction* drive = new DriveStraightAction(SelfTestConfig::kDistToDriveOnCharger_mm,
                                                         SelfTestConfig::kDriveBackwardsSpeed_mmps);

    CompoundActionParallel* action = new CompoundActionParallel();
    const bool ignoreFailure = true;
    std::weak_ptr<IActionRunner> drivePtr = action->AddAction(drive, ignoreFailure);

    WaitForLambdaAction* cancel = new WaitForLambdaAction([drivePtr](Robot& robot){
                                                            if(robot.IsOnChargerPlatform())
                                                            {
                                                              if(auto drive = drivePtr.lock())
                                                              {
                                                                drive->Cancel();
                                                              }
                                                              return true;
                                                            }
                                                            return false;
                                                          });
    action->AddAction(cancel);

    DelegateIfInControl(action, [this](){
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::SUCCESS, SelfTestStatus::Complete);
    });
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

  _buttonPressed = false;
  _buttonStartedPressed = false;
}

}
}
