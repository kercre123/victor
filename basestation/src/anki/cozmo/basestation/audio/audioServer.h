//
//  audioServer.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/6/15.
//
//

#ifndef audioServer_h
#define audioServer_h

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

  
class AudioServer {
  
public:
  
  // Transfer AduioController Ownership
  AudioServer( AudioController* audioController );
  ~AudioServer();

  // Transfer AduioClientConnection Ownership
  void RegisterClientConnection( AudioClientConnection* clientConnection );
  
  
  // Client Connection Deletgate Methods
  void ProcessMessage( const PostAudioEvent& eventMessage, uint8_t connectionId );
  void ProcessMessage( const PostAudioGameState& gameStateMessage, uint8_t connectionId );
  void ProcessMessage( const PostAudioSwitchState& switchStateMessage, uint8_t connectionId );
  void ProcessMessage( const PostAudioParameter& parameterMessage, uint8_t connectionId );
  
  
private:
  
  AudioController* _audioController;
  
  using ClientConnectionMap = std::unordered_map< uint8_t, AudioClientConnection* >;
  ClientConnectionMap _clientConnections;
  
  
  uint8_t _previousClientConnectionId;
  
  
  uint8_t GetNewClientConnectionId();
  
};
  
} // Audio
} // Cozmo
} // Anki


#endif /* audioServer_h */
