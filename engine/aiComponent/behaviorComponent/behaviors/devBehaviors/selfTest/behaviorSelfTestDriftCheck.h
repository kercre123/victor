/**
 * File: behaviorSelfTestDriftCheck.h
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestDriftCheck_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestDriftCheck_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestDriftCheck : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestDriftCheck(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }

  virtual Result OnBehaviorActivatedInternal()   override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void           OnBehaviorDeactivated()   override;
    
private:
  
  void TransitionToStartDriftCheck();
  void CheckDrift();
  
  // Angle at the start of drift check
  Radians _startingRobotOrientation = 0;
  
  // Whether or not the drift check is complete
  bool _driftCheckComplete = false;

};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestDriftCheck_H__
