/**
 * File: BehaviorAnimSequence
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation or animation sequence
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAnimSequence_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAnimSequence_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iSubtaskListener.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorAnimSequence : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAnimSequence(const Json::Value& config, bool triggerRequired = true);
  
public:
  
  virtual ~BehaviorAnimSequence();
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void AddListener(ISubtaskListener* listener) override;

  
  // Begin playing the animations
  void StartPlayingAnimations();
  void SetAnimSequence(const std::vector<AnimationTrigger>& animations){_iConfig.animTriggers = animations;}

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = _iConfig.activatableOnCharger;
  }
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual bool WantsToBeActivatedAnimSeqInternal() const { return true;}
  
  virtual void OnBehaviorActivated() override;

  // Returns an action that will play all animations in the class the appropriate number of times for one loop
  IActionRunner* GetAnimationAction();
  // Returns true if multiple animations will be played as a loop _numLoops times
  // Returns false if a single animation will play _numLoops times
  bool IsSequenceLoop();

private:
  struct InstanceConfig {
    InstanceConfig();
    bool activatableOnCharger;
    int  numLoops;
    // Class supports playing a series of animation triggers OR a series of animations by name
    // BUT NOT BOTH AT THE SAME TIME!!!!
    std::vector<AnimationTrigger> animTriggers;
    std::vector<std::string>      animationNames;
  };

  struct DynamicVariables {
    DynamicVariables();
    // for sequences it's not per animation, but per sequence, so we have to wait till the last one
    int sequenceLoopsDone; 
    std::set<ISubtaskListener*> listeners;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  // queues actions to play all the animations specified in animTriggers
  void StartSequenceLoop();
  
  // We call our listeners whenever an animation completes
  void CallToListeners();

  // internal helper to properly handle locking extra tracks if needed
  u8 GetTracksToLock() const;  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAnimSequence_H__
