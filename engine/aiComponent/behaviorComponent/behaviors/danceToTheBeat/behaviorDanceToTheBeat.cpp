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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/backpackLights/backpackLightAnimationContainer.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/random/randomGenerator.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Cozmo {

namespace {
  
  // This is a fixed latency correction factor. Animation
  // commands are sent to the anim process early by this
  // amount to account for messaging/tick latency/timing.
  const float kLatencyCorrectionTime_sec = 0.050f;
  
  const BackpackLightAnimation::BackpackAnimation beatBackpackLights = {
    .onColors               = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{60,60,60}},
    .offPeriod_ms           = {{1000,1000,1000}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{200,200,200}},
    .offset                 = {{0,0,0}}
  };
  
  // Maximum number of beats that can make up a single "dance phrase"
  const uint32_t kMaxBeatsPerDancePhrase = 32;
  
  const char* kCooldown_key = "cooldown_s";
  const char* kUseBackpackLights_key = "useBackpackLights";
  const char* kCanListenForBeatsDuringGetIn_key = "canListenForBeatsDuringGetIn";
  const char* kGetInAnim_key   = "getInAnim";
  const char* kGetOutAnim_key  = "getOutAnim";
  const char* kQuitAnim_key    = "quitAnim";
  const char* kIdleAnim_key    = "idleAnim";
  const char* kEyeHoldAnim_key = "eyeHoldAnim";
  const char* kDancePhraseConfigs_key  = "dancePhraseConfigs";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::BehaviorDanceToTheBeat(const Json::Value& config)
  : ICozmoBehavior(config)
  , _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
  MakeMemberTunable(_iConfig.cooldown_sec, kCooldown_key, ("Behavior" + GetDebugLabel()).c_str());
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
  : cooldown_sec(JsonTools::ParseFloat(config, kCooldown_key, debugName))
  , useBackpackLights(JsonTools::ParseBool(config, kUseBackpackLights_key, debugName))
  , canListenForBeatsDuringGetIn(JsonTools::ParseBool(config, kCanListenForBeatsDuringGetIn_key, debugName))
{
  JsonTools::GetCladEnumFromJSON(config, kGetInAnim_key,   getInAnim,   debugName);
  JsonTools::GetCladEnumFromJSON(config, kGetOutAnim_key,  getOutAnim,  debugName);
  JsonTools::GetCladEnumFromJSON(config, kQuitAnim_key,    quitAnim,    debugName);
  JsonTools::GetCladEnumFromJSON(config, kIdleAnim_key,    idleAnim,    debugName);
  JsonTools::GetCladEnumFromJSON(config, kEyeHoldAnim_key, eyeHoldAnim, debugName);
  
  const auto& animConfig = config[kDancePhraseConfigs_key];
  ANKI_VERIFY(!animConfig.isNull(), "BehaviorDanceToTheBeat.InstanceConfig.NullDancePhraseConfig", "");
  for (const auto& json : animConfig) {
    DancePhraseConfig dancePhraseConfig;
    dancePhraseConfig.minBeats    = JsonTools::ParseUInt32(json, "minBeats", debugName);
    dancePhraseConfig.maxBeats    = JsonTools::ParseUInt32(json, "maxBeats", debugName);
    dancePhraseConfig.multipleOf  = JsonTools::ParseUInt32(json, "multipleOf", debugName);
    dancePhraseConfig.canListenForBeats = JsonTools::ParseBool(json, "canListenForBeats", debugName);
    std::vector<std::string> tmp;
    if (JsonTools::GetVectorOptional(json, "anims", tmp)) {
      for (auto& animStr : tmp) {
        dancePhraseConfig.anims.emplace_back(std::move(animStr),
                                             dancePhraseConfig.canListenForBeats);
      }
    }
    dancePhraseConfigs.push_back(std::move(dancePhraseConfig));
  }
  
  ANKI_VERIFY(CheckValid(), "BehaviorDanceToTheBeat.InstanceConfig.InvalidConfig", "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::InstanceConfig::CheckValid()
{
  isValid = true;
  if (dancePhraseConfigs.empty()) {
    isValid = false;
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.InstanceConfig.CheckValid.NoDancePhraseConfigs",
                        "Invalid config - no dance phrase config entries");
  }
  bool prevCanListenForBeats = canListenForBeatsDuringGetIn;
  for (const auto& config : dancePhraseConfigs) {
    if (config.anims.empty()) {
      isValid = false;
      PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.InstanceConfig.CheckValid.NoAnimsInConfig",
                          "Invalid config - no animations specified in dance phrase");
    }
    // minBeats and maxBeats must be valid and multiples of multipleOf
    const bool beatsValid = (config.maxBeats <= kMaxBeatsPerDancePhrase) &&
                            (config.minBeats <= config.maxBeats) &&
                            ((config.minBeats % config.multipleOf) == 0) &&
                            ((config.maxBeats % config.multipleOf) == 0);
    if (!beatsValid) {
      isValid = false;
      PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.InstanceConfig.CheckValid.InvalidDancePhraseConfig",
                          "Invalid minBeats, maxBeats, or multipleOf (minBeats %d, maxBeats %d, multipleOf %d). Max allowed %d.",
                          config.minBeats, config.maxBeats, config.multipleOf, kMaxBeatsPerDancePhrase);
    }

    const bool canListenForBeatsThisPhrase = config.canListenForBeats;
    if (canListenForBeatsThisPhrase &&
        !prevCanListenForBeats) {
      isValid = false;
      PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.InstanceConfig.CheckValid.InvalidListenForBeats",
                          "CanListenForBeats is true for this phrase, but a previous canListenForBeats was false. Not allowed! "
                          "Can only go from 'listening' to 'not listening' and never the other way around");
    }
    prevCanListenForBeats = canListenForBeatsThisPhrase;
  }
  
  return isValid;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCooldown_key,
    kUseBackpackLights_key,
    kCanListenForBeatsDuringGetIn_key,
    kGetInAnim_key,
    kGetOutAnim_key,
    kQuitAnim_key,
    kIdleAnim_key,
    kEyeHoldAnim_key,
    kDancePhraseConfigs_key,
  };
  expectedKeys.insert(std::begin(list), std::end(list));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::WantsToBeActivatedBehavior() const
{
  // Check cooldown using BaseStation timer
  const auto nowBasestationTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool canRun = (_lastRunningBasestationTime_sec <= 0.f) ||
                      (nowBasestationTime_sec - _lastRunningBasestationTime_sec > _iConfig.cooldown_sec);
  return canRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::InitBehavior()
{
  // Reach into the animation container and grab the "beat delay"
  // for each animation.
  for (auto& dancePhraseConfig : _iConfig.dancePhraseConfigs) {
    for (auto it = dancePhraseConfig.anims.begin() ; it < dancePhraseConfig.anims.end() ; ) {
      if (!ANKI_VERIFY(GetAnimationBeatDelay_sec(it->animName, it->beatDelay_sec),
                       "BehaviorDanceToTheBeat.InitBehavior.FailedGettingAnimBeatDelay",
                       "Failed to retrieve the 'beat delay' from animation %s. Removing from the list of anims to play.",
                       it->animName.c_str())) {
        it = dancePhraseConfig.anims.erase(it);
      } else {
        ++it;
      }
    }
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
  
  // grab the next beat time here and wait to play the anim until the appropriate time
  auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
  _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();
  
  // For each dance phrase config entry, randomly draw the specified number
  // of animations from the list and add them to the queue.
  const auto& rng = GetRNG();
  for (const auto& config : _iConfig.dancePhraseConfigs) {
    // generate a random number in [minBeats, maxBeats] that is a multiple of 'multipleOf'
    int nAnims = config.multipleOf * rng.RandIntInRange(config.minBeats / config.multipleOf,
                                                        config.maxBeats / config.multipleOf);
    BOUNDED_WHILE(kMaxBeatsPerDancePhrase, nAnims-- > 0) {
      // randomly draw from the anims list
      const auto index = static_cast<size_t>(rng.RandIntInRange(0, (int) config.anims.size() - 1));
      const auto& anim = config.anims[index];
      _dVars.animsToPlay.push(anim);
    }
  }
  
  _dVars.listeningForBeats = _iConfig.canListenForBeatsDuringGetIn;
  
  // Register an OnBeat callback with BeatDetectorComponent if we
  // are to listen for beats during the behavior:
  if (_dVars.listeningForBeats) {
    _dVars.onBeatCallbackId = beatDetector.RegisterOnBeatCallback(std::bind(&BehaviorDanceToTheBeat::OnBeat, this));
  }
  
  // Start off the behavior by playing the get-in anim
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.getInAnim),
                      [this](){
                        SetNextAnimTriggerTime();
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  _lastRunningBasestationTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  // If we are currently listening for beats, double check that a beat
  // is still detected. If not, we should bail out of the behavior.
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  if (_dVars.listeningForBeats &&
      !beatDetector.IsBeatDetected()) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.BehaviorUpdate.BeatNoLongerDetected",
                        "Cancelling behavior since we are currently listening for beats "
                        "and BeatDetectorComponent says beat is no longer detected");
    CancelSelf();
    return;
  }
  
  // If we're not listening for beats, we need to call OnBeat() 'manually'
  // when the next 'extrapolated' beat occurs.
  if (!_dVars.listeningForBeats) {
    if (now_sec > _dVars.nextBeatTime_sec) {
      OnBeat();
    }
    // Unregister the OnBeat callback since we no longer need it
    UnregisterOnBeatCallback();
  }
  
  if (_dVars.nextAnimTriggerTime_sec > 0.f &&
      now_sec >= _dVars.nextAnimTriggerTime_sec) {
    // Time to play the next animation in the queue
    DEV_ASSERT(!_dVars.animsToPlay.empty(), "BehaviorDanceToTheBeat.BehaviorUpdate.NoAnimsInQueue");
    const auto& thisAnim = _dVars.animsToPlay.front();
    auto* animAction = new PlayAnimationAction(thisAnim.animName);
    // If this animation is not marked "canListenForBeats", then stop listening for beats
    if (!thisAnim.canListenForBeats) {
      _dVars.listeningForBeats = false;
    }
    _dVars.animsToPlay.pop();
    if (_dVars.animsToPlay.empty()) {
      // No more animations to play after this one. Play this
      // animation then the getout.
      DelegateNow(animAction, [this](){
        DelegateIfInControl(new TriggerAnimationAction(_iConfig.getOutAnim));
      });
      _dVars.nextAnimTriggerTime_sec = -1.f;
    } else {
      // Delegate to this animation, and play the eye hold indefinitely after that
      DelegateNow(animAction, [this](){
        DelegateIfInControl(new TriggerAnimationAction(_iConfig.eyeHoldAnim, 0));
      });
      SetNextAnimTriggerTime();
    }
  }
  
  // If we have no animation queued and we're not currently doing
  // anything, it's time to quit.
  if (!IsControlDelegated() &&
      _dVars.nextAnimTriggerTime_sec <= 0.f) {
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorDeactivated()
{
  StopBackpackLights();
  UnregisterOnBeatCallback();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::SetNextAnimTriggerTime()
{
  if (_dVars.animsToPlay.empty()) {
    _dVars.nextAnimTriggerTime_sec = -1.f;
    DEV_ASSERT(false, "BehaviorDanceToTheBeat.SetNextAnimTriggerTime.NoAnimsInQueue");
    return;
  }
  
  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  const auto& nextAnim = _dVars.animsToPlay.front();
  
  // Must start the animation beatDelay_sec before the next beat. Also include
  // the latency correction factor to account for messaging/tick times, etc.
  _dVars.nextAnimTriggerTime_sec = _dVars.nextBeatTime_sec - nextAnim.beatDelay_sec - kLatencyCorrectionTime_sec;
  
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
  
  PRINT_NAMED_INFO("BehaviorDanceToTheBeat.OnBeat",
                   "OnBeat() called. Currently%s listening for new beats. Current universal time (sec): %.3f",
                   _dVars.listeningForBeats ? "" : " NOT",
                   Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  if (_iConfig.useBackpackLights) {
    StopBackpackLights();
    auto& blc = GetBEI().GetBackpackLightComponent();
    blc.StartLoopingBackpackAnimation(beatBackpackLights,
                                   BackpackLightSource::Behavior,
                                   _dVars.backpackDataRef);
  }
  
  // If we're currently listening for beats, then this means
  // we have new information from the BeatDetectorComponent.
  if (_dVars.listeningForBeats) {
    // Update some stuff now that we have new beat information from
    // BeatDetectorComponent.
    const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
    _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
    _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();
    
    // Update the next animation trigger time if we still have
    // animations in the queue.
    if (!_dVars.animsToPlay.empty()) {
      SetNextAnimTriggerTime();
    }
  } else {
    // We are not currently listening for beats, so just manually
    // extrapolate the next beat time.
    _dVars.nextBeatTime_sec += _dVars.beatPeriod_sec;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::StopBackpackLights()
{
  auto& blc = GetBEI().GetBackpackLightComponent();
  if (_dVars.backpackDataRef.IsValid() &&
      !blc.StopLoopingBackpackAnimation(_dVars.backpackDataRef)) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.StopBackpackLights.FailedStoppingBackpackLights",
                        "Failed to stop backpack lights");
  }
}

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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::GetAnimationBeatDelay_sec(const std::string& animName, float& beatDelay_sec)
{
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if (animContainer == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.GetAnimationBeatDelay_sec.NullAnimContainer",
                      "Null CannedAnimationContainer!");
    return false;
  }
  
  const auto* anim = animContainer->GetAnimation(animName);
  if (anim == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.GetAnimationBeatDelay_sec.AnimNotFound",
                      "Animation with name %s not found in CannedAnimationContainer",
                      animName.c_str());
    return false;
  }
  
  const auto& eventTrack = anim->GetTrack<EventKeyFrame>();
  const auto* firstEventKeyframe = eventTrack.GetFirstKeyFrame();
  const auto* lastEventKeyframe = eventTrack.GetLastKeyFrame();
  if (eventTrack.IsEmpty() ||
      firstEventKeyframe == nullptr ||
      lastEventKeyframe == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.GetAnimationBeatDelay_sec.EmptyEventTrack",
                      "Empty event track or null event keyframe found for anim with name %s",
                      animName.c_str());
    return false;
  }
  if ((firstEventKeyframe != lastEventKeyframe) ||
      (firstEventKeyframe->GetAnimEvent() != AnimEvent::DANCE_BEAT_SYNC)) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.GetAnimationBeatDelay_sec.InvalidEventTrack",
                      "anim %s: Should have only one event keyframe, and it should be of type DANCE_BEAT_SYNC",
                      animName.c_str());
    return false;
  }

  beatDelay_sec = Util::MilliSecToSec(static_cast<float>(firstEventKeyframe->GetTriggerTime_ms()));
  return true;
}
  

}
}

