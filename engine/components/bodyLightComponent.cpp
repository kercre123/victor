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

#include "engine/components/bodyLightComponent.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/animations/animationContainers/backpackLightAnimationContainer.h"
#include "engine/ankiEventUtil.h"
#include "engine/components/batteryComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
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

BodyLightComponent::BodyLightComponent()
: IDependencyManagedComponent(this, RobotComponentID::BodyLights)
{  
  static_assert((int)LEDId::NUM_BACKPACK_LEDS == 3, "BodyLightComponent.WrongNumBackpackLights");
}


void BodyLightComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  _backpackLightAnimations = std::make_unique<BackpackLightWrapper>(
              *(_robot->GetContext()->GetDataLoader()->GetBackpackLightAnimations()));
  // Subscribe to messages
  if( _robot->HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBackpackLEDs>();
  }
}


void BodyLightComponent::UpdateChargingLightConfig()
{
  // Display off lights if 
  // (1) off charger, or
  // (2) on charger but not charging or battery is full.
  BackpackLightsState state = BackpackLightsState::Off;
  if(_robot->GetBatteryComponent().IsOnChargerContacts() &&
     _robot->GetBatteryComponent().IsCharging() && 
     !_robot->GetBatteryComponent().IsBatteryFull())
  {
    state = BackpackLightsState::Charging;
  }
  else if(_robot->GetBatteryComponent().IsBatteryLow())
  {
    // Both charger out of spec and low battery backpack lights are the same
    // so instead of duplicating a lightPattern in json just use what we've already got
    state = BackpackLightsState::BadCharger;
  }
#if FACTORY_TEST
  else
  {
    state = BackpackLightsState::Idle_09;
  }
#endif
  
  if(state != _curBackpackChargeState)
  {
    _curBackpackChargeState = state;
    
    const auto* anim = _backpackLightAnimations->_container.GetAnimation(StateToString(state));
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


void BodyLightComponent::UpdateDependent(const RobotCompMap& dependentComps)
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
  // Else if both new and cur configs are null then turn lights off
  else if(newConfig == nullptr && curConfig == nullptr)
  {
    SetBackpackLightsInternal(GetOffBackpackLights());
  }
}

void BodyLightComponent::SetBackpackLights(const BackpackLights& lights)
{
  StartLoopingBackpackLightsInternal(lights, Util::EnumToUnderlying(BackpackLightSourcePrivate::Shared), _sharedLightConfig);
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

Result BodyLightComponent::SetBackpackLightsInternal(const BackpackLights& lights)
{
  // Convert MS to LED FRAMES
  #define MS_TO_LED_FRAMES(ms)  (ms == std::numeric_limits<u32>::max() ? std::numeric_limits<u8>::max() : (((ms)+29)/30))
  
  std::array<Anki::Cozmo::LightState, (int)LEDId::NUM_BACKPACK_LEDS> lightStates;
  for (int i = 0; i < (int)LEDId::NUM_BACKPACK_LEDS; ++i)
  {
    lightStates[i].onColor  = lights.onColors[i];
    lightStates[i].offColor = lights.offColors[i];
    lightStates[i].onFrames  = MS_TO_LED_FRAMES(lights.onPeriod_ms[i]);
    lightStates[i].offFrames = MS_TO_LED_FRAMES(lights.offPeriod_ms[i]);
    lightStates[i].transitionOnFrames  = MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]);
    lightStates[i].transitionOffFrames = MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]);
    lightStates[i].offset = MS_TO_LED_FRAMES(lights.offset[i]);

    
    if(DEBUG_LIGHTS)
    {
      PRINT_CH_DEBUG("BodyLightComponent", "BodyLightComponent.SetBackpackLightsInternal",
                     "0x%x 0x%x %u %u %u %u %d",
                     lights.onColors[i],
                     lights.offColors[i],
                     MS_TO_LED_FRAMES(lights.onPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.offPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]),
                     MS_TO_LED_FRAMES(lights.offset[i]));
    }
  }
  
  return _robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetBackpackLights(lightStates, 0)));
}

const char* BodyLightComponent::StateToString(const BackpackLightsState& state) const
{
  switch(state)
  {
    case BackpackLightsState::BadCharger:
      return "BadCharger";
    case BackpackLightsState::Charging:
      return "Charging";
    case BackpackLightsState::Off:
      return "Off";
    case BackpackLightsState::Idle_09:
      return "Idle_09";
  }
}

const BackpackLights& BodyLightComponent::GetOffBackpackLights()
{
  static const BackpackLights kBackpackLightsOff = {
    .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{0,0,0}},
    .offPeriod_ms           = {{0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
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
