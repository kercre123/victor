/*
 * File: animationAudioInputSource.h
 *
 * Author: Jordan Rivas
 * Created: 07/24/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_AnimationAudioInputSource_H__
#define __Basestation_Animations_AnimationAudioInputSource_H__

#include "anki/cozmo/basestation/audio/mixing/audioMixerInputSource.h"

#include <stdio.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
class AudioMixingConsole;
}


namespace RobotAnimation {

class AnimationAudioInputSource : public Audio::AudioMixerInputSource {
  
public:
  
  AnimationAudioInputSource( Audio::AudioMixingConsole& mixingConsole );
  
  // Nothing happens in update for this sub-class
  virtual BufferState Update() override;
  
  
  // Method sub-class usess to set audio frame data
  // This Needs to be called before Update() & PopFrame()
  void SetNextFrame(const Audio::AudioFrameData* audioFrame);
  
  
};


} // namespace RobotAnimation
} // namespace Cozmo
} // namespace Anki

#endif /* __Basestation_Animations_AnimationAudioInputSource_H__ */
