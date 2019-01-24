/**
 * File: behaviorSelfTestTouch.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Waits for the touch sensor to be held for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestTouch_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestTouch_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestTouch : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestTouch(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void          OnBehaviorDeactivated() override;

private:
  
  bool _buttonPressed = false;
  int  _heldCountDown = SelfTestConfig::kTouchSensorDuration_sec;
  bool _addTimer      = false;
  bool _calibrated    = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestTouch_H__
