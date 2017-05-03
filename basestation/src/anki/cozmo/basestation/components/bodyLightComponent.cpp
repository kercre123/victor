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
  
enum class BackpackLightSourcePrivate : BackpackLightSourceType
{
  Shared = Util::EnumToUnderlying(BackpackLightSource::Count),
  
  Count
};


struct BackpackLightData
{
  BackpackLights _lightConfiguration{};
};

BodyLightComponent::BodyLightComponent(Robot& robot, const CozmoContext* context)
: _robot(robot)
, _backpackLightAnimations(context->GetRobotManager()->GetBackpackLightAnimations())
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

void BodyLightComponent::UpdateChargingLightConfig()
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
  else if(_robot.GetBatteryVoltage() < LOW_BATTERY_THRESH_VOLTS)
  {
    // Both charger out of spec and low battery backpack lights are the same
    // so instead of duplicating a lightPattern in json just use what we've already got
    state = BackpackLightsState::BadCharger;
  }
  
  if(state != _curBackpackChargeState)
  {
    _curBackpackChargeState = state;
    
    const auto* anim = _backpackLightAnimations.GetAnimation(StateToString(state));
    if(anim == nullptr)
    {
      PRINT_NAMED_WARNING("BodyLightComponent.UpdateChargingLightConfig.NullAnim",
                          "Got null anim for state %s",
                          StateToString(state));
      return;
    }
    
    SetBackpackLights(*anim);
  }
}

void BodyLightComponent::Update()
{
  UpdateChargingLightConfig();
  
  BackpackLightDataRefWeak bestNewConfig = GetBestLightConfig();
  
  auto newConfig = bestNewConfig.lock();
  auto curConfig = _curBackpackLightConfig.lock();
  
  // If the best config at this time is different from what we had, change it
  if (newConfig != curConfig)
  {
    // If the best config is still a thing, use it. Otherwise use the off config
    if (newConfig != nullptr)
    {
      SetBackpackLightsInternal(newConfig->_lightConfiguration);
    }
    else
    {
      SetBackpackLightsInternal(GetOffBackpackLights());
    }
    
    _curBackpackLightConfig = bestNewConfig;
  }
}

void BodyLightComponent::SetBackpackLights(const BackpackLights& lights)
{
  StartLoopingBackpackLightsInternal(lights, Util::EnumToUnderlying(BackpackLightSourcePrivate::Shared), _sharedLightConfig);
}

void BodyLightComponent::SetHeadlight(bool on)
{
  _robot.GetVisionComponent().EnableMode(VisionMode::LimitedExposure, on);
  _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetHeadlight(on)));
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
  
void BodyLightComponent::StartLoopingBackpackLights(BackpackLights lights, BackpackLightSource source, BackpackLightDataLocator& lightLocator_out)
{
  StartLoopingBackpackLightsInternal(lights, Util::EnumToUnderlying(source), lightLocator_out);
}
  
void BodyLightComponent::StartLoopingBackpackLightsInternal(BackpackLights lights, BackpackLightSourceType source, BackpackLightDataLocator& lightLocator_out)
{
  StopLoopingBackpackLights(lightLocator_out);
  
  _backpackLightMap[source].emplace_front(new BackpackLightData{std::move(lights)});
  
  BackpackLightDataLocator result{};
  result._mapIter = _backpackLightMap.find(source);
  result._listIter = result._mapIter->second.begin();
  result._dataPtr = std::weak_ptr<BackpackLightData>(*result._listIter);
  
  lightLocator_out = std::move(result);
}
  
bool BodyLightComponent::StopLoopingBackpackLights(const BackpackLightDataLocator& lightDataLocator)
{
  if (!lightDataLocator.IsValid())
  {
    PRINT_CH_INFO("BodyLightComponent", "BodyLightComponent::RemoveBackpackLightConfig.InvalidLocator",
                  "Trying to remove an invalid locator.");
    return false;
  }
  
  if(!lightDataLocator._mapIter->second.empty())
  {
    lightDataLocator._mapIter->second.erase(lightDataLocator._listIter);
  }
  else
  {
    PRINT_NAMED_WARNING("BodyLightComponent.StopLoopingBackpackLights.NoLocators",
                        "Trying to remove supposedly valid locator but locator list is empty");
    return false;
  }
  
  if (lightDataLocator._mapIter->second.empty())
  {
    _backpackLightMap.erase(lightDataLocator._mapIter);
  }
  
  return true;
}
  
std::vector<BackpackLightSourceType> BodyLightComponent::GetLightSourcePriority()
{
  constexpr BackpackLightSourceType priorityOrder[] =
  {
    Util::EnumToUnderlying(BackpackLightSource::Voice),
    Util::EnumToUnderlying(BackpackLightSource::Behavior),
    Util::EnumToUnderlying(BackpackLightSourcePrivate::Shared)
  };
  constexpr auto numElements = sizeof(priorityOrder) / sizeof(priorityOrder[0]);
  static_assert(numElements == Util::EnumToUnderlying(BackpackLightSourcePrivate::Count), "BackpackLightSource priority list does not contain an entry for each type of BackpackLightSource.");
  
  const auto* beginIter = &priorityOrder[0];
  const auto* endIter = beginIter + numElements;
  return std::vector<BackpackLightSourceType>(beginIter, endIter);
}
  
BackpackLightDataRefWeak BodyLightComponent::GetBestLightConfig()
{
  if (_backpackLightMap.empty())
  {
    return BackpackLightDataRef{};
  }
  
  static const auto priorityList = GetLightSourcePriority();
  for (const auto& source : priorityList)
  {
    auto iter = _backpackLightMap.find(source);
    if (iter != _backpackLightMap.end())
    {
      const auto& listForSource = iter->second;
      if (!listForSource.empty())
      {
        return *listForSource.begin();
      }
    }
  }
  
  return BackpackLightDataRef{};
}

}
}
