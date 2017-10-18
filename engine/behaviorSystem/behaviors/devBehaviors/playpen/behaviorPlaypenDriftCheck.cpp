/**
 * File: behaviorPlaypenDriftCheck.cpp
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, speaker works, mics work, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenDriftCheck.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenDriftCheck::BehaviorPlaypenDriftCheck(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  IBehavior::SubscribeToTags(robot.GetID(),
                             {RobotInterface::RobotToEngineTag::audioFFTResult});
}

Result BehaviorPlaypenDriftCheck::InternalInitInternal(Robot& robot)
{
  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* moveHeadUp = new MoveHeadToAngleAction(robot, MAX_HEAD_ANGLE);
  MoveHeadToAngleAction* moveHeadToSoundAngle = new MoveHeadToAngleAction(robot,
                                                                          PlaypenConfig::kHeadAngleToPlaySound);
  MoveLiftToHeightAction* moveLiftUp = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_CARRY);
  
  CompoundActionSequential* headUpDown = new CompoundActionSequential(robot, {moveHeadUp, moveHeadToSoundAngle});
  CompoundActionParallel* liftAndHead = new CompoundActionParallel(robot, {headUpDown, moveLiftUp});
  
  StartActing(liftAndHead, [this, &robot](){ TransitionToPlayingSound(robot); });
  
  return RESULT_OK;
}

void BehaviorPlaypenDriftCheck::TransitionToPlayingSound(Robot& robot)
{
  robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetRobotVolume>(1, PlaypenConfig::kSoundVolume);
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartRecordingAudio(PlaypenConfig::kDurationOfAudioToRecord_ms)));

  // Record intial starting orientation and after kIMUDriftDetectPeriod_ms check for drift
  _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
  _imuTemp.tempStart_c = robot.GetImuTemperature();
  _imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  AddTimer(PlaypenConfig::kIMUDriftDetectPeriod_ms, [this, &robot](){ CheckDrift(robot); });
  
  // TODO: Don't play sound while checking drift because of speaker to IMU coupling????
  PlayAnimationAction* soundAction = new PlayAnimationAction(robot, "soundTestAnim");
  StartActing(soundAction, [this](){ _soundComplete = true; });
}

void BehaviorPlaypenDriftCheck::CheckDrift(Robot& robot)
{
  f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
  
  // Write drift rate to robot
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

BehaviorStatus BehaviorPlaypenDriftCheck::InternalUpdateInternal(Robot& robot)
{
  // Wait until both sound and drift check complete
  if(_soundComplete && _driftCheckComplete)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, BehaviorStatus::Complete);
  }
  
  // TODO: Checking microphones here while playing sound
  return BehaviorStatus::Running;
}

void BehaviorPlaypenDriftCheck::StopInternal(Robot& robot)
{
  _soundComplete = false;
  _driftCheckComplete = false;
  _startingRobotOrientation = 0;
  _imuTemp = IMUTempDuration();
}

void BehaviorPlaypenDriftCheck::AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot)
{
  const auto& tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::audioFFTResult)
  {
    // TODO(Al): Figure out channel to mic mapping
    static const std::vector<FactoryTestResultCode> channelToMic = {
      FactoryTestResultCode::MIC_BL_NOT_WORKING,
      FactoryTestResultCode::MIC_FL_NOT_WORKING,
      FactoryTestResultCode::MIC_BR_NOT_WORKING,
      FactoryTestResultCode::MIC_FR_NOT_WORKING,
    };

    const auto& payload = event.GetData().Get_audioFFTResult();
    u8 count = 0;
    FactoryTestResultCode res = FactoryTestResultCode::UNKNOWN;
    for(u8 i = 0; i < payload.result.size(); ++i)
    {
      const auto& fftResult = payload.result[i];
      PRINT_NAMED_INFO("BehaviorPlaypenDriftCheck.HandleAudioFFTResult.Result", 
                       "FFT result for channel %u : %u",
                       i, fftResult);

      if(!Util::IsNear((float)fftResult, (float)PlaypenConfig::kFFTExpectedFreq_hz, (float)PlaypenConfig::kFFTFreqTolerance_hz))
      {
        ++count;
        res = channelToMic[i];
        PRINT_NAMED_WARNING("BehaviorPlaypenDriftCheck.HandleAudioFFTResult.FFTFailed",
                            "%s picked up freq %u which is outside %u +/- %u",
                            EnumToString(res),
                            fftResult,
                            PlaypenConfig::kFFTExpectedFreq_hz, 
                            PlaypenConfig::kFFTFreqTolerance_hz);
      }
    }

    if(count == payload.result.size())
    {
      res = FactoryTestResultCode::SPEAKER_NOT_WORKING;
      PRINT_NAMED_WARNING("BehaviorPlaypenDriftCheck.HandleAudioFFTResult.Speaker", 
                          "No mics picked up expected frequency %u, assuming speaker is not working",
                          PlaypenConfig::kFFTExpectedFreq_hz);
    }

    if(res != FactoryTestResultCode::UNKNOWN)
    {
      const_cast<Robot&>(robot).Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::PlaypenBehaviorFailed(res)));
    }
  }
}

}
}


