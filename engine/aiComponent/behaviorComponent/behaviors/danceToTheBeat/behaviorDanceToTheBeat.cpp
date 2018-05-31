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
#include "engine/animations/animationContainers/backpackLightAnimationContainer.h"
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
  const BackpackLights beatBackpackLights = {
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
    DEV_ASSERT(!dancePhraseConfig.canListenForBeats, "BehaviorDanceToTheBeat.InstanceConfig.CanListenForBeatsUnsupported");
    std::vector<std::string> tmp;
    if (JsonTools::GetVectorOptional(json, "anims", tmp)) {
      for (auto& animStr : tmp) {
        dancePhraseConfig.anims.emplace_back(std::move(animStr));
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
  }
  
  return isValid;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCooldown_key,
    kUseBackpackLights_key,
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
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
  _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();
  
  // Push the get-in animation onto the queue
  std::string animNameStr;
  if (ANKI_VERIFY(GetAnimationFromTrigger(_iConfig.getInAnim, animNameStr), "BehaviorDanceToTheBeat.OnBehaviorActivated.NoAnimForGetInTrigger", "")) {
    _dVars.animsToPlay.push(std::move(animNameStr));
  }
  
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
  
  // Push the get-out animation onto the queue
  animNameStr.clear();
  if (ANKI_VERIFY(GetAnimationFromTrigger(_iConfig.getOutAnim, animNameStr), "BehaviorDanceToTheBeat.OnBehaviorActivated.NoAnimForGetOutTrigger", "")) {
    _dVars.animsToPlay.push(std::move(animNameStr));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  _lastRunningBasestationTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  if (now_sec > _dVars.nextBeatTime_sec) {
    OnBeat();
    _dVars.nextBeatTime_sec += _dVars.beatPeriod_sec;
  }
  
  if (!IsControlDelegated()) {
    if (IsAnimQueued()) {
      if (now_sec > _dVars.nextAnimTriggerTime_sec) {
        DelegateNow(new PlayAnimationAction(_dVars.animsToPlay.front().animName));
        _dVars.animsToPlay.pop();
        _dVars.nextAnimTriggerTime_sec = 0.f;
      }
    } else {
      if (_dVars.animsToPlay.empty()) {
        // There are no more animations to play, so end the behavior.
        CancelSelf();
      } else {
        // Queue up a new animation
        const auto& nextAnim = _dVars.animsToPlay.front();
        // Must start the animation beatDelay_sec before the next beat
        _dVars.nextAnimTriggerTime_sec = _dVars.nextBeatTime_sec - nextAnim.beatDelay_sec;
        // Make sure the animation trigger time is somewhere in the future
        BOUNDED_WHILE(1000, _dVars.nextAnimTriggerTime_sec < now_sec) {
          // warn?
          _dVars.nextAnimTriggerTime_sec += _dVars.beatPeriod_sec;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorDeactivated()
{
  StopBackpackLights();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBeat()
{
  if (!IsActivated()) {
    DEV_ASSERT(false, "BehaviorDanceToTheBeat.OnBeat.NotActivated");
    return;
  }
  
  if (_iConfig.useBackpackLights) {
    StopBackpackLights();
    auto& blc = GetBEI().GetBodyLightComponent();
    blc.StartLoopingBackpackLights(beatBackpackLights,
                                   BackpackLightSource::Behavior,
                                   _dVars.backpackDataRef);
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::StopBackpackLights()
{
  auto& blc = GetBEI().GetBodyLightComponent();
  if (_dVars.backpackDataRef.IsValid() &&
      !blc.StopLoopingBackpackLights(_dVars.backpackDataRef)) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.StopBackpackLights.FailedStoppingBackpackLights",
                        "Failed to stop backpack lights");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::GetAnimationBeatDelay_sec(const std::string& animName, float& beatDelay_sec)
{
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetValue<DataAccessorComponent>();
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::GetAnimationFromTrigger(const AnimationTrigger& animTrigger, std::string& animName)
{
  auto* context = GetBEI().GetRobotInfo().GetContext();
  if (context == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.GetAnimationFromTrigger.NullContext", "Null CozmoContext!");
    return false;
  }
  
  auto* dataLoader = context->GetDataLoader();
  if (dataLoader == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.GetAnimationFromTrigger.NullDataLoader", "Null data loader!");
    return false;
  }
  
  if (!dataLoader->HasAnimationForTrigger(animTrigger)) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.GetAnimationFromTrigger.NoAnimGroupForTrigger",
                        "No anim group found for trigger %s",
                        AnimationTriggerToString(animTrigger));
    return false;
  }
  
  const auto& animGroupName = dataLoader->GetAnimationForTrigger(animTrigger);
  const bool strictCooldown = false;
  const auto& animNameStr = GetBEI().GetAnimationComponent().GetAnimationNameFromGroup(animGroupName, strictCooldown);
  if (animNameStr.empty()) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.GetAnimationFromTrigger.NoAnimForGroup",
                        "No animation found for group name %s",
                        animGroupName.c_str());
    return false;
  }
  
  animName = animNameStr;
  return true;
}
  

}
}

