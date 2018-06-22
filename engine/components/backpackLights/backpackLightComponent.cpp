/**
 * File: backpackLightComponent.cpp
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

#include "engine/components/backpackLights/backpackLightComponent.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/components/backpackLights/backpackLightAnimationContainer.h"
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
  BackpackLightAnimation::BackpackAnimation _lightConfiguration{};
};

BackpackLightComponent::BackpackLightComponent()
: IDependencyManagedComponent(this, RobotComponentID::BackpackLights)
{  
  static_assert((int)LEDId::NUM_BACKPACK_LEDS == 3, "BackpackLightComponent.WrongNumBackpackLights");
}


void BackpackLightComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  _backpackLightContainer = std::make_unique<BackpackLightAnimationContainer>(
    _robot->GetContext()->GetDataLoader()->GetBackpackLightAnimations());

  _backpackTriggerToNameMap = _robot->GetContext()->GetDataLoader()->GetBackpackAnimationTriggerMap();

  // Subscribe to messages
  if( _robot->HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBackpackLEDs>();
  }
}


void BackpackLightComponent::UpdateChargingLightConfig()
{
  // Display off lights if 
  // (1) off charger, or
  // (2) on charger but not charging or battery is full.
  BackpackAnimationTrigger chargeTrigger = BackpackAnimationTrigger::Off;
  if(_robot->GetBatteryComponent().IsOnChargerContacts() &&
     _robot->GetBatteryComponent().IsCharging() && 
     !_robot->GetBatteryComponent().IsBatteryFull())
  {
    chargeTrigger = BackpackAnimationTrigger::Charging;
  }
  else if(_robot->GetBatteryComponent().IsBatteryLow())
  {
    // Both charger out of spec and low battery backpack lights are the same
    // so instead of duplicating a lightPattern in json just use what we've already got
    chargeTrigger = BackpackAnimationTrigger::BadCharger;
  }
#if FACTORY_TEST
  else
  {
    chargeTrigger = BackpackAnimationTrigger::Idle_09;
  }
#endif
  
  if(chargeTrigger != _internalChargeLightsTrigger)
  {
    _internalChargeLightsTrigger = chargeTrigger;
    auto animName = _backpackTriggerToNameMap->GetValue(chargeTrigger);
    const auto* anim = _backpackLightContainer->GetAnimation(animName);
    if(anim == nullptr)
    {
      PRINT_NAMED_WARNING("BackpackLightComponent.UpdateChargingLightConfig.NullAnim",
                          "Got null anim for chargeTrigger %s",
                          EnumToString(chargeTrigger));
      return;
    }
    
    SetBackpackAnimation(*anim);
  }
}


void BackpackLightComponent::UpdateDependent(const RobotCompMap& dependentComps)
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
      SetBackpackAnimationInternal(newConfig->_lightConfiguration);
    }
    else
    {
      SetBackpackAnimation(BackpackAnimationTrigger::Off, false);
    }
    
    _curBackpackLightConfig = bestNewConfig;
  }
  // Else if both new and cur configs are null then turn lights off
  else if(newConfig == nullptr && curConfig == nullptr)
  {
    SetBackpackAnimation(BackpackAnimationTrigger::Off, false);
  }
}

void BackpackLightComponent::SetBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights)
{
  StartLoopingBackpackAnimationInternal(lights, Util::EnumToUnderlying(BackpackLightSourcePrivate::Shared), _sharedLightConfig);
}

void BackpackLightComponent::SetBackpackAnimation(const BackpackAnimationTrigger& trigger, bool shouldLoop)
{
  auto animName = _backpackTriggerToNameMap->GetValue(trigger);
  auto anim = _backpackLightContainer->GetAnimation(animName);

  if(shouldLoop){
    StartLoopingBackpackAnimationInternal(*anim, Util::EnumToUnderlying(BackpackLightSourcePrivate::Shared), _sharedLightConfig);
  }else{
    SetBackpackAnimationInternal(*anim);
  }
}

template<>
void BackpackLightComponent::HandleMessage(const ExternalInterface::SetBackpackLEDs& msg)
{
  const BackpackLightAnimation::BackpackAnimation lights = {
    .onColors               = msg.onColor,
    .offColors              = msg.offColor,
    .onPeriod_ms            = msg.onPeriod_ms,
    .offPeriod_ms           = msg.offPeriod_ms,
    .transitionOnPeriod_ms  = msg.transitionOnPeriod_ms,
    .transitionOffPeriod_ms = msg.transitionOffPeriod_ms,
    .offset                 = msg.offset
  };
  
  SetBackpackAnimation(lights);
}

Result BackpackLightComponent::SetBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights)
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
      PRINT_CH_DEBUG("BackpackLightComponent", "BackpackLightComponent.SetBackpackAnimationInternal",
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
 
void BackpackLightComponent::StartLoopingBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights, BackpackLightSource source, BackpackLightDataLocator& lightLocator_out)
{
  StartLoopingBackpackAnimationInternal(lights, Util::EnumToUnderlying(source), lightLocator_out);
}
  
void BackpackLightComponent::StartLoopingBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights, BackpackLightSourceType source, BackpackLightDataLocator& lightLocator_out)
{
  StopLoopingBackpackAnimation(lightLocator_out);
  
  _backpackLightMap[source].emplace_front(new BackpackLightData{std::move(lights)});
  
  BackpackLightDataLocator result{};
  result._mapIter = _backpackLightMap.find(source);
  result._listIter = result._mapIter->second.begin();
  result._dataPtr = std::weak_ptr<BackpackLightData>(*result._listIter);
  
  lightLocator_out = std::move(result);
}
  
bool BackpackLightComponent::StopLoopingBackpackAnimation(const BackpackLightDataLocator& lightDataLocator)
{
  if (!lightDataLocator.IsValid())
  {
    PRINT_CH_INFO("BackpackLightComponent", "BackpackLightComponent::RemoveBackpackLightConfig.InvalidLocator",
                  "Trying to remove an invalid locator.");
    return false;
  }
  
  if(!lightDataLocator._mapIter->second.empty())
  {
    lightDataLocator._mapIter->second.erase(lightDataLocator._listIter);
  }
  else
  {
    PRINT_NAMED_WARNING("BackpackLightComponent.StopLoopingBackpackAnimation.NoLocators",
                        "Trying to remove supposedly valid locator but locator list is empty");
    return false;
  }
  
  if (lightDataLocator._mapIter->second.empty())
  {
    _backpackLightMap.erase(lightDataLocator._mapIter);
  }
  
  return true;
}
  
std::vector<BackpackLightSourceType> BackpackLightComponent::GetLightSourcePriority()
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
  
BackpackLightDataRefWeak BackpackLightComponent::GetBestLightConfig()
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
