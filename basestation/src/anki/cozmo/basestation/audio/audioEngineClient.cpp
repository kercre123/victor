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
  auto callback = std::bind( &AudioEngineClient::HandleEvents, this, std::placeholders::_1 );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackDuration, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackMarker, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackComplete, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackError, callback ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::CallbackIdType AudioEngineClient::PostEvent( EventType event, GameObjectType gameObject, AudioCallbackFlag callbackFlag )
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
void AudioEngineClient::HandleEvents(const AnkiEvent<MessageAudioClient>& event)
{
  PRINT_NAMED_INFO("HandleEvents.HandleEvents",
                   "Handle game event of type %s !",
                   MessageAudioClientTagToString(event.GetData().GetTag()) );
  
  switch ( event.GetData().GetTag() ) {
      
    case MessageAudioClientTag::AudioCallbackDuration:
    {
      // Handle Duration Callback
      const AudioCallbackDuration& callbackMsg = event.GetData().Get_AudioCallbackDuration();
      HandleCallbackEvent( callbackMsg );
    }
      break;
      
    case MessageAudioClientTag::AudioCallbackMarker:
    {
      // Handle Marker Callback
      const AudioCallbackMarker& callbackMsg = event.GetData().Get_AudioCallbackMarker();
      HandleCallbackEvent( callbackMsg );
    }
      break;
      
    case MessageAudioClientTag::AudioCallbackComplete:
    {
      // Handle Complete Callback
      const AudioCallbackComplete& callbackMsg = event.GetData().Get_AudioCallbackComplete();
      HandleCallbackEvent( callbackMsg );
    }
      break;

    case MessageAudioClientTag::AudioCallbackError:
    {
      // Handle Complete Callback
      const AudioCallbackError& callbackMsg = event.GetData().Get_AudioCallbackError();
      HandleCallbackEvent( callbackMsg );
    }
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "HandleEvents.HandleEvents",
                        "Subscribed to unhandled event of type %s !",
                        MessageAudioClientTagToString(event.GetData().GetTag()) );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::HandleCallbackEvent( const AudioCallbackDuration& callbackMsg )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::HandleCallbackEvent( const AudioCallbackMarker& callbackMsg )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::HandleCallbackEvent( const AudioCallbackComplete& callbackMsg )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::HandleCallbackEvent( const AudioCallbackError& callbackMsg )
{
}
  
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
