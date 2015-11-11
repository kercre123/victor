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
AudioEngineClient::AudioEngineClient( AudioEngineMessageHandler& messageHandler ) :
  _messageHandler( messageHandler )
{
  // Subscribe to Audio Messages
  auto callback = std::bind( &AudioEngineClient::HandleEvents, this, std::placeholders::_1 );
  _signalHandles.emplace_back( _messageHandler.Subscribe( MessageAudioClientTag::AudioCallbackDuration, callback ) );
  _signalHandles.emplace_back( _messageHandler.Subscribe( MessageAudioClientTag::AudioCallbackMarker, callback ) );
  _signalHandles.emplace_back( _messageHandler.Subscribe( MessageAudioClientTag::AudioCallbackComplete, callback ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostEvent( EventType event, uint16_t gameObjectId, AudioCallbackFlag callbackFlag ) const
{
  const MessageAudioClient msg( PostAudioEvent( event, gameObjectId, callbackFlag ) );
  _messageHandler.Broadcast( std::move( msg ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostGameState( GameStateType gameState ) const
{
  const MessageAudioClient msg( (PostAudioGameState( gameState )) );
  _messageHandler.Broadcast( std::move( msg ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostSwitchState( SwitchStateType switchState, uint16_t gameObjectId ) const
{
  const MessageAudioClient msg( PostAudioSwitchState( switchState, gameObjectId ));
  _messageHandler.Broadcast( std::move( msg ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostParameter( ParameterType parameter,
                                       float parameterValue,
                                       uint16_t gameObjectId,
                                       int32_t timeInMilliSeconds,
                                       CurveType curve ) const
{
  const MessageAudioClient msg( PostAudioParameter( parameter, parameterValue, gameObjectId, timeInMilliSeconds, curve) );
  _messageHandler.Broadcast( std::move( msg ) );
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
    // TODO: Handle Callback
      break;
      
    case MessageAudioClientTag::AudioCallbackMarker:
    // TODO: Handle Callback
      break;
      
    case MessageAudioClientTag::AudioCallbackComplete:
    // TODO: Handle Callback
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "HandleEvents.HandleEvents",
                        "Subscribed to unhandled event of type %s !",
                        MessageAudioClientTagToString(event.GetData().GetTag()) );
    }
  }
}

} // Audio
} // Cozmo
} // Anki
