/**
 * File: behaviorSleeping.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-01
 *
 * Description: State to nap and stir occasionally in sleep
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSleeping_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSleeping_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorSleeping : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorSleeping(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual bool CanBeGentlyInterruptedNow() const override;

  virtual void OnBehaviorActivated() override;
  
  virtual void OnBehaviorDeactivated() override;

  virtual bool WantsToBeActivatedBehavior() const override {
    return true;}
private:

  void TransitionToSleeping();
  void TransitionToBoutOfStirring();
  void TransitionToPlayStirAnim();

  // helper to "wait" without doing procedural face motions and then run a callback
  void HoldFaceForTime(const float waitTime_s,
                       void(BehaviorSleeping::*callback)());


  struct InstanceConfig {
    bool shouldEnterPowerSave = true;
    bool shouldPlayEmergencyGetOut = true;
  };

  struct DynamicVariables {
    DynamicVariables() = default;
    
    bool animIsPlaying = false;
    int numRemainingInBout = 0;
    float stopHoldingFaceAtTime_s = 0.0f;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;


};

}
}


#endif
