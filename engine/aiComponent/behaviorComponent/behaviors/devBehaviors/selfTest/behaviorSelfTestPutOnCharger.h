/**
 * File: behaviorSelfTestPutOnCharger.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Prompts the user to put the robot on the charger
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestPutOnCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestPutOnCharger_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestPutOnCharger : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestPutOnCharger(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void          OnBehaviorDeactivated() override;

private:

  bool _isUpsideDown = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestPutOnCharger_H__
