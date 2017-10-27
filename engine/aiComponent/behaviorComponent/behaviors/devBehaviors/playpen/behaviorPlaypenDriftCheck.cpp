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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenDriftCheck.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "audioUtil/waveFile.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenDriftCheck::BehaviorPlaypenDriftCheck(const Json::Value& config)
: IBehaviorPlaypen(config)
{
}

void BehaviorPlaypenDriftCheck::InitBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  ICozmoBehavior::SubscribeToTags(
    {RobotInterface::RobotToEngineTag::audioFFTResult});
}

Result BehaviorPlaypenDriftCheck::OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();


  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartRecordingAudio(PlaypenConfig::kDurationOfAudioToRecord_ms,
                                                                                      false,
                                                                                      GetLogger().GetLogName()+"head_lift")));

  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* moveHeadUp = new MoveHeadToAngleAction(robot, MAX_HEAD_ANGLE);
  MoveHeadToAngleAction* moveHeadToSoundAngle = new MoveHeadToAngleAction(robot,
                                                                          PlaypenConfig::kHeadAngleToPlaySound);
  MoveLiftToHeightAction* moveLiftUp = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_CARRY);
  
  CompoundActionSequential* headUpDown = new CompoundActionSequential(robot, {moveHeadUp, moveHeadToSoundAngle});
  CompoundActionParallel* liftAndHead = new CompoundActionParallel(robot, {headUpDown, moveLiftUp});

  // After moving head and lift, wait to ensure that audio recording has stopped before transitioning to playing 
  // the sound and starting more recording
  CompoundActionSequential* action = new CompoundActionSequential(robot, {liftAndHead, 
                                                                          new WaitAction(robot, 
                                                                                         PlaypenConfig::kDurationOfAudioToRecord_ms/1000)});
  
  DelegateIfInControl(action, [this, &behaviorExternalInterface](){ TransitionToPlayingSound(behaviorExternalInterface); });
  
  return RESULT_OK;
}

void BehaviorPlaypenDriftCheck::TransitionToPlayingSound(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  // Set speaker volume to config value
  robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetRobotVolume>(1, PlaypenConfig::kSoundVolume);

  // Start recording mic audio of the sound and run an FFT on the audio to check that we actually heard the
  // sound we played
  const bool runFFT = true;
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartRecordingAudio(PlaypenConfig::kDurationOfAudioToRecord_ms,
                                                                                      runFFT,
                                                                                      GetLogger().GetLogName()+"beep")));

  // Record intial starting orientation and after kIMUDriftDetectPeriod_ms check for drift
  _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
  _imuTemp.tempStart_c = robot.GetImuTemperature();
  _imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  AddTimer(PlaypenConfig::kIMUDriftDetectPeriod_ms, [this, &behaviorExternalInterface](){ CheckDrift(behaviorExternalInterface); });
  
  // TODO: Don't play sound while checking drift because of speaker to IMU coupling????
  PlayAnimationAction* soundAction = new PlayAnimationAction(robot, "soundTestAnim");
  DelegateIfInControl(soundAction, [this](){ _soundComplete = true; });
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

BehaviorStatus BehaviorPlaypenDriftCheck::InternalUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Wait until both sound and drift check complete
  if(_soundComplete && _driftCheckComplete)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, BehaviorStatus::Complete);
  }
  
  return BehaviorStatus::Running;
}

void BehaviorPlaypenDriftCheck::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _soundComplete = false;
  _driftCheckComplete = false;
  _startingRobotOrientation = 0;
  _imuTemp = IMUTempDuration();
}

void BehaviorPlaypenDriftCheck::AlwaysHandle(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  // TODO(Al): This message is asynchronous and could be handled after this behavior has completed
  // need some way of having playpen fail if we never get this message. Playpen is long enough that
  // this likely won't happen...
  const auto& tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::audioFFTResult)
  {
    // Vector that maps channel (index) to mic/mic result code
    static const std::vector<FactoryTestResultCode> channelToMic = {
      FactoryTestResultCode::MIC_BL_NOT_WORKING,
      FactoryTestResultCode::MIC_FL_NOT_WORKING,
      FactoryTestResultCode::MIC_BR_NOT_WORKING,
      FactoryTestResultCode::MIC_FR_NOT_WORKING,
    };

    const auto& payload = event.GetData().Get_audioFFTResult();
    u8 count = 0;
    FactoryTestResultCode res = FactoryTestResultCode::UNKNOWN;

    // For each fft result
    for(u8 i = 0; i < payload.result.size(); ++i)
    {
      const auto& fftResult = payload.result[i];
      PRINT_NAMED_INFO("BehaviorPlaypenDriftCheck.HandleAudioFFTResult.Result", 
                       "FFT result for channel %u : %u",
                       i, fftResult);

      // Check that the most prominent frequency heard by this mic is 
      // near the expected frequency
      if(!Util::IsNear((float)fftResult, 
                       PlaypenConfig::kFFTExpectedFreq_hz, 
                       PlaypenConfig::kFFTFreqTolerance_hz))
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

    // If none of the mics heard the expected frequency, either they are all
    // not working or the speaker isn't working
    // Currently assuming it is the latter in this case
    if(count == payload.result.size())
    {
      res = FactoryTestResultCode::SPEAKER_NOT_WORKING;
      PRINT_NAMED_WARNING("BehaviorPlaypenDriftCheck.HandleAudioFFTResult.Speaker", 
                          "No mics picked up expected frequency %u, assuming speaker is not working",
                          PlaypenConfig::kFFTExpectedFreq_hz);
    }

    // Broadcast a failure message containing the result code
    if(res != FactoryTestResultCode::UNKNOWN)
    {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();

      using namespace ExternalInterface;
      const_cast<Robot&>(robot).Broadcast(MessageEngineToGame(PlaypenBehaviorFailed(res)));
    }
  }
}

}
}


