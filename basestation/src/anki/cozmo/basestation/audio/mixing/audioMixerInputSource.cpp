/*
 * File: audioMixerInputSource.cpp
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/audio/mixing/audioMixerInputSource.h"
#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
#include "util/logging/logging.h"
#include "util/math/math.h"


namespace Anki {
namespace Cozmo {
namespace Audio {

AudioMixerInputSource::AudioMixerInputSource( AudioMixingConsole& mixingConsole )
: _mixingConsole( mixingConsole )
{
}

const AudioEngine::AudioFrameData* AudioMixerInputSource::PopFrame()
{
  DEV_ASSERT(_state != BufferState::Wait, "AudioMixerInputSource.PopFrame.State.NotWait");
  DEV_ASSERT(_validFrame, "AudioMixerInputSource.PopFrame.ValidFrame.False");
  
  const AudioEngine::AudioFrameData* frame = _nextFrameData;
  ResetInput();
  return frame;
}
  
void AudioMixerInputSource::ResetInput()
{
  _state = BufferState::Empty;
  _validFrame = false;
  _nextFrameData = nullptr;
}


} // Audio
} // Cozmo
} // Anki
