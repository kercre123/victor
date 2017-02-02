/**
 * File: bodyLightComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Manages various lights on Cozmo's body.
 *              Currently this includes the backpack lights and headlight.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/animationContainers/backpackLightAnimationContainer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ledEncoding.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/fileUtils/fileUtils.h"

#define DEBUG_LIGHTS 0

namespace Anki {
namespace Cozmo {

BodyLightComponent::BodyLightComponent(Robot& robot, const CozmoContext* context)
: _robot(robot)
, _backpackLightAnimations(context->GetRobotManager()->GetBackpackLightAnimations())
, _currentBackpackLights(GetOffBackpackLights())
{
  // Subscribe to messages
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetHeadlight>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBackpackLEDs>();
  }
  
  static_assert((int)LEDId::NUM_BACKPACK_LEDS == 5, "BodyLightComponent.WrongNumBackpackLights");
}

void BodyLightComponent::Update()
{
  BackpackLightsState state = BackpackLightsState::OffCharger;
  if(_robot.IsOnCharger())
  {
    if(_robot.IsChargerOOS())
    {
      state = BackpackLightsState::BadCharger;
    }
    else if(_robot.IsCharging())
    {
      state = BackpackLightsState::Charging;
    }
    else
    {
      state = BackpackLightsState::Charged;
    }
  }
  
  if(state != _curBackpackState)
  {
    _curBackpackState = state;
    
    // If we are going to the off state go back to what the lights were before
    if(state == BackpackLightsState::OffCharger)
    {
      if(_loopingBackpackLights){
        SetBackpackLightsInternal(_currentLoopingBackpackLights);
      }else{
        SetBackpackLightsInternal(_currentBackpackLights);
      }
    }
    else
    {
      const auto* anim = _backpackLightAnimations.GetAnimation(StateToString(state));
      if(anim == nullptr)
      {
        PRINT_NAMED_WARNING("BodyLightComponent.Update.NullAnim",
                            "Got null anim for state %s",
                            StateToString(state));
        return;
      }
      SetBackpackLightsInternal(*anim);
    }
  }
}

Result BodyLightComponent::SetBackpackLights(const BackpackLights& lights)
{
  _currentBackpackLights = lights;
  
  // If we are looping backpack lights or our current backpack light state is not off
  // don't actually set the lights but keep track of what they were trying to be set
  // to so we can set them to it when the looping stops
  if(_loopingBackpackLights || _curBackpackState != BackpackLightsState::OffCharger)
  {
    return RESULT_OK;
  }
  
  return SetBackpackLightsInternal(lights);
}

void BodyLightComponent::SetHeadlight(bool on)
{
  _robot.GetVisionComponent().EnableMode(VisionMode::LimitedExposure, on);
  _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetHeadlight(on)));
}

void BodyLightComponent::StartLoopingBackpackLights(const BackpackLights& lights)
{
  if(_loopingBackpackLights)
  {
    PRINT_CH_INFO("BodyLightComponent", "BodyLightComponent.StartLoopingBackpackLights.AlreadyLooping",
                  "Already looping backpack lights will override current lights");
  }
  
  _loopingBackpackLights = true;
  _currentLoopingBackpackLights = lights;
  if(_curBackpackState == BackpackLightsState::OffCharger){
    SetBackpackLightsInternal(lights);
  }
}

void BodyLightComponent::StopLoopingBackpackLights()
{
  if(_loopingBackpackLights)
  {
    _loopingBackpackLights = false;
    PRINT_CH_INFO("BodyLightComponent", "BodyLightComponent.StopLoopingBackpackLights",
                  "Stopping looping backpack lights returning to previous pattern");
    if(_curBackpackState == BackpackLightsState::OffCharger){
      SetBackpackLightsInternal(_currentBackpackLights);
    }
  }
}

void BodyLightComponent::TurnOffBackpackLights()
{
  SetBackpackLights(GetOffBackpackLights());
}

template<>
void BodyLightComponent::HandleMessage(const ExternalInterface::SetBackpackLEDs& msg)
{
  const BackpackLights lights = {
    .onColors               = msg.onColor,
    .offColors              = msg.offColor,
    .onPeriod_ms            = msg.onPeriod_ms,
    .offPeriod_ms           = msg.offPeriod_ms,
    .transitionOnPeriod_ms  = msg.transitionOnPeriod_ms,
    .transitionOffPeriod_ms = msg.transitionOffPeriod_ms,
    .offset                 = msg.offset
  };
  
  SetBackpackLights(lights);
}

template<>
void BodyLightComponent::HandleMessage(const ExternalInterface::SetHeadlight& msg)
{
  SetHeadlight(msg.enable);
}

Result BodyLightComponent::SetBackpackLightsInternal(const BackpackLights& lights)
{
  std::array<Anki::Cozmo::LightState, 2> turnSignals;
  u8 turnCount = 0;
  std::array<Anki::Cozmo::LightState, 3> middleLights;
  u8 middleCount = 0;
  for (int i = 0; i < (int)LEDId::NUM_BACKPACK_LEDS; ++i)
  {
    if(i == (int)LEDId::LED_BACKPACK_RIGHT || i == (int)LEDId::LED_BACKPACK_LEFT)
    {
      turnSignals[turnCount].onColor  = ENCODED_COLOR(lights.onColors[i]);
      turnSignals[turnCount].offColor = ENCODED_COLOR(lights.offColors[i]);
      turnSignals[turnCount].onFrames  = MS_TO_LED_FRAMES(lights.onPeriod_ms[i]);
      turnSignals[turnCount].offFrames = MS_TO_LED_FRAMES(lights.offPeriod_ms[i]);
      turnSignals[turnCount].transitionOnFrames  = MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]);
      turnSignals[turnCount].transitionOffFrames = MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]);
      turnSignals[turnCount].offset = MS_TO_LED_FRAMES(lights.offset[i]);
      ++turnCount;
    }
    else
    {
      middleLights[middleCount].onColor  = ENCODED_COLOR(lights.onColors[i]);
      middleLights[middleCount].offColor = ENCODED_COLOR(lights.offColors[i]);
      middleLights[middleCount].onFrames  = MS_TO_LED_FRAMES(lights.onPeriod_ms[i]);
      middleLights[middleCount].offFrames = MS_TO_LED_FRAMES(lights.offPeriod_ms[i]);
      middleLights[middleCount].transitionOnFrames  = MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]);
      middleLights[middleCount].transitionOffFrames = MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]);
      middleLights[middleCount].offset = MS_TO_LED_FRAMES(lights.offset[i]);
      ++middleCount;
    }
    
    if(DEBUG_LIGHTS)
    {
      PRINT_CH_DEBUG("BodyLightComponent", "BodyLightComponent.SetBackpackLightsInternal",
                     "0x%x 0x%x %u %u %u %u %d",
                     ENCODED_COLOR(lights.onColors[i]),
                     ENCODED_COLOR(lights.offColors[i]),
                     MS_TO_LED_FRAMES(lights.onPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.offPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.offset[i]));
    }
  }
  
  _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::BackpackLightsMiddle(middleLights, 0)));
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::BackpackLightsTurnSignals(turnSignals, 0)));
}

const char* BodyLightComponent::StateToString(const BackpackLightsState& state) const
{
  switch(state)
  {
    case BackpackLightsState::BadCharger:
      return "BadCharger";
    case BackpackLightsState::Charged:
      return "Charged";
    case BackpackLightsState::Charging:
      return "Charging";
    case BackpackLightsState::OffCharger:
      return "OffCharger";
  }
}

const BackpackLights& BodyLightComponent::GetOffBackpackLights()
{
  static const BackpackLights kBackpackLightsOff = {
    .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{0,0,0,0,0}},
    .offPeriod_ms           = {{0,0,0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0,0}},
    .offset                 = {{0,0,0,0,0}}
  };

  return kBackpackLightsOff;
}

}
}
