/**
 * File: behaviorSelfTestMotorCalibration.cpp
 *
 * Author: Al Chaussee
 * Created: 11/19/2018
 *
 * Description: Calibrates Vector's motors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestMotorCalibration.h"

#include "engine/robot.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestMotorCalibration::BehaviorSelfTestMotorCalibration(const Json::Value& config)
  : IBehaviorSelfTest(config, SelfTestResultCode::MOTOR_CALIBRATION_TIMEOUT)
{
  SubscribeToTags({EngineToGameTag::MotorCalibration});
}

Result BehaviorSelfTestMotorCalibration::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartMotorCalibration(true,
                                                                                        true,
                                                                                        MotorCalibrationReason::SelfTest)));
  AddTimer(5000,
           [this](){ SELFTEST_SET_RESULT(SelfTestResultCode::MOTORS_UNCALIBRATED) });
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestMotorCalibration::SelfTestUpdateInternal()
{
  if(_liftCalibrated && _headCalibrated)
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::SUCCESS, SelfTestStatus::Complete);
  }
  
  return SelfTestStatus::Running;
}

void BehaviorSelfTestMotorCalibration::OnBehaviorDeactivated()
{
  _liftCalibrated = false;
  _headCalibrated = false;
}

void BehaviorSelfTestMotorCalibration::HandleWhileActivatedInternal(const EngineToGameEvent& event)
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
        SELFTEST_SET_RESULT(SelfTestResultCode::MOTOR_CALIB_UNEXPECTED);
      }
    }
  }
}

}
}

