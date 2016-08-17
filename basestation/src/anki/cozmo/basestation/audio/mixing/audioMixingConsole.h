/*
 * File: audioMixingConsole.h
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * #include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Audio_AudioMixingConsole_H__
#define __Basestation_Audio_AudioMixingConsole_H__

#include "anki/cozmo/basestation/audio/mixing/audioMixingTypes.h"
#include "util/helpers/noncopyable.h"
#include <stdint.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>


namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioMixerInputSource;
class AudioMixerOutputSource;


class AudioMixingConsole : public Util::noncopyable {
  
public:

  enum class BufferSate {
    Loading = 0,
    Ready
  };
  
  AudioMixingConsole( size_t frameSize );
  
  ~AudioMixingConsole();
  
  // FIXME: Is this frame count? it's not clock time
  using MixerClockTime = uint64_t;
  MixerClockTime GetAudioClock() const { return _audioClock; }
  
//  const uint32_t GetSampleRate() const { return _sampleRate; }
  const size_t GetFrameSize() const { return _frameSize; }
  
  
  BufferSate GetBufferState() const { return _state; }
  
  // TODO: Add engine time ?
  // Don't want to call update like this
  // Maybe I do, this will check the state of all the input sources
  BufferSate Update();
  
  // This will tick the frame
  // Output will be in output source
  void ProcessFrame();
  
  
  
  // TODO: Change this to create and pass back a handle to a channel rather then pass a channel in.
  
  
  // Give Input/Output source object ownership to mixer
  void AddInputSource( AudioMixerInputSource* inputSource );
  
  void AddOutputSource( AudioMixerOutputSource* outputSource );
  

private:
  
//  const uint32_t _sampleRate  = 0;
  const size_t   _frameSize   = 0;
  
  MixerClockTime _audioClock  = 0;
  
  BufferSate _state = BufferSate::Loading;
  
  using InputSourceList = std::vector<AudioMixerInputSource*>;
  InputSourceList _inputSources;
  
  using OutputSourceList = std::vector<AudioMixerOutputSource*>;
  OutputSourceList _outputSources;
  
  // Work memory
  
  // Temp stoage after audio frames are popped out of input sources
//  AudioFrameData** _sourceMixingBuffer = nullptr;
  
  MixingConsoleBuffer _mixingBuffer = nullptr;
  
  BufferSate TickInputSources();
  
  // Mixer methods
  // Return True if there are frames to mix
  bool MixInputSources();
  
  void SendOutputSources(bool hasFrame);
  
  
  void ClearMixingBuffer();
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioMixingConsole_H__ */
