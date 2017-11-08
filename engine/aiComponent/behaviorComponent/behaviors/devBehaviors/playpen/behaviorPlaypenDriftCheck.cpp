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
namespace Cozmo {

BehaviorPlaypenDriftCheck::BehaviorPlaypenDriftCheck(const Json::Value& config)
: IBehaviorPlaypen(config)
{
}

Result BehaviorPlaypenDriftCheck::OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();


  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartRecordingMics(PlaypenConfig::kDurationOfAudioToRecord_ms,
                                                                                     false,
                                                                                     GetLogger().GetLogName()+"head_lift")));

  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* moveHeadUp = new MoveHeadToAngleAction(robot, MAX_HEAD_ANGLE);
  MoveHeadToAngleAction* moveHeadToAngle = new MoveHeadToAngleAction(robot,
                                                                          PlaypenConfig::kHeadAngleForDriftCheck);
  MoveLiftToHeightAction* moveLiftUp = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_CARRY);
  
  CompoundActionSequential* headUpDown = new CompoundActionSequential(robot, {moveHeadUp, moveHeadToAngle});
  CompoundActionParallel* liftAndHead = new CompoundActionParallel(robot, {headUpDown, moveLiftUp});

  // After moving head and lift, wait to ensure that audio recording has stopped before transitioning to playing 
  // the sound and starting more recording
  CompoundActionSequential* action = new CompoundActionSequential(robot, {liftAndHead, 
                                                                          new WaitAction(robot, 
                                                                                         PlaypenConfig::kDurationOfAudioToRecord_ms/1000.f)});
  
  DelegateIfInControl(action, [this, &behaviorExternalInterface](){ TransitionToStartDriftCheck(behaviorExternalInterface); });
  
  return RESULT_OK;
}

void BehaviorPlaypenDriftCheck::TransitionToStartDriftCheck(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  // Record intial starting orientation and after kIMUDriftDetectPeriod_ms check for drift
  _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
  _imuTemp.tempStart_c = robot.GetImuTemperature();
  _imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  AddTimer(PlaypenConfig::kIMUDriftDetectPeriod_ms, [this, &behaviorExternalInterface](){ CheckDrift(behaviorExternalInterface); });
}

void BehaviorPlaypenDriftCheck::CheckDrift(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
  
  // Write drift rate and imu temperature change during drift detection period to robot
  IMUInfo imuInfo;
  imuInfo.driftRate_degPerSec = angleChange / (PlaypenConfig::kIMUDriftDetectPeriod_ms / 1000.f);
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
                        angleChange, (PlaypenConfig::kIMUDriftDetectPeriod_ms / 1000.f));
    PLAYPEN_SET_RESULT(FactoryTestResultCode::IMU_DRIFTING);
  }
  
  _driftCheckComplete = true;
}

BehaviorStatus BehaviorPlaypenDriftCheck::PlaypenUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Wait until both sound and drift check complete
  if(_driftCheckComplete)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, BehaviorStatus::Complete);
  }
  
  return BehaviorStatus::Running;
}

void BehaviorPlaypenDriftCheck::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _driftCheckComplete = false;
  _startingRobotOrientation = 0;
  _imuTemp = IMUTempDuration();
}

}
}


