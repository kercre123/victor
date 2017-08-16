/**
 * File: behaviorPlaypenCameraCalibration.h
 *
 * Author: Al Chaussee
 * Created: 07/25/17
 *
 * Description: Calibrates Cozmo's camera by using a single calibration target
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenCameraCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenCameraCalibration(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:

  void HandleCameraCalibration(Robot& robot, const CameraCalibration& calibMsg);
  void HandleRobotObservedObject(Robot& robot, const ExternalInterface::RobotObservedObject& msg);

  // Whether or not we are currently computing camera calibration
  bool        _computingCalibration        = false;

  // Whether or not we are currently seeing the target
  bool        _seeingTarget                = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__
