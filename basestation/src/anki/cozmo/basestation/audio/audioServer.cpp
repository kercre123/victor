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
#include "anki/cozmo/basestation/audio/musicConductor.h"
#include "clad/audio/audioMessage.h"
#include "clad/audio/audioCallbackMessage.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
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

  // Register CLAD Game Objects
  RegisterCladGameObjectsWithAudioController();
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
AudioServer::ConnectionIdType AudioServer::RegisterClientConnection( AudioClientConnection* clientConnection )
{
  // Setup Client Connection with Server
  ASSERT_NAMED( nullptr != clientConnection, "AudioServer.RegisterClientConnection Client Connection is NULL!");
  ConnectionIdType connectionId = GetNewClientConnectionId();
  clientConnection->SetConnectionId( connectionId );
  clientConnection->SetAudioServer( this );

  _clientConnections.emplace( connectionId, clientConnection );
  
  return connectionId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AudioClientConnection* AudioServer::GetConnection( uint8_t connectionId ) const
{
  const auto it = _clientConnections.find( connectionId );
  if ( it != _clientConnections.end() ) {
    return it->second;
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioEvent& message, ConnectionIdType connectionId )
{
  // Decode Event Message
  const AudioEventId eventId = Util::numeric_cast<AudioEventId>( message.audioEvent );
  const AudioGameObject objectId = static_cast<AudioGameObject>( message.gameObject );
  const uint16_t callbackId = message.callbackId;
  const AudioEngine::AudioCallbackFlag callbackFlags = (callbackId == 0) ?
                                                        AudioEngine::AudioCallbackFlag::NoCallbacks :
                                                        AudioEngine::AudioCallbackFlag::AllCallbacks;

  // Perform Event
  AudioPlayingId playingId = kInvalidAudioPlayingId;
  if ( AudioEngine::AudioCallbackFlag::NoCallbacks != callbackFlags ) {
    AudioCallbackContext* callbackContext = new AudioCallbackContext();
    // Set callback flags
    callbackContext->SetCallbackFlags( callbackFlags );
    // Register callbacks for event
    callbackContext->SetEventCallbackFunc( [this, connectionId, callbackId]
                                           ( const AudioCallbackContext* thisContext,
                                             const AudioEngine::AudioCallbackInfo& callbackInfo )
    {
      PerformCallback( connectionId, callbackId, callbackInfo );
    } );
    
    playingId = _audioController->PostAudioEvent( eventId, objectId, callbackContext );
  }
  else {
    playingId = _audioController->PostAudioEvent( eventId, objectId );
  }
  
  if ( playingId == kInvalidAudioPlayingId ) {
    PRINT_CH_DEBUG(AudioController::kAudioLogChannelName,
                   "AudioServer.ProcessMessage", "Unable to Play Event %s on GameObject %d",
                   EnumToString( message.audioEvent ), message.gameObject );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const StopAllAudioEvents& message, ConnectionIdType connectionId )
{
  const AudioGameObject objectId = static_cast<AudioGameObject>( message.gameObject );
  _audioController->StopAllAudioEvents( objectId );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioGameState& message, ConnectionIdType connectionId )
{
  // Decode Game State Message
  const AudioStateGroupId groupId = Util::numeric_cast<AudioStateGroupId>( message.stateGroup );
  const AudioStateId stateId = Util::numeric_cast<AudioStateId>( message.stateValue );
  // Perform Game State
  const bool success = _audioController->SetState( groupId, stateId );
  if ( !success ) {
    PRINT_CH_DEBUG(AudioController::kAudioLogChannelName,
                   "AudioServer.ProcessMessage", "Unable to Set State %s : %s",
                   EnumToString( message.stateGroup ), EnumToString( message.stateValue ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioSwitchState& message, ConnectionIdType connectionId )
{
  // Decode Switch State Message
  const AudioSwitchGroupId groupId = Util::numeric_cast<AudioStateGroupId>( message.switchStateGroup);
  const AudioSwitchStateId stateId = Util::numeric_cast<AudioSwitchStateId>( message.switchStateValue );
  const AudioGameObject objectId = static_cast<AudioGameObject>( message.gameObject );
  // Perform Switch State
  const bool success = _audioController->SetSwitchState( groupId, stateId, objectId );
  if ( !success ) {
    PRINT_CH_DEBUG(AudioController::kAudioLogChannelName,
                   "AudioServer.ProcessMessage", "Unable to Set Switch State %s : %s on GameObject %s",
                   EnumToString( message.switchStateGroup ), EnumToString( message.switchStateValue ),
                   EnumToString( message.gameObject ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioParameter& message, ConnectionIdType connectionId )
{
  // Decode Parameter Message
  const AudioParameterId parameterId = Util::numeric_cast<AudioParameterId>( message.parameter );
  const AudioRTPCValue value = Util::numeric_cast<AudioRTPCValue>( message.parameterValue );
  const AudioTimeMs duration = Util::numeric_cast<AudioTimeMs>( message.timeInMilliSeconds );
  
  // Translate Invalid Game Object Id
  const AudioGameObject objectId = (message.gameObject == GameObjectType::Invalid) ?
                                    kInvalidAudioGameObject : static_cast<AudioGameObject>( message.gameObject );
  
  // Translate Curve Enum types
  static const std::unordered_map< Anki::Cozmo::Audio::CurveType,
                                   AudioEngine::AudioCurveType,
                                   Util::EnumHasher > curveTypeMap
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
    PRINT_CH_DEBUG(AudioController::kAudioLogChannelName,
                   "AudioServer.ProcessMessage",
                   "Unable to Set Parameter %s to Value %f on GameObject %s with duration %d milliSeconds with \
                   curve type %s",
                   EnumToString( message.parameter ), message.parameterValue, EnumToString( message.gameObject ),
                   message.timeInMilliSeconds, EnumToString( message.curve ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::ProcessMessage( const PostAudioMusicState& message, ConnectionIdType connectionId )
{
  //Decode Music Message
  const AudioStateId stateId = Util::numeric_cast<AudioStateId>( message.stateValue );
  const bool interrupt = message.interrupt;
  const uint32_t minDuration_ms = Util::numeric_cast<uint32_t>( message.minDurationInMilliSeconds );
  _audioController->GetMusicConductor()->SetMusicState( stateId, interrupt, minDuration_ms );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::UpdateAudioController()
{
  _audioController->Update();
}


// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioServer::ConnectionIdType AudioServer::GetNewClientConnectionId()
{
  // This only works for 255 Clients
  ++_previousClientConnectionId;
  ASSERT_NAMED( 0 != _previousClientConnectionId,
                "AudioServer.GetNewClientConnectionId Invalid ConnectionId, this can be caused by adding more then 255 \
                clients");
  
  return _previousClientConnectionId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::PerformCallback( ConnectionIdType connectionId,
                                   uint16_t callbackId,
                                   const AudioEngine::AudioCallbackInfo& callbackInfo )
{
  PRINT_CH_DEBUG(AudioController::kAudioLogChannelName,
                 "AudioServer.PerformCallback",
                 "Event Callback ClientId %d CalbackId: %d - %s",
                 connectionId, callbackId, callbackInfo.GetDescription().c_str());
  
  const AudioClientConnection* connection = GetConnection( connectionId );
  if ( nullptr != connection ) {
    using namespace AudioEngine;
    
    AudioCallback callbackMsg;
    callbackMsg.callbackId = callbackId;
    
    switch ( callbackInfo.callbackType ) {
        
      case AudioEngine::AudioCallbackType::Invalid:
        // Do nothing
        PRINT_NAMED_ERROR( "AudioServer.PerformCallback", "Invalid Callback" );
        break;
        
      case AudioCallbackType::Duration:
      {
        const AudioDurationCallbackInfo& info = static_cast<const AudioDurationCallbackInfo&>( callbackInfo );
        AudioCallbackDuration callbackType( info.duration,
                                            info.estimatedDuration,
                                            info.audioNodeId,
                                            info.isStreaming );
        callbackMsg.callbackInfo.Set_callbackDuration( std::move( callbackType ) );
      }
        break;
        
      case AudioCallbackType::Marker:
      {
        const AudioMarkerCallbackInfo& info = static_cast<const AudioMarkerCallbackInfo&>( callbackInfo );
        AudioCallbackMarker callbackType( info.identifier,
                                          info.position,
                                          info.labelStr );
        callbackMsg.callbackInfo.Set_callbackMarker( std::move( callbackType ) );
      }
        break;
        
      case AudioCallbackType::Complete:
      {
        const AudioCompletionCallbackInfo& info = static_cast<const AudioCompletionCallbackInfo&>( callbackInfo );
        AudioCallbackComplete callbackType( static_cast<GameEvent::GenericEvent>( info.eventId ) );
        callbackMsg.callbackInfo.Set_callbackComplete( std::move( callbackType ) );
      }
        break;
        
      case AudioCallbackType::Error:
      {
        const AudioErrorCallbackInfo& info = static_cast<const AudioErrorCallbackInfo&>( callbackInfo );
        AudioCallbackError callbackType( ConvertErrorCallbackType( info.error ) );
        callbackMsg.callbackInfo.Set_callbackError( std::move( callbackType ) );
      }
        break;
    }
    
    if ( AudioCallbackInfo::Tag::INVALID != callbackMsg.callbackInfo.GetTag() ) {
      connection->PostCallback( std::move( callbackMsg ) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioServer::RegisterCladGameObjectsWithAudioController()
{
  // Enumerate through GameObjectType Enums
  for ( uint32_t aGameObj = static_cast<AudioGameObject>(GameObjectType::Default);
        aGameObj < static_cast<uint32_t>(GameObjectType::End);
        ++aGameObj) {
    // Register GameObjectType
    bool success = _audioController->RegisterGameObject( static_cast<AudioGameObject>( aGameObj ),
                                                         std::string(EnumToString(static_cast<GameObjectType>( aGameObj ))) );
    if (!success) {
      PRINT_NAMED_ERROR( "AudioServer.RegisterCladGameObjectsWithAudioController",
                         "Registering GameObjectId: %ul - %s was unsuccessful",
                         aGameObj, EnumToString(static_cast<GameObjectType>(aGameObj)) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Anki::Cozmo::Audio::CallbackErrorType AudioServer::ConvertErrorCallbackType( const AudioEngine::AudioCallbackErrorType errorType ) const
{
  CallbackErrorType error = CallbackErrorType::Invalid;
  
  switch ( errorType ) {
      
    case AudioEngine::AudioCallbackErrorType::Invalid:
      // Do nothing
      break;
      
    case AudioEngine::AudioCallbackErrorType::EventFailed:
      error = CallbackErrorType::EventFailed;
      break;
      
    case AudioEngine::AudioCallbackErrorType::Starvation:
      error = CallbackErrorType::Starvation;
      break;
  }
  
  return error;
}

  
} // Audio
} // Cozmo
} // Anki
