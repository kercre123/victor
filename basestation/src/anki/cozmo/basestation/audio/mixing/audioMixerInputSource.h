/*
 * File: audioMixerInputSource.h
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Audio_AudioMixerInputSource_H__
#define __Basestation_Audio_AudioMixerInputSource_H__

#include "anki/cozmo/basestation/audio/mixing/audioMixingTypes.h"
#include "util/helpers/noncopyable.h"

#include <functional>

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioMixingConsole;

class AudioMixerInputSource : public Util::noncopyable {
  
public:
  
  
  enum class BufferState {
    Empty = 0,  // No Audio frames
    Wait,       // Wait for buffer data
    Ready       // Frames are ready
  };
  
  
  AudioMixerInputSource( AudioMixingConsole& mixingConsole );
  
  virtual ~AudioMixerInputSource() = default;
  
  BufferState GetInputSourceState() const { return _state; }
  
  // Determine the current state of the input source
  virtual BufferState Update() = 0;
  
  // Consume frame in Mixer
  const AudioEngine::AudioFrameData* PopFrame();
  
  void ResetInput();
  
  
  // Source Properties
  void SetVolume( float volume ) { _volume = volume; }
  float GetVolume() const { return _volume; }
  
  
  void SetMute( bool mute ) { _mute = mute; }
  bool GetMute() const { return _mute; }
  
  
protected:
  
  AudioMixingConsole& _mixingConsole;
  
  BufferState _state = BufferState::Empty;
  
//  NextAudioFrameCallbackFunc  _nextFrameCallback = nullptr;
  const AudioEngine::AudioFrameData*  _nextFrameData = nullptr;
  bool                                _validFrame = false;
  
  float _volume = 1.0f;
  
  bool _mute = false;
  
  
  
};

  
} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioMixerInputSource_H__ */
