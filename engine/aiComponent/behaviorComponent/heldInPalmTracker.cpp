/**
 * File: heldInPalmTracker.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019-02-05
 *
 * Description: Behavior component to track various metrics, such as the
 * "trust" level that the robot has when being held in a user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#define LOG_CHANNEL "Behaviors"

#include "engine/aiComponent/behaviorComponent/heldInPalmTracker.h"

#include "clad/audio/audioParameterTypes.h"
#include "clad/types/featureGateTypes.h"

#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include "webServerProcess/src/webService.h"
#include "webServerProcess/src/webVizSender.h"

namespace Anki {
namespace Vector {

#define CONSOLE_GROUP "HeldInPalm.Tracker"

// Gain trust at this rate when held in a palm
CONSOLE_VAR(float, kHeldInPalm_trustLevelRate, CONSOLE_GROUP, 0.f);
CONSOLE_VAR(float, kHeldInPalmTracker_updatePeriod_s, CONSOLE_GROUP, 60.0);

namespace {
#if REMOTE_CONSOLE_ENABLED
  static float sForceTrustLevelDelta = 0.0f;
  void ForceTrustLevelDelta(ConsoleFunctionContextRef context)
  {
    sForceTrustLevelDelta = ConsoleArg_Get_Float(context, "deltaTrustLevel");
  }
  
  static bool sForceSetNewTrustLevel = false;
  void ForceSetNewTrustLevel(ConsoleFunctionContextRef context)
  {
    sForceSetNewTrustLevel = true;
  }
#endif

  static const char* kWebVizModuleNameHeldInPalm = "heldinpalm";
  static const float kMinTrustLevel = 0.0f;
  static const float kMaxTrustLevel = 1.0f;
  static const float kDefaultForcedTrustLevel = (kMinTrustLevel + kMaxTrustLevel)/2.0f;
}

// Attempts to increase/decrease trust by this amount instantly. This may not always do what the
// user expects if the input `deltaTrustLevel` would set the trust level to be outside of the
// range defined by [kMinTrustLevel, kMaxTrustLevel], in which case the trust level will increase
// or decrease up to those bounds.
CONSOLE_FUNC(ForceTrustLevelDelta, CONSOLE_GROUP, float deltaTrustLevel);
  
// This console variable doesn't actually change the trust level until the `ForceSetNewTrustLevel`
// console function is called.  This is done in order to prevent the user from trying to set the
// level to a value outside of the range defined by [kMinTrustLevel, kMaxTrustLevel].
CONSOLE_VAR_RANGED(float, kForcedTrustLevel, CONSOLE_GROUP, kDefaultForcedTrustLevel, kMinTrustLevel, kMaxTrustLevel);
CONSOLE_FUNC(ForceSetNewTrustLevel, CONSOLE_GROUP);
  
// Force the trust level of the tracker to remain constant. The only way to change the trust level
// when this is enabled is to use the `ForceSetNewTrustLevel` or `ForceTrustLevelDelta` console
// functions above.
CONSOLE_VAR(bool, kForceConstTrustLevel, CONSOLE_GROUP, false);

HeldInPalmTracker::HeldInPalmTracker()
  : IDependencyManagedComponent(this, BCComponentID::HeldInPalmTracker ),
    _trustEvents( { { TrustEventType::InitialHeldInPalmReactionPlayed,  0.1f},
                    { TrustEventType::RobotJolted,                     -0.2f},
                    { TrustEventType::RobotPetted,                      0.2f},
                  }
                )
{
}

void HeldInPalmTracker::SetIsHeldInPalm(const bool isHeldInPalm) {
  if (_isHeldInPalm != isHeldInPalm) {
    LOG_INFO("HeldInPalmTracker.SetIsHeldInPalm", "%s", isHeldInPalm ? "true" : "false");
  }
  _isHeldInPalm = isHeldInPalm;
}

void HeldInPalmTracker::SetTrustLevel(const float trustLevel, const bool verboseWarn)
{
  const float prevTrustLevel = _trustLevel;
  _trustLevel = Util::Clamp(trustLevel, kMinTrustLevel, kMaxTrustLevel);
  if (trustLevel != _trustLevel && verboseWarn) {
    LOG_WARNING("HeldInPalmTracker.SetTrustLevel.InvalidTrustLevel",
                "Cannot set trust level to %f, outside of acceptable range of [%f, %f]",
                trustLevel, kMinTrustLevel, kMaxTrustLevel);
  }
  if (prevTrustLevel != _trustLevel) {
    LOG_INFO("HeldInPalmTracker.SetTrustLevel", "%f", _trustLevel);
  }
}

void HeldInPalmTracker::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  // Cache a pointer to the robot's audio client component if it exists
  {
    const bool hasAudioComp = robot && robot->HasComponent<Audio::EngineRobotAudioClient>();
    _audioClient = hasAudioComp ? robot->GetComponentPtr<Audio::EngineRobotAudioClient>() : nullptr;
  }
  
  // Set up event handle for WebViz subscription, and cache a pointer to the FeatureGate interface if it exists
  const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      auto onWebVizSubscribed = [this](const std::function<void(const Json::Value&)>& sendToClient) {
        // Send data upon getting subscribed to WebViz
        Json::Value data;
        PopulateWebVizJson(data);
        sendToClient(data);
      };
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameHeldInPalm ).ScopedSubscribe(
                                    onWebVizSubscribed ));
    }
    
    _featureGate = context->GetFeatureGate();
  }
}

void HeldInPalmTracker::UpdateDependent(const BCCompMap& dependentComps)
{
  bool consoleFuncUsed = false;
#if REMOTE_CONSOLE_ENABLED
  consoleFuncUsed = sForceTrustLevelDelta != 0.0f || sForceSetNewTrustLevel;
#endif

  const float currBSTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( consoleFuncUsed || currBSTime_s > _nextUpdateTime_s ) {
    _nextUpdateTime_s = currBSTime_s + kHeldInPalmTracker_updatePeriod_s;

    if( consoleFuncUsed ) {
#if REMOTE_CONSOLE_ENABLED
      if (sForceTrustLevelDelta != 0.0f) {
        LOG_WARNING("HeldInPalmTracker.ConsoleFunc.GainTrust",
                    "Increase trust by %f",
                    sForceTrustLevelDelta);
        const float possibleNewTrustLevel = _trustLevel + sForceTrustLevelDelta;
        SetTrustLevel(possibleNewTrustLevel);
        sForceTrustLevelDelta = 0.0f;
      }
      if (sForceSetNewTrustLevel) {
        LOG_WARNING("HeldInPalmTracker.ConsoleFunc.ForceNewTrustLevel",
                    "%f", kForcedTrustLevel);
        SetTrustLevel(kForcedTrustLevel);
        sForceSetNewTrustLevel = false;
      }
#endif
    } else if( _isHeldInPalm && !kForceConstTrustLevel) {
      const float possibleNewTrustLevel = _trustLevel + kHeldInPalmTracker_updatePeriod_s * kHeldInPalm_trustLevelRate;
      SetTrustLevel(possibleNewTrustLevel, false);
    }

    // Update the audio client anytime the "trust" level is forcefully updated via a console variable/function
    // or the "trust" level changes due to a time-based update.
    SendTrustLevelToAudio();
    
    const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
    if( context ) {
      if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameHeldInPalm, context->GetWebService()) ) {
        PopulateWebVizJson(webSender->Data());
      }
    }
  }
}
  
void HeldInPalmTracker::UpdateTrustLevelForEvent( const TrustEventType& trustEventType )
{
  if (kForceConstTrustLevel) {
    LOG_DEBUG("HeldInPalmTracker.UpdateTrustLevelForEvent",
              "%s event ignored, constant trust level is being enforced",
              HeldInPalmTrustEventTypeToString(trustEventType));
    return;
  }
  
  const auto trustEvent = _trustEvents.find(trustEventType);
  if ( trustEvent != _trustEvents.end() ) {
    const float possibleNewTrustLevel = _trustLevel + trustEvent->second;
    SetTrustLevel(possibleNewTrustLevel, false);
    _lastTrustEventType = trustEventType;
    
    // Update audio now instead of waiting for the next natural update
    SendTrustLevelToAudio();
  } else {
    LOG_WARNING("HeldInPalmTracker.UpdateTrustLevelForEvent.InvalidTrustEventType",
                "%s event ignored, type not mapped to any trust level change",
                HeldInPalmTrustEventTypeToString(trustEventType));
  }
  
}

void HeldInPalmTracker::SendTrustLevelToAudio()
{
  // Disable audio updates for PR demo (Audio might use it's own settings)
  if( _featureGate && _featureGate->IsFeatureEnabled( FeatureType::PRDemo ) ) {
    return;
  }

  if ( _audioClient ) {
    _audioClient->PostParameter(AudioMetaData::GameParameter::ParameterType::Robot_Vic_Held_Trust,
                                _trustLevel);
  }
}

void HeldInPalmTracker::PopulateWebVizJson(Json::Value& data) const
{
  data["held_in_palm_trust_level"] = _trustLevel;
  if( _lastTrustEventType != TrustEventType::Invalid ) {
    data["last_trust_event_type"] = HeldInPalmTrustEventTypeToString(_lastTrustEventType);
  }
}

}
}
