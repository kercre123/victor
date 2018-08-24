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
#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/danceToTheBeatConfig.h"

#include "engine/components/backpackLights/engineBackpackLightComponent.h"

#include "clad/types/animationTrigger.h"
#include "clad/types/backpackAnimationTriggers.h"

namespace Anki {
namespace Vector {

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
  
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);

    // Check if this instance is a valid configuration
    bool CheckValid();
    
    // Is this a valid config?
    bool isValid = false;
    
    const bool useBackpackLights;
    
    BackpackAnimationTrigger backpackAnim = BackpackAnimationTrigger::Count;
    
    AnimationTrigger eyeHoldAnim    = AnimationTrigger::Count;
    AnimationTrigger getOutAnim     = AnimationTrigger::Count;
    
    std::vector<DanceSession> danceSessionConfigs;
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
    
    // If true, then we should play the getout animation if we quit the behavior due to "losing" the beat
    bool playGetoutIfBeatLost = false;
    
    // ID for registering OnBeat callback with BeatDetectorComponent. A
    // value of less than 0 indicates that no callback is registered.
    int onBeatCallbackId = -1;
    
    // The queue of animations to play while dancing
    std::deque<DanceAnimMetadata> danceAnims;
  };
  
  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void TransitionToDancing();
  
  void WhileDancing();
  
  void PlayNextDanceAnim();
  
  void SetNextAnimTriggerTime();
  
  void OnBeat();
  
  void StopBackpackLights();
  
  void SetListeningForBeats(const bool listenForBeats);
  
  void UnregisterOnBeatCallback();
  
};


} // namespace Vector
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorDanceToTheBeat_H__

