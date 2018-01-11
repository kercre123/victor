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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenCameraCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenCameraCalibration(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual PlaypenStatus PlaypenUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:

  void HandleCameraCalibration(BehaviorExternalInterface& behaviorExternalInterface, const CameraCalibration& calibMsg);
  void HandleRobotObservedObject(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);

  // Whether or not we are currently computing camera calibration
  bool _computingCalibration = false;

  // Whether or not we are currently seeing the target
  bool _seeingTarget         = false;

  // Whether or not we are waiting to store an image for calibration
  bool _waitingToStoreImage  = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__
