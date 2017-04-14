/*
 * File: audioMixerOutputSource.h
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Audio_AudioMixerOutputSource_H__
#define __Basestation_Audio_AudioMixerOutputSource_H__

#include "anki/cozmo/basestation/audio/mixing/audioMixingTypes.h"
#include <util/helpers/noncopyable.h>
#include <stdio.h>

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioMixingConsole;


class AudioMixerOutputSource : public Util::noncopyable {
  
public:
  
  AudioMixerOutputSource( const AudioMixingConsole& mixingConsole );
  
  virtual ~AudioMixerOutputSource() = default;
  
  // Final mix, apply output processing and provide data to destination
  virtual void ProcessTick( const MixingConsoleBuffer& audioFrame ) = 0;
  
  
  // Channel Properties
  void SetVolume( float volume ) { _volume = volume; }
  float GetVolume() const { return _volume; }
  
  
  void SetMute( bool mute ) { _mute = mute; }
  bool GetMute() const { return _mute; }
  
protected:
  
  const AudioMixingConsole& _mixingConsole;
  
  float _volume = 1.0;
  
  bool _mute = false;
  
  void ProcessAudioFrame( MixingConsoleBuffer& in_out_sourceFrame );
};

  
} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioMixerOutputSource_H__ */
