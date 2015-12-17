/*
 * File: audioEngineClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a sub-class of AudioClient which provides communication between its self and an
 *              AudioEngineClientConnection by means of AudioEngineMessageHandler. It provide core audio functionality
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::SetMessageHandler( AudioEngineMessageHandler* messageHandler )
{
  ASSERT_NAMED(nullptr != messageHandler, "Can NOT set message handler to NULL");
  // Subscribe to Audio Messages
  _messageHandler = messageHandler;
  auto callback = [this]( const AnkiEvent<MessageAudioClient>& event ) {
    HandleCallbackEvent( event.GetData().Get_AudioCallback() );
  };
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallback, callback ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::CallbackIdType AudioEngineClient::PostEvent( EventType event,
                                                                GameObjectType gameObject,
                                                                AudioCallbackFlag callbackFlag,
                                                                CallbackFunc* callback )
{
  if ( nullptr != _messageHandler ) {
    const CallbackIdType callbackId = AudioCallbackFlag::EventNone != callbackFlag ?
    GetNewCallbackId() : kInvalidCallbackId;
    const MessageAudioClient msg( PostAudioEvent( event, gameObject, callbackFlag, callbackId ) );
    _messageHandler->Broadcast( std::move( msg ) );
    return callbackId;
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostEvent", "Message Handler is Null Can NOT post Event");
  }
  
  return kInvalidCallbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::StopAllEvents( GameObjectType gameObject )
{
  if ( nullptr != _messageHandler ) {
    const MessageAudioClient msg( (StopAllAudioEvents( gameObject )) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostGameState( GameStateGroupType gameStateGroup, GameStateType gameState )
{
  if ( nullptr != _messageHandler ) {
    const MessageAudioClient msg( (PostAudioGameState( gameStateGroup, gameState )) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostGameState", "Message Handler is Null Can NOT post Game State");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostSwitchState( SwitchStateGroupType switchStateGroup, SwitchStateType switchState, GameObjectType gameObject )
{
  if ( nullptr != _messageHandler ) {
    const MessageAudioClient msg( PostAudioSwitchState( switchStateGroup, switchState, gameObject ) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostSwitchState", "Message Handler is Null Can NOT post Switch State");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostParameter( ParameterType parameter,
                                       float parameterValue,
                                       GameObjectType gameObject,
                                       int32_t timeInMilliSeconds,
                                       CurveType curve ) const
{
  if ( nullptr != _messageHandler ) {
    const MessageAudioClient msg( PostAudioParameter( parameter, parameterValue, gameObject, timeInMilliSeconds, curve) );
    _messageHandler->Broadcast( std::move( msg ) );
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineClient.PostParameter", "Message Handler is Null Can NOT post Parameter");
  }
}


// Private  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::CallbackIdType AudioEngineClient::GetNewCallbackId()
{
  // We intentionally let the callback id roll over
  CallbackIdType callbackId = ++_previousCallbackId;
  if ( kInvalidCallbackId == callbackId ) {
    ++callbackId;
  }
  
  return callbackId;
}

} // Audio
} // Cozmo
} // Anki
