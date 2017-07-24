/**
 * File: behaviorDance.h
 *
 * Author: Al Chaussee
 * Created: 05/11/17
 *
 * Description: Behavior to have Cozmo dance
 *              Plays dancing animation, triggers music from device, and
 *              plays cube light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenCameraCalibration : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenCameraCalibration(Robot& robot, const Json::Value& config);
  
public:

  virtual BehaviorStatus GetResults() override;
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenCameraCalibration_H__
