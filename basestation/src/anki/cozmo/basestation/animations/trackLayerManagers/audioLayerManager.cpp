/**
 * File: audioLayerManager.cpp
 *
 * Authors: Al Chaussee
 * Created: 06/28/2017
 *
 * Description: Specific track layer manager for AnimKeyFrame::AudioSample
 *              Handles generating audio for procedural
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/animations/trackLayerManagers/audioLayerManager.h"

namespace Anki {
namespace Cozmo {

AudioLayerManager::AudioLayerManager(const Util::RandomGenerator& rng)
: ITrackLayerManager<AnimKeyFrame::AudioSample>(rng)
{
  
}

void AudioLayerManager::GenerateGlitchAudio(u32 numFramesToGen,
                                            Animations::Track<AnimKeyFrame::AudioSample>& outTrack) const
{
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
}

}
}
