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

#include "engine/components/bodyLightComponent.h"
#include "engine/components/touchSensorComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenWaitToStart::BehaviorPlaypenWaitToStart(const Json::Value& config)
: IBehaviorPlaypen(config)
{

}

Result BehaviorPlaypenWaitToStart::OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  // Remove the default playpen behavior timeout timer since this behavior will
  // run forever until the conditions for playpen to start are met
  ClearTimers();

  // Turn backpack lights blue to know that this behavior is running
  static const BackpackLights lights = {
    .onColors               = {{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}},
    .offColors              = {{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}},
    .onPeriod_ms            = {{1000,1000,1000}},
    .offPeriod_ms           = {{100,100,100}},
    .transitionOnPeriod_ms  = {{450,450,450}},
    .transitionOffPeriod_ms = {{450,450,450}},
    .offset                 = {{0,0,0}}
  };
  robot.GetBodyLightComponent().SetBackpackLights(lights);

  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenWaitToStart::InternalUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  const TimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  const TouchGesture gesture = robot.GetTouchSensorComponent().GetLatestTouchGesture();

  if(gesture == TouchGesture::ContactInitial && _touchStartTime_ms == 0)
  {
    _touchStartTime_ms = curTime;

    // Turn the lights cyan to indicate we are detecting a touch
    static const BackpackLights lights = {
      .onColors               = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
      .offColors              = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
      .onPeriod_ms            = {{1000,1000,1000}},
      .offPeriod_ms           = {{100,100,100}},
      .transitionOnPeriod_ms  = {{450,450,450}},
      .transitionOffPeriod_ms = {{450,450,450}},
      .offset                 = {{0,0,0}}
    };
    robot.GetBodyLightComponent().SetBackpackLights(lights);
  }
  else if(gesture == TouchGesture::NoTouch && _touchStartTime_ms != 0)
  {
    _touchStartTime_ms = 0;

    // Turn the lights blue to indicate no touch
    static const BackpackLights lights = {
      .onColors               = {{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}},
      .offColors              = {{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}},
      .onPeriod_ms            = {{1000,1000,1000}},
      .offPeriod_ms           = {{100,100,100}},
      .transitionOnPeriod_ms  = {{450,450,450}},
      .transitionOffPeriod_ms = {{450,450,450}},
      .offset                 = {{0,0,0}}
    };
    robot.GetBodyLightComponent().SetBackpackLights(lights);
  }

  if((_touchStartTime_ms != 0 && 
      curTime - _touchStartTime_ms > PlaypenConfig::kTouchDurationToStart_ms) &&
     robot.IsOnCharger())
  {
    // Draw nothing on the screen to clear it
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::DrawTextOnScreen(RobotInterface::ColorRGB(0,0,0),
                                                                                     RobotInterface::ColorRGB(0,0,0),
                                                                                     "")));

    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, BehaviorStatus::Complete);
  }

  return BehaviorStatus::Running;
}

void BehaviorPlaypenWaitToStart::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  _touchStartTime_ms = 0;

  robot.GetBodyLightComponent().SetBackpackLights(robot.GetBodyLightComponent().GetOffBackpackLights());
}

}
}


