/**
 * File: behaviorSelfTestSoundCheck.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Checks speaker and mics work
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestSoundCheck.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestSoundCheck::BehaviorSelfTestSoundCheck(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::SOUND_CHECK_TIMEOUT)
{
}

void BehaviorSelfTestSoundCheck::InitBehaviorInternal()
{
  ICozmoBehavior::SubscribeToTags({RobotInterface::RobotToEngineTag::audioFFTResult});
}

Result BehaviorSelfTestSoundCheck::OnBehaviorActivatedInternal()
{
  // Move head and lift to extremes then move to sound playing angle
  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(SelfTestConfig::kHeadAngleToPlaySound);
  MoveLiftToHeightAction* lift = new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK);
  
  CompoundActionParallel* liftAndHead = new CompoundActionParallel({head, lift});

  DelegateIfInControl(liftAndHead, [this](){ TransitionToPlayingSound(); });
  
  return RESULT_OK;
}

void BehaviorSelfTestSoundCheck::TransitionToPlayingSound()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Set speaker volume to config value
  robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetRobotVolume>(SelfTestConfig::kSoundVolume);

  // Start recording mic audio of the sound and run an FFT on the audio to check that we actually heard the
  // sound we played
  const bool runFFT = true;
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartRecordingMicsRaw(SelfTestConfig::kDurationOfAudioToRecord_ms,
                                                                                        runFFT,
                                                                                        "/data/beep")));
  
  CompoundActionSequential* action = new CompoundActionSequential({
    new PlayAnimationAction("soundTestAnim"), 
    new WaitAction(Util::MilliSecToSec((float)SelfTestConfig::kDurationOfAudioToRecord_ms))
  });
 
  DelegateIfInControl(action, [this, &robot](){
                                DrawTextOnScreen(robot,
                                                 {"Test Running"});
                                SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
                              });
}

void BehaviorSelfTestSoundCheck::OnBehaviorDeactivated()
{
  _soundComplete = false;
}

void BehaviorSelfTestSoundCheck::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  const auto& tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::audioFFTResult)
  {
    ReceivedFFTResult();

    // Vector that maps channel (index) to mic/mic result code
    static const std::vector<SelfTestResultCode> channelToMic = {
      SelfTestResultCode::MIC_BL_NOT_WORKING,
      SelfTestResultCode::MIC_FL_NOT_WORKING,
      SelfTestResultCode::MIC_BR_NOT_WORKING,
      SelfTestResultCode::MIC_FR_NOT_WORKING,
    };

    const auto& payload = event.GetData().Get_audioFFTResult();
    u8 count = 0;
    SelfTestResultCode res = SelfTestResultCode::UNKNOWN;

    // For each fft result
    for(u8 i = 0; i < payload.result.size(); ++i)
    {
      const auto& fftResult = payload.result[i];
      PRINT_NAMED_INFO("BehaviorSelfTestDriftCheck.HandleAudioFFTResult.Result", 
                       "FFT result for channel %u : %u",
                       i, fftResult);

      // Check that the most prominent frequency heard by this mic is 
      // near the expected frequency
      if(!Util::IsNear((float)fftResult, 
                       SelfTestConfig::kFFTExpectedFreq_hz, 
                       SelfTestConfig::kFFTFreqTolerance_hz))
      {
        ++count;
        res = channelToMic[i];
        PRINT_NAMED_WARNING("BehaviorSelfTestDriftCheck.HandleAudioFFTResult.FFTFailed",
                            "%s picked up freq %u which is outside %u +/- %u",
                            EnumToString(res),
                            fftResult,
                            SelfTestConfig::kFFTExpectedFreq_hz, 
                            SelfTestConfig::kFFTFreqTolerance_hz);
      }
    }

    // If none of the mics heard the expected frequency, either they are all
    // not working or the speaker isn't working
    // Currently assuming it is the latter in this case
    if(count == payload.result.size())
    {
      res = SelfTestResultCode::SPEAKER_NOT_WORKING;
      PRINT_NAMED_WARNING("BehaviorSelfTestDriftCheck.HandleAudioFFTResult.Speaker", 
                          "No mics picked up expected frequency %u, assuming speaker is not working",
                          SelfTestConfig::kFFTExpectedFreq_hz);
    }

    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    if(!robot.IsPhysical())
    {
      PRINT_NAMED_DEBUG("BehaviorSelfTestDriftCheck.HandleAudioFFTResult.SimulatedRobot",
                        "Ignoring sound check result for simulated robot");
      return;
    }

    // Broadcast a failure message containing the result code
    if(res != SelfTestResultCode::UNKNOWN)
    {
      using namespace ExternalInterface;
      const_cast<Robot&>(robot).Broadcast(MessageEngineToGame(SelfTestBehaviorFailed(res)));
    }
  }
}

}
}


