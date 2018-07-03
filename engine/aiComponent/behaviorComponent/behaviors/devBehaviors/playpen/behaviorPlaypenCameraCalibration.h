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
#include "clad/types/imageTypes.h"

namespace Anki {
namespace Cozmo {
  
namespace ExternalInterface {
struct RobotObservedObject;
}

class BehaviorPlaypenCameraCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenCameraCalibration(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }

  virtual Result         OnBehaviorActivatedInternal()   override;
  virtual PlaypenStatus PlaypenUpdateInternal() override;
  virtual void           OnBehaviorDeactivated()   override;
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event) override;
    
private:

  void HandleCameraCalibration(const CameraCalibration& calibMsg);
  void HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg);

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
