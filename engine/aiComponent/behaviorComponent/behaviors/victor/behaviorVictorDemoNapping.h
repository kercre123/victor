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

  virtual bool ShouldRunWhileOnCharger() const override { return true;}
  virtual bool CarryingObjectHandledInternally() const override {return false;}

  virtual bool CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true;}
private:

  void TransitionToSleeping(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBoutOfStirring(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlayStirAnim(BehaviorExternalInterface& behaviorExternalInterface);

  bool _animIsPlaying = false;

  int _numRemainingInBout = 0;
};

}
}


#endif
