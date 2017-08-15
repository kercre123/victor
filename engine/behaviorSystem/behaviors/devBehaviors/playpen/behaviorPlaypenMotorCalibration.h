/**
 * File: behaviorPlaypenMotorCalibration.h
 *
 * Author: Al Chaussee
 * Created: 07/26/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenMotorCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenMotorCalibration(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  bool _liftCalibrated = false;
  bool _headCalibrated = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenMotorCalibration_H__
