/*
 * File: audioClientConnection.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is the base class for Client Connections which handle communication between the server and the audio
 *              clients. It provides core audio functionality by using Audio CLAD messages as the transport interface.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#ifndef __Basestation_Audio_AudioClientConnection_H__
#define __Basestation_Audio_AudioClientConnection_H__

#include "util/helpers/noncopyable.h"
#include <stdio.h>
#include <stdint.h>

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioServer;
struct PostAudioEvent;
struct StopAllAudioEvents;
struct PostAudioGameState;
struct PostAudioSwitchState;
struct PostAudioParameter;
struct PostAudioMusicState;
struct AudioCallback;
  
  
class AudioClientConnection : public Util::noncopyable {
  
public:
  
  AudioClientConnection();
  virtual ~AudioClientConnection();
  
  static const char* kAudioLogChannel;
  
  void SetAudioServer( AudioServer* server ) { _server = server; }
  
  void SetConnectionId( uint8_t connectionId ) { _connectionId = connectionId; }
  uint8_t GetConnectionId() const { return _connectionId; }
  
  virtual void PostCallback( const AudioCallback& callbackMessage ) const {}
  
  
protected:
  
  virtual void HandleMessage( const PostAudioEvent& eventMessage );
  virtual void HandleMessage( const StopAllAudioEvents& stopEventMessage );
  virtual void HandleMessage( const PostAudioGameState& gameStateMessage );
  virtual void HandleMessage( const PostAudioSwitchState& switchStateMessage );
  virtual void HandleMessage( const PostAudioParameter& parameterMessage );
  virtual void HandleMessage( const PostAudioMusicState& musicStateMessage );
  
private:
  
  AudioServer* _server = nullptr;
  uint8_t _connectionId = 0;
  
  
};
  
} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioClientConnection_H__ */
