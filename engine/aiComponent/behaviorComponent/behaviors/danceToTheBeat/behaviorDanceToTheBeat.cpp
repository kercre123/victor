/**
 * File: behaviorDanceToTheBeat.cpp
 *
 * Author: Matt Michini
 * Created: 2018-05-07
 *
 * Description: Dance to a beat detected by the microphones/beat detection algorithm
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/behaviorDanceToTheBeat.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/logging/DAS.h"
#include "util/random/randomGenerator.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Vector {

namespace {
  
  // This is a fixed latency correction factor. Animation
  // commands are sent to the anim process early by this
  // amount to account for messaging/tick latency/timing.
  const float kLatencyCorrectionTime_sec = 0.050f;
  
  const char* kUseBackpackLights_key = "useBackpackLights";
  const char* kBackpackAnim_key  = "backpackAnim";
  const char* kEyeHoldAnim_key   = "eyeHoldAnim";
  const char* kGetOutAnim_key    = "getOutAnim";
  const char* kDanceSessions_key = "danceSessions";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::BehaviorDanceToTheBeat(const Json::Value& config)
  : ICozmoBehavior(config)
  , _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
  : useBackpackLights(JsonTools::ParseBool(config, kUseBackpackLights_key, debugName))
{
  eyeHoldAnim = AnimationTrigger::Count;
  getOutAnim = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kBackpackAnim_key,  backpackAnim,  debugName);
  JsonTools::GetCladEnumFromJSON(config, kEyeHoldAnim_key,   eyeHoldAnim,   debugName);
  JsonTools::GetCladEnumFromJSON(config, kGetOutAnim_key,    getOutAnim,    debugName);
  
  const auto& danceSessionConfig = config[kDanceSessions_key];
  DEV_ASSERT(!danceSessionConfig.isNull() && danceSessionConfig.isArray() && !danceSessionConfig.empty(),
             "BehaviorDanceToTheBeat.InstanceConfig.EmptyOrInvalidDanceSessionList");
  for (const auto& json : danceSessionConfig) {
    danceSessionConfigs.emplace_back(json);
    DEV_ASSERT(danceSessionConfigs.back().IsValid(), "BehaviorDanceToTheBeat.InstanceConfig.InvalidDanceSessionConfig");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::InstanceConfig::CheckValid()
{
  isValid = true;
  
  if (danceSessionConfigs.empty()) {
    isValid = false;
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.InstanceConfig.CheckValid.NoDanceSessions",
                        "Invalid config - no dance session config entries");
  }

  for (const auto& session : danceSessionConfigs) {
    if (!session.IsValid()) {
      isValid = false;
    }
  }

  return isValid;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kUseBackpackLights_key,
    kBackpackAnim_key,
    kEyeHoldAnim_key,
    kGetOutAnim_key,
    kDanceSessions_key,
  };
  expectedKeys.insert(std::begin(list), std::end(list));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::InitBehavior()
{
  // Initialize "dance sessions"
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if (animContainer == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.InitBehavior.NullAnimContainer",
                      "Null CannedAnimationContainer!");
    return;
  }
  for (auto& session : _iConfig.danceSessionConfigs) {
    const bool success = session.Init(*animContainer);
    DEV_ASSERT(success, "BehaviorDanceToTheBeat.InitBehavior.FailedInitializingDanceSession");
  }
  
  // Double check that the config is still valid
  ANKI_VERIFY(_iConfig.CheckValid(), "BehaviorDanceToTheBeat.InitBehavior.InvalidConfig", "");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorActivated()
{
  if (!_iConfig.isValid) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.OnBehaviorActivated.InvalidConfig",
                        "Configuration is invalid - ending behavior");
    CancelSelf();
    return;
  }

  // Reset dynamic variables:
  _dVars = DynamicVariables();
  
  // Pre-generate the queue of all animations to play by asking each dance session to generate a list of animations
  for (const auto& sessionConfig : _iConfig.danceSessionConfigs) {
    const auto& animSequence = sessionConfig.GenerateAnimSequence();
    _dVars.danceAnims.insert(_dVars.danceAnims.end(),
                             animSequence.begin(),
                             animSequence.end());
  }
  
  // Log DAS event
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  DASMSG(dttb_activated, "dttb.activated", "DanceToTheBeat behavior activated");
  DASMSG_SET(i1, beatDetector.GetCurrTempo_bpm(), "Current tempo estimate (beats per minute)");
  DASMSG_SET(i2, 1000.f * beatDetector.GetCurrConfidence(), "Current detection confidence (unitless). This is the raw confidence value multiplied by 1000");
  DASMSG_SET(i3, (int) _dVars.danceAnims.size(), "Number of dance animations that have been queued to play");
  DASMSG_SEND();

  TransitionToDancing();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const bool danceAnimQueued = (_dVars.nextAnimTriggerTime_sec > 0.f);
  
  // If we have no animation queued and we're not currently doing anything, it's time to quit.
  if (!IsControlDelegated() && !danceAnimQueued) {
    CancelSelf();
  }
  
  if (danceAnimQueued) {
    WhileDancing();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorDeactivated()
{
  StopBackpackLights();
  UnregisterOnBeatCallback();
  
  // Log DAS event
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  DASMSG(dttb_end, "dttb.end", "DanceToTheBeat deactivated");
  DASMSG_SET(i1, beatDetector.GetCurrTempo_bpm(), "Current tempo estimate (beats per minute)");
  DASMSG_SET(i2, 1000.f * beatDetector.GetCurrConfidence(), "Current detection confidence (unitless). This is the raw confidence value multiplied by 1000");
  DASMSG_SET(i3, (int) _dVars.danceAnims.size(), "Number of unplayed dance animations remaining in the queue");
  DASMSG_SEND();
  
  // Clear out any remaining queued anims to free memory
  _dVars.danceAnims.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::TransitionToDancing()
{
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  
  // Double check that a beat is still detected. If no beat is detected, do not queue a dance animation. This will cause
  // the behavior to cancel itself in Update(). Note: We cannot cancel the behavior here since unit tests disallow
  // cancelling a behavior on the same tick it was activated.
  if (!beatDetector.IsBeatDetected()) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.TransitionToDancing.NoBeat",
                        "Beat no longer detected - not queuing any dance animations");
    _dVars.nextBeatTime_sec = -1.f;
    _dVars.beatPeriod_sec = -1.f;
    _dVars.nextAnimTriggerTime_sec = -1.f;
    return;
  }
  
  // grab the next beat time here and wait to play the anim until the appropriate time
  _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
  _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();

  // Kick off the dancing animations by setting the next trigger time.
  SetNextAnimTriggerTime();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::WhileDancing()
{
  // If we are currently listening for beats, double check that a beat
  // is still detected. If not, we should bail out of the behavior.
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  if (_dVars.listeningForBeats &&
      !beatDetector.IsBeatDetected()) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.WhileDancing.BeatNoLongerDetected",
                        "Cancelling behavior since we are currently listening for beats "
                        "and BeatDetectorComponent says beat is no longer detected (play getout anim: %d)",
                        _dVars.playGetoutIfBeatLost);
    DASMSG(dttb_cancel_beat_lost, "dttb.cancel_beat_lost", "DanceToTheBeat cancelling because beat is lost");
    DASMSG_SET(i1, beatDetector.GetCurrTempo_bpm(), "Current tempo estimate (beats per minute)");
    DASMSG_SET(i2, 1000.f * beatDetector.GetCurrConfidence(), "Current detection confidence (unitless). This is the raw confidence value multiplied by 1000");
    DASMSG_SET(i3, (int) _dVars.danceAnims.size(), "Number of unplayed dance animations remaining in the queue");
    DASMSG_SEND();
    _dVars.nextAnimTriggerTime_sec = -1.f;
    SetListeningForBeats(false);
    if (_dVars.playGetoutIfBeatLost) {
      DelegateNow(new TriggerAnimationAction(_iConfig.getOutAnim), [this](){
        CancelSelf();
      });
    } else {
      // No get-out, just cancel ourself
      CancelSelf();
    }
    return;
  }
  
  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  // If we're not listening for beats, we need to call OnBeat() 'manually' when the next 'extrapolated' beat occurs.
  if (!_dVars.listeningForBeats) {
    if (now_sec > _dVars.nextBeatTime_sec) {
      OnBeat();
    }
  }
  
  // Play the next dancing animation if it's time
  if (_dVars.nextAnimTriggerTime_sec > 0.f &&
      now_sec >= _dVars.nextAnimTriggerTime_sec) {
    PlayNextDanceAnim();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::PlayNextDanceAnim()
{
  // Time to play the next animation in the queue
  DEV_ASSERT(!_dVars.danceAnims.empty(), "BehaviorDanceToTheBeat.PlayNextDanceAnim.NoDanceSessions");
  
  SetListeningForBeats(_dVars.danceAnims.front().CanListenForBeats());
  _dVars.playGetoutIfBeatLost = _dVars.danceAnims.front().PlayGetoutIfInterrupted();
  auto* animAction = new PlayAnimationAction(_dVars.danceAnims.front().GetAnimName());
  _dVars.danceAnims.pop_front();
  
  if (_dVars.danceAnims.empty()) {
    // No more animations to play after this one, so play the get-out after
    DelegateNow(animAction, [this](){
      DelegateIfInControl(new TriggerAnimationAction(_iConfig.getOutAnim));
    });
    _dVars.nextAnimTriggerTime_sec = -1.f;
  } else {
    // Delegate to this animation, and play the eye hold indefinitely after that
    DelegateNow(animAction, [this](){
      DelegateIfInControl(new ReselectingLoopAnimationAction(_iConfig.eyeHoldAnim));
    });
    SetNextAnimTriggerTime();
  }
  
  // Since we've played a dancing animation, reset the dancing cooldown behavior timer
  GetBEI().GetBehaviorTimerManager().GetTimer(BehaviorTimerTypes::DancingCooldown).Reset();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::SetNextAnimTriggerTime()
{
  if (_dVars.danceAnims.empty()) {
    _dVars.nextAnimTriggerTime_sec = -1.f;
    DEV_ASSERT(false, "BehaviorDanceToTheBeat.SetNextAnimTriggerTime.NoAnimsInQueue");
    return;
  }

  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());

  const auto& nextAnim = _dVars.danceAnims.front();

  // Must start the animation beatDelay_sec before the next beat. Also include
  // the latency correction factor to account for messaging/tick times, etc.
  _dVars.nextAnimTriggerTime_sec = _dVars.nextBeatTime_sec - nextAnim.GetBeatDelay_sec() - kLatencyCorrectionTime_sec;

  // Make sure the animation trigger time is somewhere in the future
  BOUNDED_WHILE(1000, _dVars.nextAnimTriggerTime_sec < now_sec) {
    _dVars.nextAnimTriggerTime_sec += _dVars.beatPeriod_sec;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBeat()
{
  if (!IsActivated()) {
    DEV_ASSERT(false, "BehaviorDanceToTheBeat.OnBeat.NotActivated");
    return;
  }

  PRINT_CH_INFO("Behaviors", "BehaviorDanceToTheBeat.OnBeat",
                "OnBeat() called. Currently%s listening for new beats. Current universal time (sec): %.3f",
                _dVars.listeningForBeats ? "" : " NOT",
                Util::Time::UniversalTime::GetCurrentTimeInSeconds());

  if (_iConfig.useBackpackLights) {
    StopBackpackLights();
    auto& blc = GetBEI().GetBackpackLightComponent();
    blc.SetBackpackAnimation(_iConfig.backpackAnim);
  }

  // If we're currently listening for beats, then this means we have new information from the BeatDetectorComponent.
  if (_dVars.listeningForBeats) {
    // Update some stuff now that we have new beat information from BeatDetectorComponent. Only update if there is
    // still a beat currently detected (to avoid invalid values for tempo and nextBeatTime). If the beat has stopped,
    // WhileDancing() will take care of cancelling the behavior.
    const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
    if (beatDetector.IsBeatDetected()) {
      _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
      _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();
    }
      
    // Update the next animation trigger time if we still have animations in the queue.
    if (!_dVars.danceAnims.empty()) {
      SetNextAnimTriggerTime();
    }
  } else {
    // We are not currently listening for beats, so just manually extrapolate the next beat time.
    _dVars.nextBeatTime_sec += _dVars.beatPeriod_sec;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::StopBackpackLights()
{
  auto& blc = GetBEI().GetBackpackLightComponent();
  blc.ClearAllBackpackLightConfigs();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::SetListeningForBeats(const bool listenForBeats)
{
  if (listenForBeats == _dVars.listeningForBeats) {
    return;
  }
  
  PRINT_CH_INFO("Behaviors", "BehaviorDanceToTheBeat.SetListeningForBeats.Set", "%d", listenForBeats);
  // Register/unregister an OnBeat callback if necessary
  auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  if (listenForBeats) {
    if (_dVars.onBeatCallbackId < 0) {
      _dVars.onBeatCallbackId = beatDetector.RegisterOnBeatCallback(std::bind(&BehaviorDanceToTheBeat::OnBeat, this));
    }
  } else if (_dVars.onBeatCallbackId >= 0) {
    UnregisterOnBeatCallback();
  }
  
  _dVars.listeningForBeats = listenForBeats;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::UnregisterOnBeatCallback()
{
  if (_dVars.onBeatCallbackId > 0) {
    auto& beatDetector = GetBEI().GetBeatDetectorComponent();
    if (!beatDetector.UnregisterOnBeatCallback(_dVars.onBeatCallbackId)) {
      PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.UnregisterOnBeatCallback.FailedToUnregisterCallback",
                        "Failed to unregister OnBeat callback (id %d)", _dVars.onBeatCallbackId);
    }
  }
  _dVars.onBeatCallbackId = -1;
}
  

}
}
