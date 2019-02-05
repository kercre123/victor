/**
 * File: behaviorSelfTestDockWithCharger.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Robot docks with the charger
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestDockWithCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestDockWithCharger_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestDockWithCharger : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestDockWithCharger(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual void          OnBehaviorDeactivated() override;

private:

  void TransitionToOnChargerChecks();
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestDockWithCharger_H__
