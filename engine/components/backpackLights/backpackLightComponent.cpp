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
#include "coretech/common/engine/utils/timer.h"
#include "engine/components/backpackLights/backpackLightAnimationContainer.h"
#include "engine/engineTimeStamp.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/batteryComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/internetUtils/internetUtils.h"

#include "osState/osState.h"

#define DEBUG_LIGHTS 0

namespace Anki {
namespace Vector {

CONSOLE_VAR(u32, kOfflineTimeBeforeLights_ms, "Backpacklights", (1000*60*2));
CONSOLE_VAR(u32, kOfflineCheckFreq_ms,        "Backpacklights", 10000);
  
enum class BackpackLightSourcePrivate : BackpackLightSourceType
{
  Shared = Util::EnumToUnderlying(BackpackLightSource::Count),
  Critical,
  
  Count
};


struct BackpackLightData
{
  BackpackLightAnimation::BackpackAnimation _lightConfiguration{};
};

BackpackLightComponent::BackpackLightComponent()
: IDependencyManagedComponent(this, RobotComponentID::BackpackLights)
, _offlineAtTime_ms(0)
{  
  static_assert((int)LEDId::NUM_BACKPACK_LEDS == 3, "BackpackLightComponent.WrongNumBackpackLights");
}


void BackpackLightComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
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


void BackpackLightComponent::UpdateCriticalBackpackLightConfig(bool isCloudStreamOpen)
{
  const EngineTimeStamp_t curTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
 
  // Check which, if any, backpack lights should be displayed
  // Low Battery, Offline, Streaming, Charging, or Nothing
  BackpackAnimationTrigger trigger = BackpackAnimationTrigger::Off;

  const auto& batteryComponent = _robot->GetBatteryComponent();
  if(batteryComponent.IsBatteryLow() &&
     !batteryComponent.IsCharging())
  {
    // Both charger out of spec and low battery backpack lights are the same
    // so instead of duplicating a lightPattern in json just use what we've already got
    trigger = BackpackAnimationTrigger::BadCharger;
  }
  // If we have been offline for long enough
  else if(_offlineAtTime_ms > 0 &&
          ((TimeStamp_t)curTime_ms - _offlineAtTime_ms > kOfflineTimeBeforeLights_ms))
  {
    trigger = BackpackAnimationTrigger::Offline; 
  }
  // If we are currently streaming to the cloud
  else if(isCloudStreamOpen)
  {
    trigger = BackpackAnimationTrigger::Streaming;
  }
  // If we are on the charger and charging
  else if(batteryComponent.IsOnChargerContacts() &&
          batteryComponent.IsCharging() &&
          !batteryComponent.IsBatteryFull())
  {
    trigger = BackpackAnimationTrigger::Charging;
  }

  if(trigger != _internalChargeLightsTrigger)
  {
    _internalChargeLightsTrigger = trigger;
    auto animName = _backpackTriggerToNameMap->GetValue(trigger);
    const auto* anim = _backpackLightContainer->GetAnimation(animName);
    if(anim == nullptr)
    {
      PRINT_NAMED_WARNING("BackpackLightComponent.UpdateChargingLightConfig.NullAnim",
                          "Got null anim for trigger %s",
                          EnumToString(trigger));
      return;
    }

    PRINT_CH_INFO("BackpackLightComponent",
                  "BackpackLightComponent.UpdateCriticalLightConfig",
                  "%s", EnumToString(trigger));
          
    // All of the backpack lights set by the above checks (except for Off)
    // take precedence over all other backpack lights so play them
    // under the "critical" backpack light source
    if(trigger != BackpackAnimationTrigger::Off)
    {
      StartLoopingBackpackAnimationInternal(*anim,
                                            Util::EnumToUnderlying(BackpackLightSourcePrivate::Critical),
                                            _criticalLightConfig);
    }
    else
    {
      StopLoopingBackpackAnimation(_criticalLightConfig);
    }
  }
}


void BackpackLightComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  UpdateOfflineCheck();

  auto& bc = _robot->GetAIComponent().GetComponent<BehaviorComponent>();
  const bool isCloudStreamOpen = bc.GetComponent<UserIntentComponent>().IsCloudStreamOpen();
  
  UpdateCriticalBackpackLightConfig(isCloudStreamOpen);

  UpdateSystemLightState(isCloudStreamOpen);
  
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

