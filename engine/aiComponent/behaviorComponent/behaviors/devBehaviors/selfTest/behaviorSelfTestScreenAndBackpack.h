/**
 * File: behaviorSelfTestScreenAndBackpack.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestScreenAndBackpack_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestScreenAndBackpack_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Cozmo {

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

  bool _buttonPressed = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestScreenAndBackpack_H__
