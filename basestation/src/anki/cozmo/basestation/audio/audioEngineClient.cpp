/*
 * File: audioEngineClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a subclass of AudioClient which provides communication between itself and an
 *              AudioEngineClientConnection by means of AudioEngineMessageHandler. It provides core audio functionality
 *              by broadcasting post messages and subscribing to callback messages.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#include "anki/cozmo/basestation/audio/audioEngineClient.h"
#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
using namespace AudioEngine::Multiplexer;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::AudioEngineClient()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::~AudioEngineClient()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::SetMessageHandler( AudioEngineMessageHandler* messageHandler )
{
  DEV_ASSERT(nullptr != messageHandler, "AudioEngineClient.SetMessageHandler.MessageHandlerNull");
  // Subscribe to Audio Messages
  _messageHandler = messageHandler;
  auto callback = [this]( const AnkiEvent<MessageAudioClient>& event ) {
    HandleCallbackEvent( event.GetData().Get_AudioCallback() );
  };
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallback, callback ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  AudioEngineClient::CallbackIdType AudioEngineClient::PostEvent( const AudioMetaData::GameEvent::GenericEvent event,
                                                                  const AudioMetaData::GameObjectType gameObject,
                                                                  const CallbackFunc& callback )
{
  if ( nullptr != _messageHandler ) {
    const CallbackIdType callbackId = nullptr != callback ? GetNewCallbackId() : kInvalidCallbackId;
    // Store callback
    if ( nullptr != callback ) {
      _callbackMap.emplace( callbackId, callback );
    }
    // Post event
    MessageAudioClient msg( PostAudioEvent( event, gameObject, callbackId ) );
    _messageHandler->Broadcast( std::move( msg ) );
    return callbackId;
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostEvent", "Message Handler is Null Can NOT post Event");
  }
  
  return kInvalidCallbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::StopAllEvents( const AudioMetaData::GameObjectType gameObject )
{
  if ( nullptr != _messageHandler ) {
    MessageAudioClient msg( (StopAllAudioEvents( gameObject )) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.StopAllEvents", "Message Handler is Null Can NOT Stop All Events");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void AudioEngineClient::PostGameState( const AudioMetaData::GameState::StateGroupType gameStateGroup,
                                         const AudioMetaData::GameState::GenericState gameState )
{
  if ( nullptr != _messageHandler ) {
    MessageAudioClient msg( (PostAudioGameState( gameStateGroup, gameState )) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostGameState", "Message Handler is Null Can NOT post Game State");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostSwitchState( const AudioMetaData::SwitchState::SwitchGroupType switchGroup,
                                         const AudioMetaData::SwitchState::GenericSwitch switchState,
                                         const AudioMetaData::GameObjectType gameObject )
{
  if ( nullptr != _messageHandler ) {
    MessageAudioClient msg( PostAudioSwitchState( switchGroup, switchState, gameObject ) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostSwitchState", "Message Handler is Null Can NOT post Switch State");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostParameter( const AudioMetaData::GameParameter::ParameterType parameter,
                                       const float parameterValue,
                                       const AudioMetaData::GameObjectType gameObject,
                                       const int32_t timeInMilliSeconds,
                                       const AudioEngine::Multiplexer::CurveType curve ) const
{
  if ( nullptr != _messageHandler ) {
    MessageAudioClient msg( PostAudioParameter( parameter, parameterValue, gameObject, timeInMilliSeconds, curve ) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostParameter", "Message Handler is Null Can NOT post Parameter w/ GameObject");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostMusicState( const AudioMetaData::GameState::GenericState musicState,
                                        const bool interrupt,
                                        const uint32_t minDuration_ms )
{
  if ( nullptr != _messageHandler ) {
    MessageAudioClient msg( PostAudioMusicState( musicState, interrupt, minDuration_ms ) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostMusicState", "Message Handler is Null Can NOT post Music State");
  }
}


} // Audio
} // Cozmo
} // Anki
