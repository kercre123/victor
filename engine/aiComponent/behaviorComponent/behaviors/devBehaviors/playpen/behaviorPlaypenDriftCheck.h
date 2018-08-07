/**
 * File: behaviorPlaypenDriftCheck.h
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Vector {

class BehaviorPlaypenDriftCheck : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenDriftCheck(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }

  virtual Result OnBehaviorActivatedInternal()   override;
  virtual PlaypenStatus PlaypenUpdateInternal() override;
  virtual void           OnBehaviorDeactivated()   override;
    
private:
  
  void TransitionToStartDriftCheck();
  void CheckDrift();
  
  // Angle at the start of drift check
  Radians _startingRobotOrientation = 0;
  
  // Whether or not the drift check is complete
  bool _driftCheckComplete = false;
  
  // Struct to hold imu temperature readings before and after drift check
  IMUTempDuration _imuTemp;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
