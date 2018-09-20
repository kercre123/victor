/**
 * File: behaviorPlaypenWaitToStart.cpp
 *
 * Author: Al Chaussee
 * Created: 10/12/17
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenWaitToStart.h"

#include "clad/types/backpackAnimationTriggers.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

BehaviorPlaypenWaitToStart::BehaviorPlaypenWaitToStart(const Json::Value& config)
: IBehaviorPlaypen(config)
{
  // Subscribe to all of the base class's failure tags to prevent us from automatically failing
  // if we get put on the charger, motor calibration on startup, etc...
  std::set<ExternalInterface::MessageEngineToGameTag> tags = GetFailureTags();
  SubscribeToTags(std::move(tags));
}

Result BehaviorPlaypenWaitToStart::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Remove the default playpen behavior timeout timer since this behavior will
  // run forever until the conditions for playpen to start are met
  ClearTimers();

  // Turn the middle backpack light green to know that this behavior is running
  robot.GetBackpackLightComponent().SetBackpackAnimation(_lights);

  return RESULT_OK;
}

IBehaviorPlaypen::PlaypenStatus BehaviorPlaypenWaitToStart::PlaypenUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const EngineTimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  if(robot.GetTouchSensorComponent().GetIsPressed() && _touchStartTime_ms == 0)
  {
    _touchStartTime_ms = curTime;

    _lights.onColors[(int)LEDId::LED_BACKPACK_FRONT] = NamedColors::CYAN;
    _lights.offColors[(int)LEDId::LED_BACKPACK_FRONT] = NamedColors::CYAN;
    _needLightUpdate = true;
  }
  else if(!robot.GetTouchSensorComponent().GetIsPressed() && _touchStartTime_ms != 0)
  {
    _touchStartTime_ms = 0;

    _lights.onColors[(int)LEDId::LED_BACKPACK_FRONT] = NamedColors::BLUE;
    _lights.offColors[(int)LEDId::LED_BACKPACK_FRONT] = NamedColors::BLUE;
    _needLightUpdate = true;
  }

  if(robot.IsPowerButtonPressed() && !_buttonPressed)
  {
    _lights.onColors[(int)LEDId::LED_BACKPACK_BACK] = NamedColors::GREEN;
    _lights.offColors[(int)LEDId::LED_BACKPACK_BACK] = NamedColors::GREEN;
    _needLightUpdate = true;
  }
  _buttonPressed = robot.IsPowerButtonPressed();

  if(_needLightUpdate)
  {
    robot.GetBackpackLightComponent().SetBackpackAnimation(_lights);
    _needLightUpdate = false;
  }

  // If we aren't using touchToStart or we are using it and we have been touched long enough
  const bool touchGood = !PlaypenConfig::kUseTouchToStart || 
                         (PlaypenConfig::kUseTouchToStart && 
                          (_touchStartTime_ms != 0 && 
                           curTime - _touchStartTime_ms > PlaypenConfig::kTouchDurationToStart_ms));

  // If we aren't using buttonToStart or we are using it and the button has been pressed
  // or this is sim
  const bool buttonGood = !PlaypenConfig::kUseButtonToStart || 
                          (PlaypenConfig::kUseButtonToStart &&
                           _buttonPressed);

  if(touchGood && buttonGood && (robot.GetBatteryComponent().IsOnChargerContacts() || robot.GetBatteryComponent().IsCharging()))
  {
    // Draw nothing on the screen to clear it
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::DrawTextOnScreen(true,	
                                                                                     RobotInterface::ColorRGB(0,0,0),	
                                                                                     RobotInterface::ColorRGB(0,0,0),	
                                                                                     "")));
    
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, PlaypenStatus::Complete);
  }

  return PlaypenStatus::Running;
}

void BehaviorPlaypenWaitToStart::OnBehaviorDeactivated()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  
  _touchStartTime_ms = 0;
  _buttonPressed = false;

  _lights = {
    .onColors               = {{NamedColors::BLUE,NamedColors::GREEN,NamedColors::RED}},
    .offColors              = {{NamedColors::BLUE,NamedColors::GREEN,NamedColors::RED}},
    .onPeriod_ms            = {{1000,1000,1000}},
    .offPeriod_ms           = {{100,100,100}},
    .transitionOnPeriod_ms  = {{450,450,450}},
    .transitionOffPeriod_ms = {{450,450,450}},
    .offset                 = {{0,0,0}}
  };

  robot.GetBackpackLightComponent().SetBackpackAnimation(BackpackAnimationTrigger::Off);

  // Request exit pairing mode to switchboard in case we're in it
  robot.Broadcast(ExternalInterface::MessageEngineToGame(SwitchboardInterface::ExitPairing()));

  // Tell robot process that playpen is starting so that it can reset touch sensor valid state
  // and update range values in case they were updated in emr
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::PlaypenStart()));
}

}
}


