//
//  audioClientConnection.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/6/15.
//
//

#ifndef audioClientConnection_h
#define audioClientConnection_h

#include <stdio.h>
#include <stdint.h>

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioServer;
struct PostAudioEvent;
struct PostAudioGameState;
struct PostAudioSwitchState;
struct PostAudioParameter;
struct AudioCallbackDuration;
struct AudioCallbackMarker;
struct AudioCallbackComplete;
  
class AudioClientConnection {
  
public:
  
  AudioClientConnection();
  virtual ~AudioClientConnection();
  
  void SetAudioServer( AudioServer* server ) { _server = server; }
  
  void SetConnectionId( uint8_t connectionId ) { _connectionId = connectionId; }
  uint8_t GetConnectionId() const { return _connectionId; }
  
  
protected:
  
  void HandleMessage( const PostAudioEvent& eventMessage );
  void HandleMessage( const PostAudioGameState& gameStateMessage );
  void HandleMessage( const PostAudioSwitchState& switchStateMessage );
  void HandleMessage( const PostAudioParameter& parameterMessage );
  
  virtual void PostCallback( const AudioCallbackDuration& callbackMessage ) {}
  virtual void PostCallback( const AudioCallbackMarker& callbackMessage ) {}
  virtual void PostCallback( const AudioCallbackComplete& callbackMessage ) {}
  
private:
  
  AudioServer* _server;
  uint8_t _connectionId;
  
  
};
  
} // Audio
} // Cozmo
} // Anki

#endif /* audioClientConnection_h */
