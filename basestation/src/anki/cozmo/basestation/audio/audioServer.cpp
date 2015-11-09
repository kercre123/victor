//
//  audioServer.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/6/15.
//
//

#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioClientConnection.h"
#include "clad/audio/audioMessage.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioEngine;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioServer::AudioServer( AudioController* audioController ) :
  _audioController( audioController ),
  _previousClientConnectionId( 0 )
{
  ASSERT_NAMED( nullptr != _audioController, "AudioServer Audio Controller is NULL");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioServer::~AudioServer()
{
  Util::SafeDelete( _audioController );
  
  for ( auto& kvp : _clientConnections ) {
    delete kvp.second;
  }
  _clientConnections.clear(); 
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::RegisterClientConnection( AudioClientConnection* clientConnection )
{
  // Setup Client Connection with Server
  uint8_t connectionId = GetNewClientConnectionId();
  clientConnection->SetConnectionId( connectionId );
  clientConnection->SetAudioServer( this );

  _clientConnections.emplace( connectionId, clientConnection );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioEvent& eventMessage, uint8_t connectionId )
{
  // Decode Event Message
  const AudioEventID eventId = static_cast<AudioEventID>( eventMessage.audioEvent );
  const AudioGameObject objectId = static_cast<AudioGameObject>( eventMessage.gameObjectId );
  // Perform Event
  const AudioPlayingID playingId = _audioController->PostAudioEvent( eventId, objectId );
  // Register callbacks for event
  if ( playingId == kInvalidAudioPlayingID ) {
    PRINT_NAMED_ERROR( "AudioServer.ProcessMessage", "Unable To Play Event %s on GameObject %d",
                       EnumToString( eventMessage.audioEvent ), eventMessage.gameObjectId );
  } 

  // TODO: Callbakc stuff
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioGameState& gameStateMessage, uint8_t connectionId )
{
  // Decode Game State Message
  
  // Perform Game State
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioSwitchState& switchStateMessage, uint8_t connectionId )
{
  // Decode Switch State Message
  
  // Perform Switch State
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioParameter& parameterMessage, uint8_t connectionId )
{
  // Decode Parameter Message
  
  // Perform Parameter
}
  
  
  
// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t AudioServer::GetNewClientConnectionId()
{
  // This only works for 255 Clients
  return ++_previousClientConnectionId;
}
  
  
} // Audio
} // Cozmo
} // Anki
