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

#include <util/helpers/noncopyable.h>
#include <stdio.h>
#include <unordered_map>

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioController;
class AudioClientConnection;
struct PostAudioEvent;
struct PostAudioGameState;
struct PostAudioSwitchState;
struct PostAudioParameter;
struct PostCallbackEventBegin;
struct PostCallbackEventMarker;
struct PostCallbackEventComplete;

  
class AudioServer : public Util::noncopyable {
  
public:
  
  // Transfer AduioController Ownership
  AudioServer( AudioController* audioController );
  ~AudioServer();

  // Transfer AduioClientConnection Ownership
  void RegisterClientConnection( AudioClientConnection* clientConnection );
  
  // Client Connection Deletgate Methods
  void ProcessMessage( const PostAudioEvent& message, uint8_t connectionId );
  void ProcessMessage( const PostAudioGameState& message, uint8_t connectionId );
  void ProcessMessage( const PostAudioSwitchState& message, uint8_t connectionId );
  void ProcessMessage( const PostAudioParameter& message, uint8_t connectionId );
  
  
private:
  
  AudioController* _audioController = nullptr;
  
  using ClientConnectionMap = std::unordered_map< uint8_t, AudioClientConnection* >;
  ClientConnectionMap _clientConnections;
  
  
  uint8_t _previousClientConnectionId = 0;
  
  
  uint8_t GetNewClientConnectionId();
  
};
  
} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioServer_H__ */
