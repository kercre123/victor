/**
 * File: behaviorSelfTestScreenAndBackpack.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Displays the same color on the backpack and screen to test lights
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestScreenAndBackpack_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestScreenAndBackpack_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestScreenAndBackpack : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestScreenAndBackpack(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void          OnBehaviorDeactivated() override;

private:

  void TransitionToButtonCheck();
  
  bool _buttonStartedPressed = false;
  bool _buttonPressed = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestScreenAndBackpack_H__
