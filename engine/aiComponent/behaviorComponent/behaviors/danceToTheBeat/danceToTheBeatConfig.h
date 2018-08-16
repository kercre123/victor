/**
 * File: danceToTheBeatConfig.h
 *
 * Author: Matt Michini
 * Created: 2018-08-07
 *
 * Description: Containers for "dance to the beat" configuration metadata
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_BehaviorComponent_DanceToTheBeatConfig_H__
#define __Engine_BehaviorComponent_DanceToTheBeatConfig_H__

#include <deque>
#include <string>
#include <vector>

namespace Json {
  class Value;
}

namespace Anki {
namespace Vector {
  
class CannedAnimationContainer;
  
// DanceAnimMetadata stores information about dance animations. Each dance animation is short, and meant to last only a
// single beat. The beatDelay_sec indicates when in the animation the musical beat should land, and is determined by an
// event keyframe in the animation itself.
class DanceAnimMetadata
{
public:
  DanceAnimMetadata(std::string&& animStr, const bool canListenForBeats);
  
  const std::string& GetAnimName() const { return _animName; }
  
  // Is it safe to listen for new tempos/beat detection while this animation is playing?
  bool CanListenForBeats() const { return _canListenForBeats; }
  
  float GetBeatDelay_sec() const {return _beatDelay_sec; }
  
  // Populates beatDelay_sec with the time into the given animation where the beat should land. For example, a value of
  // 0.100 would mean that the musical beat should fall 100 ms into the animation. Returns true if we successfully
  // found the beat delay, emits an error and returns false otherwise.
  bool ComputeBeatDelay_sec(const CannedAnimationContainer& animContainer);
  
private:
  std::string _animName;
  bool _canListenForBeats = false;
  float _beatDelay_sec = 0.f;
};
  
  
// A DancePhrase is made up of one or more possible dance animations that can be strung together and played on
// sequential musical beats. DancePhraseConfig specifies the rules by which dance phrases are generated when the
// behavior is run.
//
// When the behavior begins, animations are randomly drawn from the list in accordance with the min/max beats. The
// number of animations that make up the phrase is random, but is always between 'minBeats' and 'maxBeats', and is
// always a multiple of 'multipleOf'.
class DancePhrase
{
public:
  // Construct from json
  DancePhrase(const Json::Value& json);
  
  bool IsValid() const;
  
  bool Init(const CannedAnimationContainer& animContainer);
  
  // Draw a random animation from the list
  DanceAnimMetadata GetRandomAnim() const;
  
  bool CanListenForBeats() const { return _canListenForBeats; }
  
  uint32_t GetMinBeats() const   { return _minBeats; }
  uint32_t GetMaxBeats() const   { return _maxBeats; }
  uint32_t GetMultipleOf() const { return _multipleOf; }
  
private:
  uint32_t _minBeats    = 0;
  uint32_t _maxBeats    = 0;
  uint32_t _multipleOf  = 1;
  bool _canListenForBeats = false;
  std::vector<DanceAnimMetadata> _anims;
};
  
  
// A DanceSession is made up of one or more DancePhrases. It corresponds to a full, choreographed dance 'event'.
class DanceSession
{
public:
  // Construct from json
  DanceSession(const Json::Value& json);
  
  bool IsValid() const;
  
  bool Init(const CannedAnimationContainer& animContainer);
  
  // Generate a dance animation sequence (using randomness)
  std::deque<DanceAnimMetadata> GenerateAnimSequence() const;
  
private:

  std::vector<DancePhrase> _dancePhrases;
  bool _initialized = false;
};


} // namespace Vector
} // namespace Anki


#endif // __Engine_BehaviorComponent_DanceToTheBeatConfig_H__

