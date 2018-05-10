/**
 * File: audioLayerManager.cpp
 *
 * Authors: Al Chaussee
 * Created: 06/28/2017
 *
 * Description: Specific track layer manager for RobotAudioKeyFrame
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cannedAnimLib/baseTypes/audioKeyFrameTypes.h"
#include "cozmoAnim/animation/trackLayerManagers/audioLayerManager.h"
#include "util/console/consoleInterface.h"
#include <algorithm>


namespace Anki {
namespace Cozmo {

namespace
{
  CONSOLE_VAR(u32, kEyeDartShortLongThreshold_ms, "AudioLayers", 100);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioLayerManager::AudioLayerManager(const Util::RandomGenerator& rng)
: ITrackLayerManager<RobotAudioKeyFrame>(rng)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AudioLayerManager::AddEyeBlinkToAudioTrack(const std::string& layerName, const BlinkEventList& eventList)
{
  using namespace AudioKeyFrameType;
  using namespace AudioMetaData;
  Animations::Track<RobotAudioKeyFrame> audioTrack;
  const auto eventIt = std::find_if(eventList.begin(), eventList.end(), [](const BlinkEvent& event)
                                                                        { return (BlinkState::Closed == event.State); });
  if (eventIt != eventList.end()) {
    RobotAudioKeyFrame frame;
    // Add Switch for procedural event type
    auto switchRef = AudioSwitchRef(SwitchState::SwitchGroupType::Robot_Vic_Scrn_Procedural,
                                    static_cast<SwitchState::GenericSwitch>(SwitchState::Robot_Vic_Scrn_Procedural::Blink));
    frame.AddAudioRef(AudioRef(std::move(switchRef)));
    // Add Event Group
    AudioEventGroupRef eventGroup;
    eventGroup.AddEvent(GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Scrn_Procedural, 1.0f, 1.0f);
    AudioRef ref = AudioRef(std::move(eventGroup));
    frame.AddAudioRef(std::move(ref));
    frame.SetTriggerTime(eventIt->Time_ms);
    audioTrack.AddKeyFrameToBack(frame);
  }

  if (audioTrack.IsEmpty()) {
    // Don't add an empty track
    return RESULT_OK;
  }
  
  return AddLayer(layerName, audioTrack);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AudioLayerManager::AddEyeDartToAudioTrack(const std::string& layerName, TimeStamp_t interpolationTime_ms)
{
  using namespace AudioKeyFrameType;
  using namespace AudioMetaData;
  SwitchState::Robot_Vic_Scrn_Procedural switchState = (interpolationTime_ms < kEyeDartShortLongThreshold_ms) ?
                                                       SwitchState::Robot_Vic_Scrn_Procedural::Shift_Short :
                                                       SwitchState::Robot_Vic_Scrn_Procedural::Shift_Long;
  
  Animations::Track<RobotAudioKeyFrame> audioTrack;
  RobotAudioKeyFrame frame;
  // Add Switch for procedural event type
  auto switchRef = AudioSwitchRef(SwitchState::SwitchGroupType::Robot_Vic_Scrn_Procedural,
                                  static_cast<SwitchState::GenericSwitch>(switchState));
  frame.AddAudioRef(AudioRef(std::move(switchRef)));
  // Add Event Group
  AudioEventGroupRef eventGroup;
  eventGroup.AddEvent(GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Scrn_Procedural, 1.0f, 1.0f);
  AudioRef ref = AudioRef(std::move(eventGroup));
  frame.AddAudioRef(std::move(ref));
  frame.SetTriggerTime(0);  // Always start with begining of movement
  audioTrack.AddKeyFrameToBack(frame);

  return AddLayer(layerName, audioTrack);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AudioLayerManager::AddEyeSquintToAudioTrack(const std::string& layerName)
{
  using namespace AudioKeyFrameType;
  using namespace AudioMetaData;
  Animations::Track<RobotAudioKeyFrame> audioTrack;
  RobotAudioKeyFrame frame;
  // Add Switch for procedural event type
  auto switchRef = AudioSwitchRef(SwitchState::SwitchGroupType::Robot_Vic_Scrn_Procedural,
                                  static_cast<SwitchState::GenericSwitch>(SwitchState::Robot_Vic_Scrn_Procedural::Squint));
  frame.AddAudioRef(AudioRef(std::move(switchRef)));
  // Add Event Group
  AudioEventGroupRef eventGroup;
  eventGroup.AddEvent(GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Scrn_Procedural, 1.0f, 1.0f);
  AudioRef ref = AudioRef(std::move(eventGroup));
  frame.AddAudioRef(std::move(ref));
  frame.SetTriggerTime(0);  // Always start with begining of movement
  audioTrack.AddKeyFrameToBack(frame);
  
  return AddLayer(layerName, audioTrack);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioLayerManager::GenerateGlitchAudio(u32 numFramesToGen,
                                            Animations::Track<RobotAudioKeyFrame>& outTrack) const
{
  // TODO: VIC-447: Restore glitching
  /*
  float prevGlitchAudioSampleVal = 0.f;
  
  for(int i = 0; i < numFramesToGen; i++)
  {
    AnimKeyFrame::AudioSample sample;
    for(int i = 0; i < sample.sample.size(); ++i)
    {
      // Adapted Brownian noise generator from
      // https://noisehack.com/generate-noise-web-audio-api/
      const float rand = GetRNG().RandDbl() * 2 - 1;
      prevGlitchAudioSampleVal = (prevGlitchAudioSampleVal + (0.02 * rand)) / 1.02;
      sample.sample[i] = ((prevGlitchAudioSampleVal * 3.5) + 1) * 128;
    }

    outTrack.AddKeyFrameToBack(sample);
  }
   */
}
  
}
}
