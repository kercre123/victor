/*
 * File: audioServer.cpp
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

#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioClientConnection.h"
#include "clad/audio/audioMessage.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>
#include <unordered_map>

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioEngine;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioServer::AudioServer( AudioController* audioController ) :
  _audioController( audioController )
{
  ASSERT_NAMED( nullptr != _audioController, "AudioServer Audio Controller is NULL");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioServer::~AudioServer()
{
  Util::SafeDelete( _audioController );
  
  for ( auto& kvp : _clientConnections ) {
    Util::SafeDelete( kvp.second );
  }
  _clientConnections.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::RegisterClientConnection( AudioClientConnection* clientConnection )
{
  // Setup Client Connection with Server
  ASSERT_NAMED( nullptr != clientConnection, "AudioServer.RegisterClientConnection Client Connection is NULL!");
  uint8_t connectionId = GetNewClientConnectionId();
  clientConnection->SetConnectionId( connectionId );
  clientConnection->SetAudioServer( this );

  _clientConnections.emplace( connectionId, clientConnection );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioEvent& message, uint8_t connectionId )
{
  // Decode Event Message
  const AudioEventID eventId = static_cast<AudioEventID>( message.audioEvent );
  const AudioGameObject objectId = static_cast<AudioGameObject>( message.gameObjectId );
  // Perform Event
  const AudioPlayingID playingId = _audioController->PostAudioEvent( eventId, objectId );
  // Register callbacks for event
  if ( playingId == kInvalidAudioPlayingID ) {
    PRINT_NAMED_ERROR( "AudioServer.ProcessMessage", "Unable To Play Event %s on GameObject %d",
                       EnumToString( message.audioEvent ), message.gameObjectId );
  } 

  // TODO: Callbakc stuff
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioGameState& message, uint8_t connectionId )
{
  // Decode Game State Message
  const AudioStateGroupId groupId = 0;  // TODO: This is wrong we need to get the type form the state or add it to message
  const AudioStateId stateId = static_cast<AudioStateId>( message.state );
  // Perform Game State
  const bool success = _audioController->SetState( groupId, stateId );
  if ( !success ) {
    PRINT_NAMED_ERROR( "AudioServer.ProcessMessage", "Unable To Set State %s",
                      EnumToString( message.state ) );
  }
  // TODO: Callbakc stuff
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioSwitchState& message, uint8_t connectionId )
{
  // Decode Switch State Message
  const AudioSwitchGroupId groupId = 0;  // TODO: This is wrong we need to get the type form the state or add it to message
  const AudioSwitchStateId stateId = static_cast<AudioSwitchStateId>( message.switchState );
  const AudioGameObject objectId = static_cast<AudioGameObject>( message.gameObjectId );
  // Perform Switch State
  const bool success = _audioController->SetSwitchState( groupId, stateId, objectId );
  if ( !success ) {
    PRINT_NAMED_ERROR( "AudioServer.ProcessMessage", "Unable To Set Switch State %s on GameObject %d",
                      EnumToString( message.switchState ), message.gameObjectId );
  }
  // TODO: Callbakc stuff
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioParameter& message, uint8_t connectionId )
{
  // Decode Parameter Message
  const AudioParameterId parameterId = static_cast<AudioParameterId>( message.parameter );
  const AudioRTPCValue value = static_cast<AudioRTPCValue>( message.parameterValue );
  const AudioGameObject objectId = static_cast<AudioGameObject>( message.gameObjectId );
  const AudioTimeMs duration = static_cast<AudioTimeMs>( message.timeInMilliSeconds );
  
  // Translate Curve Enum types
  static const std::unordered_map< Anki::Cozmo::Audio::CurveType, AudioEngine::AudioCurveType, Util::EnumHasher > curveTypeMap
  {
    { CurveType::Linear,            AudioCurveType::Linear },
    { CurveType::SCurve,            AudioCurveType::SCurve },
    { CurveType::InversedSCurve,    AudioCurveType::InversedSCurve },
    { CurveType::Sine,              AudioCurveType::Sine },
    { CurveType::SineReciprocal,    AudioCurveType::SineReciprocal },
    { CurveType::Exp1,              AudioCurveType::Exp1 },
    { CurveType::Exp3,              AudioCurveType::Exp3 },
    { CurveType::Log1,              AudioCurveType::Log1 },
    { CurveType::Log3,              AudioCurveType::Log3 },
  };
  
  AudioCurveType curve = AudioCurveType::Linear;
  const auto& it = curveTypeMap.find( message.curve );
  if ( it != curveTypeMap.end() ) {
    curve = it->second;
  }
  else {
    PRINT_NAMED_ERROR("AudioServer.ProcessMessage", "Can NOT find Parameter Curve Type %s", EnumToString( message.curve ));
  }
  
  // Perform Parameter
  const bool success = _audioController->SetParameter( parameterId, value, objectId, duration, curve );
  if ( !success ) {
    PRINT_NAMED_ERROR( "AudioServer.ProcessMessage",
                       "Unable To Set Parameter %s to Value %f on GameObject %d with duration %d milliSeconds with curve type %s",
                       EnumToString( message.parameter ), message.parameterValue, message.gameObjectId,
                       message.timeInMilliSeconds, EnumToString( message.curve ) );
  }
  // TODO: Callbakc stuff
}
  
  
// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t AudioServer::GetNewClientConnectionId()
{
  // This only works for 255 Clients
  ++_previousClientConnectionId;
  ASSERT_NAMED( 0 == _previousClientConnectionId, "AudioServer.GetNewClientConnectionId Invalid ConnectionId, this can be caused by adding more then 255 clients");
  
  return _previousClientConnectionId;
  
}
  
  
} // Audio
} // Cozmo
} // Anki
