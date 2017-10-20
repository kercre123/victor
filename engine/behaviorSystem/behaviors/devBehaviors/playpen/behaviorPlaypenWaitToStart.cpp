/**
 * File: behaviorPlaypenWaitToStart.cpp
 *
 * Author: Al Chaussee
 * Created: 10/12/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenWaitToStart.h"

#include "engine/components/bodyLightComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include <iomanip>

namespace Anki {
namespace Cozmo {

BehaviorPlaypenWaitToStart::BehaviorPlaypenWaitToStart(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({EngineToGameTag::RobotTouched});
}

Result BehaviorPlaypenWaitToStart::InternalInitInternal(Robot& robot)
{
  // Remove the default playpen behavior timeout timer since this behavior will
  // run forever until the conditions for playpen to start are met
  ClearTimers();

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

BehaviorStatus BehaviorPlaypenWaitToStart::InternalUpdateInternal(Robot& robot)
{
  const TimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  if((touchStartTime_ms != 0 && curTime - touchStartTime_ms > 1000) &&
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

void BehaviorPlaypenWaitToStart::StopInternal(Robot& robot)
{
  touchStartTime_ms = 0;

  robot.GetBodyLightComponent().SetBackpackLights(robot.GetBodyLightComponent().GetOffBackpackLights());
}

void BehaviorPlaypenWaitToStart::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  const EngineToGameTag tag = event.GetData().GetTag();
  if(tag == EngineToGameTag::RobotTouched)
  {
    const auto& payload = event.GetData().Get_RobotTouched();

    if(payload.touchStarted)
    {
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
      touchStartTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    }
    else
    {
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
      touchStartTime_ms = 0;
    }
  }
}

}
}


