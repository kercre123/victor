/**
 * File: animBackpackLightComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Manages various lights on Vector's body.
 *              Currently this includes the backpack lights.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/backpackLights/animBackpackLightComponent.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animTimeStamp.h"
#include "cozmoAnim/animComms.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/robotDataLoader.h"
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
  Engine = Util::EnumToUnderlying(BackpackLightSource::Count),
  Critical,
  
  Count
};

struct BackpackLightData
{
  BackpackLightAnimation::BackpackAnimation _lightConfiguration;
};
  
BackpackLightComponent::BackpackLightComponent(const AnimContext* context)
: _context(context)
, _offlineAtTime_ms(0)
{  
  static_assert((int)LEDId::NUM_BACKPACK_LEDS == 3, "BackpackLightComponent.WrongNumBackpackLights");

  // Add callbacks so we know when trigger word/audio stream are updated
  _context->GetMicDataSystem()->AddTriggerWordDetectedCallback([this](bool willStream)
    {
      _willStreamOpen = willStream;
    });
  
  _context->GetMicDataSystem()->AddStreamUpdatedCallback([this](bool streamStart)
    {
      _isStreaming = streamStart;
      _willStreamOpen = false;
    });
}


void BackpackLightComponent::Init()
{
  _backpackLightContainer = std::make_unique<BackpackLightAnimationContainer>(
    _context->GetDataLoader()->GetBackpackLightAnimations());

  _backpackTriggerToNameMap = _context->GetDataLoader()->GetBackpackAnimationTriggerMap();
}


void BackpackLightComponent::UpdateCriticalBackpackLightConfig(bool isCloudStreamOpen)
{
  const AnimTimeStamp_t curTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
 
  // Check which, if any, backpack lights should be displayed
  // Low Battery, Offline, Streaming, Charging, or Nothing
  BackpackAnimationTrigger trigger = BackpackAnimationTrigger::Off;

  if(_isBatteryLow && !_isBatteryCharging)
  {
    trigger = BackpackAnimationTrigger::LowBattery;
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
  else if(_isOnChargerContacts &&
          _isBatteryCharging &&
          !_isBatteryFull)
  {
    trigger = BackpackAnimationTrigger::Charging;
  }

  if(trigger != _internalCriticalLightsTrigger)
  {
    _internalCriticalLightsTrigger = trigger;
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
      StartBackpackAnimationInternal(*anim,
                                     Util::EnumToUnderlying(BackpackLightSourcePrivate::Critical),
                                     _criticalLightConfig);
    }
    else
    {
      StopBackpackAnimationInternal(_criticalLightConfig);
    }
  }
}


void BackpackLightComponent::Update()
{
  UpdateOfflineCheck();

  // Consider stream to be open when the trigger word is detected or we are actually
  // streaming. Trigger word stays detected until the stream state is updated
  const bool isCloudStreamOpen = (_willStreamOpen || _isStreaming);
  UpdateCriticalBackpackLightConfig(isCloudStreamOpen);

  UpdateSystemLightState(isCloudStreamOpen);
  
  BackpackLightDataRefWeak bestNewConfig = GetBestLightConfig();
  
  auto newConfig = bestNewConfig.lock();
  auto curConfig = _curBackpackLightConfig.lock();

  // Prevent spamming of off lights while both configs are null
  static bool sBothConfigsWereNull = false;

  // If the best config at this time is different from what we had, change it
  if (newConfig != curConfig)
  {
    sBothConfigsWereNull = false;
    // If the best config is still a thing, use it. Otherwise use the off config
    if (newConfig != nullptr)
    {
      SendBackpackLights(newConfig->_lightConfiguration);
    }
    else
    {
      SendBackpackLights(BackpackAnimationTrigger::Off);
    }
    
    _curBackpackLightConfig = bestNewConfig;
  }
  // Else if both new and cur configs are null then turn lights off
  else if(newConfig == nullptr && curConfig == nullptr &&
          !sBothConfigsWereNull)
  {
    sBothConfigsWereNull = true;
    SendBackpackLights(BackpackAnimationTrigger::Off);
  }
}

void BackpackLightComponent::SetBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights)
{
  StartBackpackAnimationInternal(lights,
                                 Util::EnumToUnderlying(BackpackLightSourcePrivate::Engine),
                                 _engineLightConfig);
}

void BackpackLightComponent::SetBackpackAnimation(const BackpackAnimationTrigger& trigger)
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
  
  StartBackpackAnimationInternal(*anim,
                                 Util::EnumToUnderlying(BackpackLightSourcePrivate::Engine),
                                 _engineLightConfig);
}
 
void BackpackLightComponent::StartBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights,
                                                            BackpackLightSourceType source,
                                                            BackpackLightDataLocator& lightLocator_out)
{
  StopBackpackAnimationInternal(lightLocator_out);

  _backpackLightMap[source].emplace_front(new BackpackLightData{lights});
  
  BackpackLightDataLocator result{};
  result._mapIter = _backpackLightMap.find(source);
  result._listIter = result._mapIter->second.begin();
  result._dataPtr = std::weak_ptr<BackpackLightData>(*result._listIter);
  
  lightLocator_out = std::move(result);
}
  
bool BackpackLightComponent::StopBackpackAnimationInternal(const BackpackLightDataLocator& lightDataLocator)
{
  if (!lightDataLocator.IsValid())
  {
    PRINT_CH_INFO("BackpackLightComponent",
                  "BackpackLightComponent.StopBackpackAnimationInternal.InvalidLocator",
                  "Trying to remove an invalid locator.");
    return false;
  }
  
  if(!lightDataLocator._mapIter->second.empty())
  {
    lightDataLocator._mapIter->second.erase(lightDataLocator._listIter);
  }
  else
  {
    PRINT_NAMED_WARNING("BackpackLightComponent.StopBackpackAnimationInternal.NoLocators",
                        "Trying to remove supposedly valid locator but locator list is empty");
    return false;
  }
  
  if (lightDataLocator._mapIter->second.empty())
  {
    _backpackLightMap.erase(lightDataLocator._mapIter);
  }
  
  return true;
}

Result BackpackLightComponent::SendBackpackLights(const BackpackLightAnimation::BackpackAnimation& lights)
{
  RobotInterface::SetBackpackLights setBackpackLights = lights.lights;
  setBackpackLights.layer = EnumToUnderlyingType(BackpackLightLayer::BPL_USER);

  const auto msg = RobotInterface::EngineToRobot(setBackpackLights);
  const bool res = AnimComms::SendPacketToRobot((char*)msg.GetBuffer(), msg.Size());
  return (res ? RESULT_OK : RESULT_FAIL);
}

Result BackpackLightComponent::SendBackpackLights(const BackpackAnimationTrigger& trigger)
{
  auto animName = _backpackTriggerToNameMap->GetValue(trigger);
  auto anim = _backpackLightContainer->GetAnimation(animName);

  if(anim == nullptr)
  {
    PRINT_NAMED_ERROR("BackpackLightComponent.SendBackpackLights.NoAnimForTrigger",
                      "Could not find animation for trigger %s name %s",
                      EnumToString(trigger), animName.c_str());
    return RESULT_FAIL;
  }

  return SendBackpackLights(*anim);
}


std::vector<BackpackLightSourceType> BackpackLightComponent::GetLightSourcePriority()
{
  constexpr BackpackLightSourceType priorityOrder[] =
  {
    Util::EnumToUnderlying(BackpackLightSourcePrivate::Critical),
    Util::EnumToUnderlying(BackpackLightSourcePrivate::Engine),
  };
  constexpr auto numElements = sizeof(priorityOrder) / sizeof(priorityOrder[0]);
  static_assert(numElements == Util::EnumToUnderlying(BackpackLightSourcePrivate::Count),
                "BackpackLightSource priority list does not contain an entry for each type of BackpackLightSource.");
  
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
        light.onPeriod_ms = 960;
        light.offPeriod_ms = 960;
        light.transitionOnPeriod_ms = 0;
        light.transitionOffPeriod_ms = 0;
        light.offset_ms = 0;
      }
      break;

      case SystemLightState::Pairing:
      {
        // Pulsing yellow
        light.onColor = 0xFFFF0000;
        light.offColor = 0x00FF0000;
        light.onPeriod_ms = 960;
        light.offPeriod_ms = 960;
        light.transitionOnPeriod_ms = 0;
        light.transitionOffPeriod_ms = 0;
        light.offset_ms = 0;
      }
      break;

      case SystemLightState::Streaming:
      {
        // Pulsing cyan
        light.onColor = 0x00FFFF00;
        light.offColor = 0x00FFFF00;
        light.onPeriod_ms = 960;
        light.offPeriod_ms = 960;
        light.transitionOnPeriod_ms = 0;
        light.transitionOffPeriod_ms = 0;
        light.offset_ms = 0;
      }
      break;
    }

    // If user space is unsecure then mix white in
    // to the system light as the off color (normally green)
    if(!OSState::getInstance()->IsUserSpaceSecure())
    {
      light.offColor = 0xFFFFFF00;
      light.onPeriod_ms = 960;
      light.offPeriod_ms = 960;
      light.transitionOnPeriod_ms = 0;
      light.transitionOffPeriod_ms = 0;
      light.offset_ms = 0;
    }

    const auto msg = RobotInterface::EngineToRobot(RobotInterface::SetSystemLight({light}));
    AnimComms::SendPacketToRobot((char*)msg.GetBuffer(), msg.Size());
  }
}

// TODO: Rework offline check to just check if connected to AP and have ip address
// VIC-5437
void BackpackLightComponent::UpdateOfflineCheck()
{
  static AnimTimeStamp_t lastCheck_ms = 0;
  const AnimTimeStamp_t curTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

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

void BackpackLightComponent::UpdateBatteryStatus(const RobotInterface::BatteryStatus& msg)
{
  _isBatteryLow = msg.isLow;
  _isBatteryCharging = msg.isCharging;
  _isOnChargerContacts = msg.onChargerContacts;
  _isBatteryFull = msg.isBatteryFull;
}

}
}
