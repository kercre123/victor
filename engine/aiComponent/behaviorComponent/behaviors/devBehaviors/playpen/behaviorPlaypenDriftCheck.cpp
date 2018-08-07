/**
 * File: behaviorPlaypenDriftCheck.cpp
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenDriftCheck.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

BehaviorPlaypenDriftCheck::BehaviorPlaypenDriftCheck(const Json::Value& config)
: IBehaviorPlaypen(config)
{
}

Result BehaviorPlaypenDriftCheck::OnBehaviorActivatedInternal()
{
  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* moveHeadUp = new MoveHeadToAngleAction(MAX_HEAD_ANGLE);
  MoveHeadToAngleAction* moveHeadToAngle = new MoveHeadToAngleAction(PlaypenConfig::kHeadAngleForDriftCheck);
  MoveLiftToHeightAction* moveLiftUp = new MoveLiftToHeightAction(LIFT_HEIGHT_CARRY);
  
  CompoundActionSequential* headUpDown = new CompoundActionSequential({moveHeadUp, moveHeadToAngle});
  CompoundActionParallel* liftAndHead = new CompoundActionParallel({headUpDown, moveLiftUp});

  // After moving head and lift, wait to ensure that audio recording has stopped before transitioning to playing 
  // the sound and starting more recording
  CompoundActionSequential* action = new CompoundActionSequential({liftAndHead, 
     new WaitAction(Util::MilliSecToSec((float)PlaypenConfig::kDurationOfAudioToRecord_ms))});
  
  DelegateIfInControl(action, [this](){ TransitionToStartDriftCheck(); });
  
  return RESULT_OK;
}

void BehaviorPlaypenDriftCheck::TransitionToStartDriftCheck()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Record intial starting orientation and after kIMUDriftDetectPeriod_ms check for drift
  _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
  _imuTemp.tempStart_c = robot.GetImuTemperature();
  _imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  RecordTouchSensorData(robot, GetDebugLabel());
  
  AddTimer(PlaypenConfig::kIMUDriftDetectPeriod_ms, [this](){ CheckDrift(); });
}

void BehaviorPlaypenDriftCheck::CheckDrift()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
  
  // Write drift rate and imu temperature change during drift detection period to robot
  IMUInfo imuInfo;
  imuInfo.driftRate_degPerSec = angleChange / Util::MilliSecToSec((float)PlaypenConfig::kIMUDriftDetectPeriod_ms);
  _imuTemp.tempEnd_c = robot.GetImuTemperature();
  _imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _imuTemp.duration_ms;
  imuInfo.tempDuration = std::move(_imuTemp);
  
  WriteToStorage(robot, NVStorage::NVEntryTag::NVEntry_IMUInfo, (u8*)&imuInfo, sizeof(imuInfo),
                 FactoryTestResultCode::IMU_INFO_WRITE_FAILED);
  
  // Write drift rate to log
  PLAYPEN_TRY(GetLogger().Append(imuInfo), FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  
  if(angleChange > PlaypenConfig::kIMUDriftAngleThreshDeg)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenDriftCheck.CheckDrift.DriftDetected",
                        "Angle change of %f deg detected in %f seconds",
                        angleChange, Util::MilliSecToSec((float)PlaypenConfig::kIMUDriftDetectPeriod_ms));
    PLAYPEN_SET_RESULT(FactoryTestResultCode::IMU_DRIFTING);
  }
  
  _driftCheckComplete = true;
}

IBehaviorPlaypen::PlaypenStatus BehaviorPlaypenDriftCheck::PlaypenUpdateInternal()
{
  // Wait until both sound and drift check complete
  if(_driftCheckComplete)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, PlaypenStatus::Complete);
  }
  
  return PlaypenStatus::Running;
}

void BehaviorPlaypenDriftCheck::OnBehaviorDeactivated()
{
  _driftCheckComplete = false;
  _startingRobotOrientation = 0;
  _imuTemp = IMUTempDuration();
}

}
}


