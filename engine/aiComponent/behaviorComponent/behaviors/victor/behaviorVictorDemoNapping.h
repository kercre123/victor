/**
 * File: behaviorVictorDemoNapping.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-01
 *
 * Description: State to nap and stir occasionally in sleep
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorDemoNapping_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorDemoNapping_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorVictorDemoNapping : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorVictorDemoNapping(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{}


  virtual bool CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true;}
private:

  void TransitionToSleeping(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBoutOfStirring(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlayStirAnim(BehaviorExternalInterface& behaviorExternalInterface);

  // helper to "wait" without doing procedural face motions and then run a callback
  void HoldFaceForTime(BehaviorExternalInterface& behaviorExternalInterface,
                       const float waitTime_s,
                       void(BehaviorVictorDemoNapping::*callback)(BehaviorExternalInterface& behaviorExternalInterface));
  void LoopHoldFace(BehaviorExternalInterface& behaviorExternalInterface,
                    void(BehaviorVictorDemoNapping::*callback)(BehaviorExternalInterface& behaviorExternalInterface));

  bool _animIsPlaying = false;

  int _numRemainingInBout = 0;

  float _stopHoldingFaceAtTime_s = 0.0f;
};

}
}


#endif
