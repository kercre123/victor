/*
 * File: audioClientConnection.cpp
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

#include "anki/cozmo/basestation/audio/audioClientConnection.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioClientConnection::AudioClientConnection()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioClientConnection::~AudioClientConnection()
{
  // We don't own the server
  _server = nullptr;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioClientConnection::HandleMessage( const PostAudioEvent& eventMessage )
{
  if ( nullptr != _server ) {
    _server->ProcessMessage( eventMessage, GetConnectionId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioClientConnection.HandleMessage" , "Server is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioClientConnection::HandleMessage( const PostAudioGameState& gameStateMessage )
{
  if ( nullptr != _server ) {
    _server->ProcessMessage( gameStateMessage, GetConnectionId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioClientConnection.HandleMessage" , "Server is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioClientConnection::HandleMessage( const PostAudioSwitchState& switchStateMessage )
{
  if ( nullptr != _server ) {
    _server->ProcessMessage( switchStateMessage, GetConnectionId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioClientConnection.HandleMessage" , "Server is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioClientConnection::HandleMessage( const PostAudioParameter& parameterMessage )
{
  if ( nullptr != _server ) {
    _server->ProcessMessage( parameterMessage, GetConnectionId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioClientConnection.HandleMessage" , "Server is NULL" );
  }
}
  
  
} // Audio
} // Cozmo
} // Anki
