/**
 * File: behaviorSelfTestDriftCheck.cpp
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestDriftCheck.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestDriftCheck::BehaviorSelfTestDriftCheck(const Json::Value& config)
  : IBehaviorSelfTest(config, SelfTestResultCode::DRIFT_CHECK_TIMEOUT)
{
}

Result BehaviorSelfTestDriftCheck::OnBehaviorActivatedInternal()
{
  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* moveHeadUp = new MoveHeadToAngleAction(MAX_HEAD_ANGLE);
  MoveHeadToAngleAction* moveHeadToAngle = new MoveHeadToAngleAction(SelfTestConfig::kHeadAngleForDriftCheck);
  MoveLiftToHeightAction* moveLiftUp = new MoveLiftToHeightAction(LIFT_HEIGHT_CARRY);
  
  CompoundActionSequential* headUpDown = new CompoundActionSequential({moveHeadUp, moveHeadToAngle});
  CompoundActionParallel* liftAndHead = new CompoundActionParallel({headUpDown, moveLiftUp});

  // After moving head and lift, wait to ensure that audio recording has stopped before transitioning to playing 
  // the sound and starting more recording
  CompoundActionSequential* action = new CompoundActionSequential({liftAndHead, 
     new WaitAction(Util::MilliSecToSec((float)SelfTestConfig::kDurationOfAudioToRecord_ms))});
  
  DelegateIfInControl(action, [this](){ TransitionToStartDriftCheck(); });
  
  return RESULT_OK;
}

void BehaviorSelfTestDriftCheck::TransitionToStartDriftCheck()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Record intial starting orientation and after kIMUDriftDetectPeriod_ms check for drift
  _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
  
  AddTimer(SelfTestConfig::kIMUDriftDetectPeriod_ms, [this](){ CheckDrift(); });
}

void BehaviorSelfTestDriftCheck::CheckDrift()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
  
  if(angleChange > SelfTestConfig::kIMUDriftAngleThreshDeg)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDriftCheck.CheckDrift.DriftDetected",
                        "Angle change of %f deg detected in %f seconds",
                        angleChange, Util::MilliSecToSec((float)SelfTestConfig::kIMUDriftDetectPeriod_ms));
    SELFTEST_SET_RESULT(SelfTestResultCode::IMU_DRIFTING);
  }
  
  _driftCheckComplete = true;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestDriftCheck::SelfTestUpdateInternal()
{
  // Wait until both sound and drift check complete
  if(_driftCheckComplete)
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::SUCCESS, SelfTestStatus::Complete);
  }
  
  return SelfTestStatus::Running;
}

void BehaviorSelfTestDriftCheck::OnBehaviorDeactivated()
{
  _driftCheckComplete = false;
  _startingRobotOrientation = 0;
}

}
}


