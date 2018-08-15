/**
 * File: behaviorPlaypenMotorCalibration.cpp
 *
 * Author: Al Chaussee
 * Created: 07/25/17
 *
 * Description: Calibrates Cozmo's motors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenMotorCalibration.h"

#include "engine/robot.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

BehaviorPlaypenMotorCalibration::BehaviorPlaypenMotorCalibration(const Json::Value& config)
: IBehaviorPlaypen(config)
{
  SubscribeToTags({EngineToGameTag::MotorCalibration});
}

Result BehaviorPlaypenMotorCalibration::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartMotorCalibration(true, true)));
  AddTimer(PlaypenConfig::kMotorCalibrationTimeout_ms,
           [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::MOTORS_UNCALIBRATED) });
  
  return RESULT_OK;
}

IBehaviorPlaypen::PlaypenStatus BehaviorPlaypenMotorCalibration::PlaypenUpdateInternal()
{
  if(_liftCalibrated && _headCalibrated)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, PlaypenStatus::Complete);
  }
  
  return PlaypenStatus::Running;
}

void BehaviorPlaypenMotorCalibration::OnBehaviorDeactivated()
{
  _liftCalibrated = false;
  _headCalibrated = false;
}

void BehaviorPlaypenMotorCalibration::HandleWhileActivatedInternal(const EngineToGameEvent& event)
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
}

}
}

