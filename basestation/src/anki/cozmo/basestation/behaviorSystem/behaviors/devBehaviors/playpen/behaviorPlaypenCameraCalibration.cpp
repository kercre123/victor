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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenCameraCalibration.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenCameraCalibration::BehaviorPlaypenCameraCalibration(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  
}

BehaviorStatus BehaviorPlaypenCameraCalibration::GetResults()
{
  return (IsRunning() ? BehaviorStatus::Running : BehaviorStatus::Complete);
}

Result BehaviorPlaypenCameraCalibration::InitInternal(Robot& robot)
{
  return RESULT_OK;
}

void BehaviorPlaypenCameraCalibration::StopInternal(Robot& robot)
{
  
}

}
}

