/**
 * File: behaviorSelfTestMotorCalibration.cpp
 *
 * Author: Al Chaussee
 * Created: 11/19/2018
 *
 * Description: Calibrates Vector's motors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestMotorCalibration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestMotorCalibration_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestMotorCalibration : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestMotorCalibration(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }

  virtual Result         OnBehaviorActivatedInternal()   override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void           OnBehaviorDeactivated()   override;
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event) override;
    
private:
  
  bool _liftCalibrated = false;
  bool _headCalibrated = false;
  
};

}
}

#endif
