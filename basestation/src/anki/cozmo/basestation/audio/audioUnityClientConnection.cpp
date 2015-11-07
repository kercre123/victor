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
  _externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::PostAudioEvent, callback );
  _externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::PostAudioGameState, callback );
  _externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::PostAudioSwitchState, callback );
  _externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::PostAudioParameter, callback );
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
  switch (event.GetData().GetTag()) {
     
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

  
} // Audio
} // Cozmo
} // Anki
