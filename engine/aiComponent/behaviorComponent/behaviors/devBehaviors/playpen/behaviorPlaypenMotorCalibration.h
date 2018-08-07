/**
 * File: behaviorPlaypenMotorCalibration.h
 *
 * Author: Al Chaussee
 * Created: 07/26/17
 *
 * Description: Calibrates Cozmo's motors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Vector {

class BehaviorPlaypenMotorCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenMotorCalibration(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }

  virtual Result         OnBehaviorActivatedInternal()   override;
  virtual PlaypenStatus PlaypenUpdateInternal() override;
  virtual void           OnBehaviorDeactivated()   override;
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event) override;
    
private:
  
  bool _liftCalibrated = false;
  bool _headCalibrated = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__
