//
//  audioEngineClient.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#include "anki/cozmo/basestation/audio/audioEngineClient.h"
#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::AudioEngineClient( AudioEngineMessageHandler* messageHandler ) :
  _messageHandler( messageHandler )
{
  ASSERT_NAMED( "AudioEngineClient", "Message Handler is NULL!" );
  
  // Subscribe to Audio Messages
  auto callback = std::bind( &AudioEngineClient::HandleEvents, this, std::placeholders::_1 );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackDuration, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackMarker, callback ) );
  _signalHandles.emplace_back( _messageHandler->Subscribe( MessageAudioClientTag::AudioCallbackComplete, callback ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::~AudioEngineClient()
{
  // We don't own message handler
  _messageHandler = nullptr;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostEvent( EventType event, uint16_t gameObjectId, AudioCallbackFlag callbackFlag ) const
{
  const MessageAudioClient msg( PostAudioEvent( event, gameObjectId, callbackFlag ) );
  _messageHandler->Broadcast( msg );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostGameState( GameStateType gameState ) const
{
  const MessageAudioClient msg( (PostAudioGameState( gameState )) );
  _messageHandler->Broadcast( msg );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostSwitchState( SwitchStateType switchState, uint16_t gameObjectId ) const
{
  const MessageAudioClient msg( PostAudioSwitchState( switchState, gameObjectId ));
  _messageHandler->Broadcast( msg );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineClient::PostParameter( ParameterType parameter,
                                       float parameterValue,
                                       uint16_t gameObjectId,
                                       int32_t timeInMilliSeconds,
                                       CurveType curve ) const
{
  const MessageAudioClient msg( PostAudioParameter( parameter, parameterValue, gameObjectId, timeInMilliSeconds, curve) );
  _messageHandler->Broadcast( msg );
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
//      HandleMessage( event.GetData().Get_PostAudioEvent() );
      break;
      
    case MessageAudioClientTag::AudioCallbackMarker:
//      HandleMessage( event.GetData().Get_PostAudioGameState() );
      break;
      
    case MessageAudioClientTag::AudioCallbackComplete:
//      HandleMessage( event.GetData().Get_PostAudioSwitchState() );
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
