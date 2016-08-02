/*
 * File: audioServer.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: The server multiplexes AudioClientConnections messages to perform AudioController functionality. The
 *              connection’s Audio Post Messages is decoded to perform audio tasks. When the AudioController performs a
 *              callback it is encoded into Audio Callback Messages and is passed to the appropriate connection to
 *              transport to its client. The AudioServer owns the Audio Connection.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_AudioServer_H__
#define __Basestation_Audio_AudioServer_H__

#include "DriveAudioEngine/audioTypes.h"
#include "DriveAudioEngine/audioCallback.h"
#include <util/helpers/noncopyable.h>
#include <stdio.h>
#include <unordered_map>


namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioController;
class AudioClientConnection;
struct PostAudioEvent;
struct StopAllAudioEvents;
struct PostAudioGameState;
struct PostAudioSwitchState;
struct PostAudioParameter;
struct PostAudioMusicState;
struct AudioCallbackDuration;
struct AudioCallbackMarker;
struct AudioCallbackMarkerComplete;
enum class AudioCallbackFlag : uint8_t;
enum class CallbackErrorType : uint8_t;
  
class AudioServer : public Util::noncopyable {
  
public:
  
  // Transfer AduioController Ownership
  AudioServer( AudioController* audioController );
  ~AudioServer();

  // Transfer AduioClientConnection Ownership
  using ConnectionIdType = uint8_t;
  ConnectionIdType RegisterClientConnection( AudioClientConnection* clientConnection );
  
  const AudioClientConnection* GetConnection( ConnectionIdType connectionId ) const;
  
  // Client Connection Deletgate Methods
  // Events
  void ProcessMessage( const PostAudioEvent& message, ConnectionIdType connectionId );
  void ProcessMessage( const StopAllAudioEvents& message, ConnectionIdType connectionId );
  void ProcessMessage( const PostAudioGameState& message, ConnectionIdType connectionId );
  void ProcessMessage( const PostAudioSwitchState& message, ConnectionIdType connectionId );
  void ProcessMessage( const PostAudioParameter& message, ConnectionIdType connectionId );
  // Music
  void ProcessMessage( const PostAudioMusicState& message, ConnectionIdType connectionId );
  
  AudioController* GetAudioController() { return _audioController; }
  
  // Helper method to update Audio Controller
  // Note: This is thread safe
  void UpdateAudioController();
  

private:
  
  AudioController* _audioController = nullptr;
  
  using ClientConnectionMap = std::unordered_map< ConnectionIdType, AudioClientConnection* >;
  ClientConnectionMap _clientConnections;
  
  uint8_t _previousClientConnectionId = 0;
  
  // Methods
  
  ConnectionIdType GetNewClientConnectionId();
  
  void PerformCallback( ConnectionIdType connectionId,
                        uint16_t callbackId,
                        const AudioEngine::AudioCallbackInfo& callbackInfo );
  
  void RegisterCladGameObjectsWithAudioController();
  
  Anki::Cozmo::Audio::CallbackErrorType ConvertErrorCallbackType( const AudioEngine::AudioCallbackErrorType errorType ) const;
  
};

  
} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioServer_H__ */
