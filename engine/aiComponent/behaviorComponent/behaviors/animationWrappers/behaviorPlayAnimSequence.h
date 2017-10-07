/**
 * File: BehaviorPlayAnimSequence
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation or animation sequence
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequence_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequence_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iSubtaskListener.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorPlayAnimSequence : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlayAnimSequence(const Json::Value& config, bool triggerRequired = true);
  
public:
  
  virtual ~BehaviorPlayAnimSequence();
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  virtual void AddListener(ISubtaskListener* listener) override;
  
  // Begin playing the animations
  void StartPlayingAnimations(BehaviorExternalInterface& behaviorExternalInterface);
  void SetAnimSequence(const std::vector<AnimationTrigger>& animations){_animTriggers = animations;}

protected:
  
  virtual bool IsRunnableAnimSeqInternal(BehaviorExternalInterface& behaviorExternalInterface) const { return true;}
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  // don't allow resume
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface) override { return RESULT_FAIL; }
    
  // ========== Members ==========
  
  std::vector<AnimationTrigger> _animTriggers;
  int _numLoops;
  int _sequenceLoopsDone; // for sequences it's not per animation, but per sequence, so we have to wait till the last one

private:

  // queues actions to play all the animations specified in _animTriggers
  void StartSequenceLoop(BehaviorExternalInterface& behaviorExternalInterface);
  
  // We call our listeners whenever an animation completes
  void CallToListeners(BehaviorExternalInterface& behaviorExternalInterface);
  
  std::set<ISubtaskListener*> _listeners;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequence_H__
