/*
 * File: audioMultiplexer.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: The AudioMultiplexer multiplexes AudioMuxInputs messages to perform AudioEngineController
 *              functionality. The Input’s Audio Post Messages is decoded to perform audio tasks. When the
 *              AudioEngineController performs a callback it is encoded into Audio Callback Messages and is passed to
 *              the appropriate Input to transport to its Client. The AudioMultiplexer owns the Audio Input.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __AnkiAudio_AudioMultiplexer_H__
#define __AnkiAudio_AudioMultiplexer_H__

#include "audioEngine/audioExport.h"
#include "util/helpers/noncopyable.h"
#include <unordered_map>


namespace Anki {
namespace AudioEngine {
class AudioEngineController;
struct AudioCallbackInfo;
enum class AudioCallbackErrorType : uint8_t;

namespace Multiplexer {
class AudioMuxInput;
struct PostAudioEvent;
struct StopAllAudioEvents;
struct PostAudioGameState;
struct PostAudioSwitchState;
struct PostAudioParameter;
struct PostAudioMusicState;
enum class AudioCallbackFlag : uint8_t;
enum class CallbackErrorType : uint8_t;


class AUDIOENGINE_EXPORT AudioMultiplexer : private Anki::Util::noncopyable {
  
public:
  
  // Transfer AudioController Ownership
  AudioMultiplexer( AudioEngineController* audioController );
  
  virtual ~AudioMultiplexer();

  // Transfer AudioMuxInput Ownership
  using InputIdType = uint8_t;
  InputIdType RegisterInput( AudioMuxInput* input );
  
  AudioMuxInput* GetInput( InputIdType inputId ) const;
  
  // Input Deletgate Methods
  // Events
  void ProcessMessage( const PostAudioEvent& message, InputIdType inputId );
  void ProcessMessage( const StopAllAudioEvents& message, InputIdType inputId );
  void ProcessMessage( const PostAudioGameState& message, InputIdType inputId );
  void ProcessMessage( const PostAudioSwitchState& message, InputIdType inputId );
  void ProcessMessage( const PostAudioParameter& message, InputIdType inputId );
  // Music
  void ProcessMessage( const PostAudioMusicState& message, InputIdType inputId );
  
  AudioEngineController* GetAudioController() { return _audioController; }
  
  // Helper method to update Audio Controller
  // Note: This is thread safe
  void UpdateAudioController();
  

private:
  
  AudioEngineController* _audioController = nullptr;
  
  using InputMap = std::unordered_map< InputIdType, AudioMuxInput* >;
  InputMap _inputs;
  
  uint8_t _previousInputId = 0;
  
  // Methods
  
  InputIdType GetNewInputId();
  
  void PerformCallback( InputIdType inputId,
                        uint16_t callbackId,
                        const AudioCallbackInfo& callbackInfo );
  
  CallbackErrorType ConvertErrorCallbackType( const AudioCallbackErrorType errorType ) const;
  
};

} // Multiplexer
} // AudioEngine
} // Anki


#endif /* __AnkiAudio_AudioMultiplexer_H__ */
