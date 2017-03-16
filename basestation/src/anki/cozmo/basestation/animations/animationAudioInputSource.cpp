/*
 * File: animationAudioInputSource.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/24/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */

#include "anki/cozmo/basestation/animations/animationAudioInputSource.h"


#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
#include "util/logging/logging.h"
#include "util/math/math.h"


namespace Anki {
namespace Cozmo {
namespace Audio {
}


namespace RobotAnimation {


AnimationAudioInputSource::AnimationAudioInputSource( Audio::AudioMixingConsole& mixingConsole )
: Audio::AudioMixerInputSource(mixingConsole)
{
  
}

AnimationAudioInputSource::BufferState AnimationAudioInputSource::Update()
{
  // FIXME: May always return empty, we expect the state of the audio to be determined by the Animation
  return _state;
}

  
void AnimationAudioInputSource::SetNextFrame(const AudioEngine::AudioFrameData* audioFrame)
{
  // We expect that the animation is buffer is ready so the frame is either empty or ready
  _nextFrameData = audioFrame;
  _state = (_nextFrameData != nullptr) ? BufferState::Ready : BufferState::Empty;
  _validFrame = true;
}

} // namespace RobotAnimation
} // namespace Cozmo
} // namespace Anki
