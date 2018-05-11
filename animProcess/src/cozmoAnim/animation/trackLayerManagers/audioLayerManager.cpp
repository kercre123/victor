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

#include "cozmoAnim/animation/trackLayerManagers/audioLayerManager.h"

namespace Anki {
namespace Cozmo {

AudioLayerManager::AudioLayerManager(const Util::RandomGenerator& rng)
: ITrackLayerManager<RobotAudioKeyFrame>(rng)
{
  
}

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
