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
  ASSERT_NAMED(nullptr != messageHandler, "AudioEngineClient.SetMessageHandler.MessageHandlerNull");
  // Subscribe to Audio Messages
  _messageHandler = messageHandler;
  auto callback = [this]( const AnkiEvent<MessageAudioClient>& event ) {
    HandleCallbackEvent( event.GetData().Get_AudioCallback() );
  };
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallback, callback ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  AudioEngineClient::CallbackIdType AudioEngineClient::PostEvent( const GameEvent::GenericEvent event,
                                                                  const GameObjectType gameObject,
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
void AudioEngineClient::StopAllEvents( const GameObjectType gameObject )
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
void AudioEngineClient::PostGameState( const GameState::StateGroupType gameStateGroup,
                                       const GameState::GenericState gameState )
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
void AudioEngineClient::PostSwitchState( const SwitchState::SwitchGroupType switchGroup,
                                         const SwitchState::GenericSwitch switchState,
                                         const GameObjectType gameObject )
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
void AudioEngineClient::PostParameter( const GameParameter::ParameterType parameter,
                                       const float parameterValue,
                                       const GameObjectType gameObject,
                                       const int32_t timeInMilliSeconds,
                                       const CurveType curve ) const
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
void AudioEngineClient::PostMusicState( const GameState::GenericState musicState,
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
  
  
// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::HandleCallbackEvent( const AudioCallback& callbackMsg )
{
//  const CallbackIdType callbackId = static_cast<CallbackIdType>( callbackMsg.callbackId );
  const auto& callbackIt = _callbackMap.find( static_cast<CallbackIdType>( callbackMsg.callbackId ) );
  if ( callbackIt != _callbackMap.end() ) {
    // Perfomr Callback Func
    callbackIt->second( callbackMsg );
    
    // FIXME: Waiting to hear back from WWise if complete callback is allways called, if so remove callback check
    // Delete if it is completed or there there is an error
    AudioCallbackInfoTag callbackTag = callbackMsg.callbackInfo.GetTag();
    if ( AudioCallbackInfoTag::callbackComplete == callbackTag ||
         AudioCallbackInfoTag::callbackError == callbackTag )
    {
      _callbackMap.erase( callbackIt );
    }
  }
  else {
    // Received unexpected callback!
    PRINT_NAMED_ERROR( "AudioEngineClient.HandleCallbackEvent", "Received Unexpected callbackId: %d Type %s",
                       callbackMsg.callbackId, AudioCallbackInfoTagToString( callbackMsg.callbackInfo.GetTag() ) );
  }
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
