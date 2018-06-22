/**
 * File: behaviorDanceToTheBeat.h
 *
 * Author: Matt Michini
 * Created: 2018-05-07
 *
 * Description: Dance to a beat detected by the microphones/beat detection algorithm
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Behaviors_BehaviorDanceToTheBeat_H__
#define __Engine_Behaviors_BehaviorDanceToTheBeat_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/components/backpackLights/backpackLightComponent.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BehaviorDanceToTheBeat : public ICozmoBehavior
{
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDanceToTheBeat(const Json::Value& config);
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  };
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  using base = ICozmoBehavior;
  
  // DanceAnimMetadata stores information about dance animations.
  // Each dance animation is short, and meant to last only a single
  // beat. The beatDelay_sec indicates when in the animation the
  // musical beat should land, and is determined by an event keyframe
  // in the animation itself.
  struct DanceAnimMetadata {
    DanceAnimMetadata(std::string&& name, const bool canListenForBeats)
      : animName(std::move(name))
      , canListenForBeats(canListenForBeats) {}
    std::string animName;
    float beatDelay_sec = 0.f;
    bool canListenForBeats = false;
  };
  
  // A DancePhrase is made up of one or more possible dance animations
  // that can be strung together and played on sequential musical beats.
  // DancePhraseConfig specifies the rules by which dance phrases are
  // generated when the behavior is run.
  //
  // When the behavior begins, animations are randomly drawn from the
  // list in accordance with the min/max beats. The number of animations
  // that make up the phrase is random, but is always between 'minBeats'
  // and 'maxBeats', and is always a multiple of 'multipleOf'.
  struct DancePhraseConfig {
    uint32_t minBeats    = 0;
    uint32_t maxBeats    = 0;
    uint32_t multipleOf  = 1;
    bool canListenForBeats = false;
    std::vector<DanceAnimMetadata> anims;
  };
  
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);

    // Check if this instance is a valid configuration
    bool CheckValid();
    
    // Is this a valid config?
    bool isValid = false;
    
    float cooldown_sec = 0.f;
    
    const bool useBackpackLights;
    const bool canListenForBeatsDuringGetIn;
    
    AnimationTrigger getInAnim   = AnimationTrigger::Count;
    AnimationTrigger getOutAnim  = AnimationTrigger::Count;
    AnimationTrigger quitAnim    = AnimationTrigger::Count;
    AnimationTrigger idleAnim    = AnimationTrigger::Count;
    AnimationTrigger eyeHoldAnim = AnimationTrigger::Count;
    
    std::vector<DancePhraseConfig> dancePhraseConfigs;
  };
  
  struct DynamicVariables {
    float beatPeriod_sec = -1.f;
    float nextBeatTime_sec = -1.f;
    float nextAnimTriggerTime_sec = -1.f;
    
    // True if we are actively listening for new beats, during animations
    // that don't move motors for example. Normally false since the motors
    // themselves create too much noise for beat detection to continue
    // running during the behavior.
    bool listeningForBeats = false;
    
    // ID for registering OnBeat callback with BeatDetectorComponent. A
    // value of less than 0 indicates that no callback is registered.
    int onBeatCallbackId = -1;
    
    BackpackLightDataLocator backpackDataRef;
    
    // The queue of animations to play
    std::queue<DanceAnimMetadata> animsToPlay;
  };
  
  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void SetNextAnimTriggerTime();
  
  // Called when a beat occurs while the behavior is running
  void OnBeat();
  
  void StopBackpackLights();
  
  void UnregisterOnBeatCallback();
  
  // Populates beatDelay_sec with the time into the given animation where
  // the beat should land. For example, a value of 0.100 would mean that
  // the musical beat should fall 100 ms into the animation.
  //
  // Returns true if we successfully found the beat delay, emits an error
  // and returns false otherwise.
  bool GetAnimationBeatDelay_sec(const std::string& animName, float& beatDelay_sec);

  // Note, this time uses BasestationTimer, not UniversalTime
  float _lastRunningBasestationTime_sec = -1.f;
  
};


} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorDanceToTheBeat_H__

