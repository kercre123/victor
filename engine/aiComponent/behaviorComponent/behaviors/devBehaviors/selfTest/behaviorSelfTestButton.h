/**
 * File: behaviorSelfTestButton.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Waits for the backpack button to be pressed
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestButton_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestButton_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestButton : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestButton(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void          OnBehaviorDeactivated() override;

private:

  void WaitToBeOnTreads();
  
  bool _buttonStartedPressed = false;
  bool _buttonPressed = false;

};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestButton_H__
