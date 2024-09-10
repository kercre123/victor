/*
 * File: audioMuxInput.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is the base class for Audio Mux Input which handle communication between the Multiplexer and the
 *              Mux inputs. It provides core audio functionality by using Audio CLAD messages as the transport
 *              interface.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#ifndef __AnkiAudio_AudioMuxInput_H__
#define __AnkiAudio_AudioMuxInput_H__

#include "audioEngine/audioExport.h"
#include "util/helpers/noncopyable.h"
#include <stdio.h>
#include <stdint.h>

namespace Anki {
namespace AudioEngine {
namespace Multiplexer {
class AudioMultiplexer;
struct PostAudioEvent;
struct StopAllAudioEvents;
struct PostAudioGameState;
struct PostAudioSwitchState;
struct PostAudioParameter;
struct PostAudioMusicState;
struct AudioCallbackDuration;
struct AudioCallbackMarker;
struct AudioCallbackComplete;
struct AudioCallbackError;


class AUDIOENGINE_EXPORT AudioMuxInput : private Anki::Util::noncopyable {
  
public:
  
  AudioMuxInput();
  virtual ~AudioMuxInput();
  
  static const char* kAudioLogChannel;
  
  void SetAudioMultiplexer( AudioMultiplexer* mux ) { _mux = mux; }
  
  void SetInputId( uint8_t inputId ) { _inputId = inputId; }
  uint8_t GetInputId() const { return _inputId; }
  
  virtual void PostCallback( AudioCallbackDuration&& callbackMessage ) const {}
  virtual void PostCallback( AudioCallbackMarker&& callbackMessage ) const {}
  virtual void PostCallback( AudioCallbackComplete&& callbackMessage ) const {}
  virtual void PostCallback( AudioCallbackError&& callbackMessage ) const {}
  
  
protected:
  
  virtual void HandleMessage( const PostAudioEvent& eventMessage );
  virtual void HandleMessage( const StopAllAudioEvents& stopEventMessage );
  virtual void HandleMessage( const PostAudioGameState& gameStateMessage );
  virtual void HandleMessage( const PostAudioSwitchState& switchStateMessage );
  virtual void HandleMessage( const PostAudioParameter& parameterMessage );
  virtual void HandleMessage( const PostAudioMusicState& musicStateMessage );
  
private:
  
  AudioMultiplexer* _mux = nullptr;
  uint8_t _inputId = 0;
  
  
};
} // Multiplexer
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_AudioMuxInput_H__ */
