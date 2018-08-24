/**
 * File: danceToTheBeatConfig.cpp
 *
 * Author: Matt Michini
 * Created: 2018-08-07
 *
 * Description: Containers for "dance to the beat" configuration metadata
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/danceToTheBeatConfig.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/random/randomGenerator.h"

namespace Anki {
namespace Vector {

DanceAnimMetadata::DanceAnimMetadata(std::string&& animStr)
  : _animName(std::move(animStr))
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DanceAnimMetadata::ComputeBeatDelay_sec(const CannedAnimationContainer& animContainer)
{
  const auto* anim = animContainer.GetAnimation(_animName);
  if (anim == nullptr) {
    PRINT_NAMED_ERROR("DanceAnimMetadata.GetAnimationBeatDelay_sec.AnimNotFound",
                      "Animation with name %s not found in CannedAnimationContainer",
                      _animName.c_str());
    return false;
  }
  
  const auto& eventTrack = anim->GetTrack<EventKeyFrame>();
  const auto* firstEventKeyframe = eventTrack.GetFirstKeyFrame();
  const auto* lastEventKeyframe = eventTrack.GetLastKeyFrame();
  if (eventTrack.IsEmpty() ||
      firstEventKeyframe == nullptr ||
      lastEventKeyframe == nullptr) {
    PRINT_NAMED_ERROR("DanceAnimMetadata.GetAnimationBeatDelay_sec.EmptyEventTrack",
                      "Empty event track or null event keyframe found for anim with name %s",
                      _animName.c_str());
    return false;
  }
  if ((firstEventKeyframe != lastEventKeyframe) ||
      (firstEventKeyframe->GetAnimEvent() != AnimEvent::DANCE_BEAT_SYNC)) {
    PRINT_NAMED_ERROR("DanceAnimMetadata.GetAnimationBeatDelay_sec.InvalidEventTrack",
                      "anim %s: Should have only one event keyframe, and it should be of type DANCE_BEAT_SYNC",
                      _animName.c_str());
    return false;
  }
  
  _beatDelay_sec = Util::MilliSecToSec(static_cast<float>(firstEventKeyframe->GetTriggerTime_ms()));
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DancePhrase::DancePhrase(const Json::Value& json)
{
  const std::string debugName = "DancePhrase";
  _minBeats          = JsonTools::ParseUInt32(json, "minBeats", debugName);
  _maxBeats          = JsonTools::ParseUInt32(json, "maxBeats", debugName);
  _multipleOf        = JsonTools::ParseUInt32(json, "multipleOf", debugName);
  std::vector<std::string> tmp;
  const bool hasAnims = JsonTools::GetVectorOptional(json, "anims", tmp);
  DEV_ASSERT(hasAnims && !tmp.empty(), "DancePhrase.Ctor.NoAnims");
  for (auto& anim : tmp) {
    _anims.emplace_back(std::move(anim));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DancePhrase::IsValid() const
{
  bool isValid = true;
  
  if (_anims.empty()) {
    isValid = false;
    PRINT_NAMED_WARNING("DancePhrase.IsValid.NoAnimsInPhrase",
                        "Invalid config - no animations specified in dance phrase");
  }
  // minBeats and maxBeats must be valid and multiples of multipleOf
  const bool beatsValid = (_minBeats <= _maxBeats) &&
                          ((_minBeats % _multipleOf) == 0) &&
                          ((_maxBeats % _multipleOf) == 0);
  if (!beatsValid) {
    isValid = false;
    PRINT_NAMED_WARNING("DancePhrase.IsValid.InvalidDancePhraseConfig",
                        "Invalid minBeats, maxBeats, or multipleOf (minBeats %d, maxBeats %d, multipleOf %d)",
                        _minBeats, _maxBeats, _multipleOf);
  }

  return isValid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DancePhrase::Init(const CannedAnimationContainer& animContainer)
{
  for (auto& anim : _anims) {
    if (!anim.ComputeBeatDelay_sec(animContainer)) {
      return false;
    }
  }
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DanceAnimMetadata DancePhrase::GetRandomAnim() const
{
  if (_anims.empty()) {
    DEV_ASSERT(false, "DancePhrase.GetRandomAnim.NoAnims");
    return DanceAnimMetadata{"invalid"};
  }
 
  // Draw a random anim from the list
  static Util::RandomGenerator rng;
  const auto index = static_cast<size_t>(rng.RandIntInRange(0, (int) _anims.size() - 1));
  return _anims[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DancePhrase::SetCanListenForBeats(const bool b)
{
  for (auto& anim : _anims) {
    anim.SetCanListenForBeats(b);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DancePhrase::SetPlayGetoutIfInterrupted(const bool b)
{
  for (auto& anim : _anims) {
    anim.SetPlayGetoutIfInterrupted(b);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DanceSession::DanceSession(const Json::Value& json)
{
  const std::string debugName = "DanceSession";
  const bool canListenForBeats = JsonTools::ParseBool(json, "canListenForBeats", debugName);
  const bool playGetoutIfInterrupted = JsonTools::ParseBool(json, "playGetoutIfInterrupted", debugName);
  
  const auto& dancePhrases = json["dancePhrases"];
  DEV_ASSERT(!dancePhrases.isNull() && dancePhrases.isArray() && !dancePhrases.empty(),
             "DanceSession.DanceSession.EmptyOrInvalidDancePhraseList");
  
  for (const auto& dancePhraseJson : dancePhrases) {
    _dancePhrases.emplace_back(dancePhraseJson);
    _dancePhrases.back().SetCanListenForBeats(canListenForBeats);
    _dancePhrases.back().SetPlayGetoutIfInterrupted(playGetoutIfInterrupted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DanceSession::IsValid() const
{
  if (_dancePhrases.empty()) {
    PRINT_NAMED_WARNING("DanceSession.IsValid.NoDancePhrases", "");
    return false;
  }
  
  for (const auto& phrase : _dancePhrases) {
    if (!phrase.IsValid()) {
      return false;
    }
  }
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DanceSession::Init(const CannedAnimationContainer& animContainer)
{
  for (auto& phrase : _dancePhrases) {
    if (!phrase.Init(animContainer)) {
      return false;
    }
  }
  
  _initialized = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::deque<DanceAnimMetadata> DanceSession::GenerateAnimSequence() const
{
  std::deque<DanceAnimMetadata> animSequence;
  if (!_initialized || _dancePhrases.empty()) {
    PRINT_NAMED_ERROR("DanceSession.GenerateAnimSequence.NotInitialized",
                      "initialized %d, _dancePhrases.size() = %zu",
                      _initialized, _dancePhrases.size());
    return animSequence;
  }
  
  static Util::RandomGenerator rng;
  for (const auto& phrase : _dancePhrases) {
    // generate a random number in [minBeats, maxBeats] that is a multiple of 'multipleOf'
    const int nAnims = phrase.GetMultipleOf() * rng.RandIntInRange(phrase.GetMinBeats() / phrase.GetMultipleOf(),
                                                                   phrase.GetMaxBeats() / phrase.GetMultipleOf());
    // Draw nAnims animations from this phrase
    std::generate_n(std::back_inserter(animSequence),
                    nAnims,
                    [&]() { return phrase.GetRandomAnim(); });
  }
  
  return animSequence;
}
  
}
}

