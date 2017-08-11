/**
 * File: behaviorPlaypenMotorCalibration.cpp
 *
 * Author: Al Chaussee
 * Created: 07/25/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenMotorCalibration.h"

#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenMotorCalibration::BehaviorPlaypenMotorCalibration(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({EngineToGameTag::MotorCalibration/*,
                    EngineToGameTag::MotorAutoEnabled*/});
}

void BehaviorPlaypenMotorCalibration::GetResultsInternal()
{
  
}

Result BehaviorPlaypenMotorCalibration::InternalInitInternal(Robot& robot)
{
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartMotorCalibration(true, true)));
  AddTimer(PlaypenConfig::kMotorCalibrationTimeout_ms,
           [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::MOTORS_UNCALIBRATED) });
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenMotorCalibration::InternalUpdateInternal(Robot& robot)
{
  if(_liftCalibrated && _headCalibrated)
  {
    SetResult(FactoryTestResultCode::SUCCESS);
    return BehaviorStatus::Complete;
  }
  
  return BehaviorStatus::Running;
}

void BehaviorPlaypenMotorCalibration::StopInternal(Robot& robot)
{
  _liftCalibrated = false;
  _headCalibrated = false;
}

void BehaviorPlaypenMotorCalibration::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  const EngineToGameTag tag = event.GetData().GetTag();
  if(tag == EngineToGameTag::MotorCalibration)
  {
    const auto& payload = event.GetData().Get_MotorCalibration();
    if(!payload.calibStarted)
    {
      if(payload.motorID == MotorID::MOTOR_HEAD)
      {
        _headCalibrated = true;
      }
      else if(payload.motorID == MotorID::MOTOR_LIFT)
      {
        _liftCalibrated = true;
      }
      else
      {
        PLAYPEN_SET_RESULT(FactoryTestResultCode::MOTOR_CALIB_UNEXPECTED);
      }
    }
  }
  /*
  else if(tag == EngineToGameTag::MotorAutoEnabled)
  {
    
  }
  */
}

}
}

