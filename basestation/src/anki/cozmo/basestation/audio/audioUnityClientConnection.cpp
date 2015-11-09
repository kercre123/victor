//
//  audioUnityClientCunnection.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/6/15.
//
//

#include "anki/cozmo/basestation/audio/audioUnityClientConnection.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioUnityClientConnection::AudioUnityClientConnection( IExternalInterface* externalInterface ) :
  _externalInterface( externalInterface )
{
  ASSERT_NAMED( nullptr != externalInterface, "AudioUnityClientCunnection External Interface is NULL" );
  
  // Subscribe to Audio Messages
  auto callback = std::bind(&AudioUnityClientConnection::HandleGameEvents, this, std::placeholders::_1);
  _signalHandles.emplace_back( _externalInterface->Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioEvent, callback ) );
  _signalHandles.emplace_back( _externalInterface->Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioGameState, callback ) );
  _signalHandles.emplace_back( _externalInterface->Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioSwitchState, callback ) );
  _signalHandles.emplace_back( _externalInterface->Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioParameter, callback ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioUnityClientConnection::~AudioUnityClientConnection()
{
  // We're not the owner
  _externalInterface = nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioUnityClientConnection::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  // TODO: Remove
  PRINT_NAMED_INFO("AudioUnityClientCunnection.HandleGameEvents",
                   "Handle game event of type %s !",
                   ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) );
  
  switch ( event.GetData().GetTag() ) {
     
    case ExternalInterface::MessageGameToEngineTag::PostAudioEvent:
      HandleMessage( event.GetData().Get_PostAudioEvent() );
      break;
      
    case ExternalInterface::MessageGameToEngineTag::PostAudioGameState:
      HandleMessage( event.GetData().Get_PostAudioGameState() );
      break;
      
    case ExternalInterface::MessageGameToEngineTag::PostAudioSwitchState:
      HandleMessage( event.GetData().Get_PostAudioSwitchState() );
      break;
      
    case ExternalInterface::MessageGameToEngineTag::PostAudioParameter:
      HandleMessage( event.GetData().Get_PostAudioParameter() );
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "AudioUnityClientCunnection.HandleGameEvents",
                         "Subscribed to unhandled event of type %s !",
                         ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) );
    }
  }
}

// FIXME: Check if this is how I should wrap callback messages to send them
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioUnityClientConnection::PostCallback( const AudioCallbackDuration& callbackMessage )
{
  const ExternalInterface::MessageEngineToGame msg((AudioCallbackDuration( callbackMessage )));
  _externalInterface->Broadcast( msg );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioUnityClientConnection::PostCallback( const AudioCallbackMarker& callbackMessage )
{
  const ExternalInterface::MessageEngineToGame msg((AudioCallbackMarker( callbackMessage )));
  _externalInterface->Broadcast( msg );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioUnityClientConnection::PostCallback( const AudioCallbackComplete& callbackMessage )
{
  const ExternalInterface::MessageEngineToGame msg((AudioCallbackComplete( callbackMessage )));
  _externalInterface->Broadcast( msg );
}


  
} // Audio
} // Cozmo
} // Anki
