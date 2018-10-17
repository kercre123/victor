/**
 * File: behaviorPlaypenSoundCheck.cpp
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks speaker and mics work
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenSoundCheck.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

BehaviorPlaypenSoundCheck::BehaviorPlaypenSoundCheck(const Json::Value& config)
: IBehaviorPlaypen(config)
{
}

void BehaviorPlaypenSoundCheck::InitBehaviorInternal()
{
  ICozmoBehavior::SubscribeToTags({RobotInterface::RobotToEngineTag::audioFFTResult});
}

Result BehaviorPlaypenSoundCheck::OnBehaviorActivatedInternal()
{
  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(PlaypenConfig::kHeadAngleToPlaySound);
  MoveLiftToHeightAction* lift = new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK);
  
  CompoundActionParallel* liftAndHead = new CompoundActionParallel({head, lift});

  DelegateIfInControl(liftAndHead, [this](){ TransitionToPlayingSound(); });
  
  return RESULT_OK;
}

void BehaviorPlaypenSoundCheck::TransitionToPlayingSound()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  RecordTouchSensorData(robot, GetDebugLabel());

  // Set speaker volume to config value
  robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetRobotVolume>(PlaypenConfig::kSoundVolume);

  // Start recording mic audio of the sound and run an FFT on the audio to check that we actually heard the
  // sound we played
  const bool runFFT = true;
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartRecordingMicsRaw(PlaypenConfig::kDurationOfAudioToRecord_ms,
                                                                                     runFFT,
                                                                                     GetLogger().GetLogName()+"beep")));

  PlayAnimationAction* soundAction = new PlayAnimationAction("soundTestAnim");
  DelegateIfInControl(soundAction, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS) });
}

void BehaviorPlaypenSoundCheck::OnBehaviorDeactivated()
{
  _soundComplete = false;
}

void BehaviorPlaypenSoundCheck::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  const auto& tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::audioFFTResult)
  {
    ReceivedFFTResult();

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
      PRINT_CH_INFO("Behaviors", "BehaviorPlaypenDriftCheck.HandleAudioFFTResult.Result", 
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

    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    if(!robot.IsPhysical())
    {
      PRINT_CH_DEBUG("Behaviors", "BehaviorPlaypenDriftCheck.HandleAudioFFTResult.SimulatedRobot",
                        "Ignoring sound check result for simulated robot");
      return;
    }

    // Broadcast a failure message containing the result code
    if(res != FactoryTestResultCode::UNKNOWN)
    {
      using namespace ExternalInterface;
      const_cast<Robot&>(robot).Broadcast(MessageEngineToGame(PlaypenBehaviorFailed(res)));
    }
  }
}

}
}


