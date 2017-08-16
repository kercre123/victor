/**
 * File: audioLayerManager.h
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

#ifndef __Anki_Cozmo_AudioLayerManager_H__
#define __Anki_Cozmo_AudioLayerManager_H__

#include "anki/common/types.h"
#include "engine/animations/animation.h"
#include "engine/animations/trackLayerManagers/iTrackLayerManager.h"
#include "engine/animations/track.h"

namespace Anki {
namespace Cozmo {

class AudioLayerManager : public ITrackLayerManager<AnimKeyFrame::AudioSample>
{
public:
  
  AudioLayerManager(const Util::RandomGenerator& rng);
  
  // Generates a track of all audio "keyframes" necessary for creating audio glitch sounds
  // Needs to know how many keyframes to generate so that the audio matches with other
  // animation tracks
  void GenerateGlitchAudio(u32 numFramesToGen,
                           Animations::Track<AnimKeyFrame::AudioSample>& outTrack) const;

};

}
}


#endif
