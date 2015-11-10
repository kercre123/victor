/*
 * File: audioEngineClientCunnection.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a sub-class of AudioClientConnection which provides communication between its self and an
 *              AudioEngineClient by means of AudioEngineMessageHandler.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/audioEngineClientConnection.h"
#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>

namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClientConnection::AudioEngineClientConnection( AudioEngineMessageHandler* messageHandler ) :
  _messageHandler( messageHandler )
{
  ASSERT_NAMED( nullptr != messageHandler, "Message Handler is NULL!" );
  
  // Subscribe to Connection Side Messages
  auto callback = std::bind(&AudioEngineClientConnection::HandleEvents, this, std::placeholders::_1);
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioEvent, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioGameState, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioSwitchState, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioParameter, callback ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClientConnection::~AudioEngineClientConnection()
{
  Util::SafeDelete( _messageHandler );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClientConnection::PostCallback( const AudioCallbackDuration& callbackMessage )
{
  const MessageAudioClient msg(( AudioCallbackDuration( callbackMessage ) ));
  _messageHandler->Broadcast( std::move( msg ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClientConnection::PostCallback( const AudioCallbackMarker& callbackMessage )
{
  const MessageAudioClient msg(( AudioCallbackMarker( callbackMessage ) ));
  _messageHandler->Broadcast( std::move( msg ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClientConnection::PostCallback( const AudioCallbackComplete& callbackMessage )
{
  const MessageAudioClient msg(( AudioCallbackComplete( callbackMessage ) ));
  _messageHandler->Broadcast( std::move( msg ) );
}

// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClientConnection::HandleEvents(const AnkiEvent<MessageAudioClient>& event)
{
  PRINT_NAMED_INFO("AudioEngineClientConnection.HandleGameEvents",
                   "Handle game event of type %s !",
                   MessageAudioClientTagToString(event.GetData().GetTag()) );
  
  switch ( event.GetData().GetTag() ) {
      
    case MessageAudioClientTag::PostAudioEvent:
      HandleMessage( event.GetData().Get_PostAudioEvent() );
      break;
      
    case MessageAudioClientTag::PostAudioGameState:
      HandleMessage( event.GetData().Get_PostAudioGameState() );
      break;
      
    case MessageAudioClientTag::PostAudioSwitchState:
      HandleMessage( event.GetData().Get_PostAudioSwitchState() );
      break;
      
    case MessageAudioClientTag::PostAudioParameter:
      HandleMessage( event.GetData().Get_PostAudioParameter() );
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "AudioEngineClientConnection.HandleGameEvents",
                         "Subscribed to unhandled event of type %s !",
                         MessageAudioClientTagToString(event.GetData().GetTag()) );
    }
  }
}

} // Audio
} // Cozmo
} // Anki
