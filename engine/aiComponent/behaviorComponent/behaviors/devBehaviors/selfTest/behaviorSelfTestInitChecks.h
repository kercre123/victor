/**
 * File: behaviorSelfTestInitChecks.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Performs basic checks for the self test
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestInitChecks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestInitChecks_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestInitChecks : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestInitChecks(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;

private:

  void TransitionToChecks();
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestInitChecks_H__