  if(anim == nullptr)
  {
    PRINT_NAMED_ERROR("BackpackLightComponent.SetBackpackAnimation.NoAnimForTrigger",
                      "Could not find animation for trigger %s name %s",
                      EnumToString(trigger), animName.c_str());
    return;
  }
  
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
  
  std::array<Anki::Vector::LightState, (int)LEDId::NUM_BACKPACK_LEDS> lightStates;
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
    Util::EnumToUnderlying(BackpackLightSourcePrivate::Critical),
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

void BackpackLightComponent::SetPairingLight(bool isOn)
{
  _systemLightState = (isOn ? SystemLightState::Pairing : SystemLightState::Off);
}

void BackpackLightComponent::UpdateSystemLightState(bool isCloudStreamOpen)
{
  // Check if cloud streaming has changed
  // Only show streaming system light if we are not showing anything else
  // We don't want to accidentally override the pairing light. We will still
  // indicate we are streaming with the other backpack lights.
  if(_systemLightState == SystemLightState::Off &&
     isCloudStreamOpen)
  {
    _systemLightState = SystemLightState::Streaming; 
  }
  else if(_systemLightState == SystemLightState::Streaming &&
          !isCloudStreamOpen)
  {
    _systemLightState = SystemLightState::Off; 
  }
  
  static SystemLightState prevState = SystemLightState::Invalid;
  if(prevState != _systemLightState)
  {
    prevState = _systemLightState;

    LightState light;
    switch(_systemLightState)
    {
      case SystemLightState::Invalid:
      {
        DEV_ASSERT(false, "BackpackLightComponent.UpdateSystemLightState.Invalid");
        return;
      }
      break;
      case SystemLightState::Off:
      {
        light.onColor = 0x00FF0000;
        light.offColor = 0x00FF0000;
        light.onFrames = 1;
        light.offFrames = 1;
        light.transitionOnFrames = 0;
        light.transitionOffFrames = 0;
        light.offset = 0;
      }
      break;

      case SystemLightState::Pairing:
      {
        // Pulsing yellow
        light.onColor = 0xFFFF0000;
        light.offColor = 0x00FF0000;
        light.onFrames = 32;
        light.offFrames = 32;
        light.transitionOnFrames = 0;
        light.transitionOffFrames = 0;
        light.offset = 0;
      }
      break;

      case SystemLightState::Streaming:
      {
        // Pulsing cyan
        light.onColor = 0x00FFFF00;
        light.offColor = 0x00FFFF00;
        light.onFrames = 32;
        light.offFrames = 32;
        light.transitionOnFrames = 0;
        light.transitionOffFrames = 0;
        light.offset = 0;
      }
      break;
    }

    // If user space is unsecure then mix white in
    // to the system light as the off color (normally green)
    if(!OSState::getInstance()->IsUserSpaceSecure())
    {
      light.offColor = 0xFFFFFF00;
      light.onFrames = 32;
      light.offFrames = 32;
      light.transitionOnFrames = 0;
      light.transitionOffFrames = 0;
      light.offset = 0;
    }

    _robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetSystemLight({light})));
  }
}

// TODO: Connectivity checks should be moved to a central location as multiple
// parts of the system are starting to do them (faceInfoScreenManager.cpp)
// VIC-4094 VIC-4099
void BackpackLightComponent::UpdateOfflineCheck()
{
  static EngineTimeStamp_t lastCheck_ms = 0;
  const EngineTimeStamp_t curTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  auto offlineCheck = [this, curTime_ms]() {
    const bool hasVoiceAccess = Anki::Util::InternetUtils::CanConnectToHostName("chipper-dev.api.anki.com", 443);
    if(_offlineAtTime_ms == 0 && !hasVoiceAccess)
    {
      _offlineAtTime_ms = (TimeStamp_t)curTime_ms;
    }
    else if(_offlineAtTime_ms > 0 && hasVoiceAccess)
    {
      _offlineAtTime_ms = 0;
    }
  };

  const bool valid = _offlineCheckFuture.valid();
  std::future_status status = std::future_status::ready;
  if(valid)
  {
    status = _offlineCheckFuture.wait_for(std::chrono::milliseconds(0));
  }
  if(status == std::future_status::ready &&
     curTime_ms - lastCheck_ms > kOfflineCheckFreq_ms)
  {
    lastCheck_ms = curTime_ms;
    _offlineCheckFuture = std::async(std::launch::async, offlineCheck);  
  }
}

}
}
