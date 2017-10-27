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
namespace Cozmo {

class BehaviorPlaypenMotorCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenMotorCalibration(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual BehaviorStatus InternalUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  bool _liftCalibrated = false;
  bool _headCalibrated = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__
