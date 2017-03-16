/*
 * File: audioUnityInput.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Connection is specific to communicating with the Unity Message Interface. It is a subclass of
 *              AudioMuxInput.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/audioUnityInput.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioUnityInput::AudioUnityInput( IExternalInterface& externalInterface ) :
  _externalInterface( externalInterface )
{
  // Subscribe to Audio Messages
  auto callback = std::bind(&AudioUnityInput::HandleGameEvents, this, std::placeholders::_1);
  _signalHandles.emplace_back( _externalInterface.Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioEvent, callback ) );
  _signalHandles.emplace_back( _externalInterface.Subscribe( ExternalInterface::MessageGameToEngineTag::StopAllAudioEvents, callback ) );
  _signalHandles.emplace_back( _externalInterface.Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioGameState, callback ) );
  _signalHandles.emplace_back( _externalInterface.Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioSwitchState, callback ) );
  _signalHandles.emplace_back( _externalInterface.Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioParameter, callback ) );
  _signalHandles.emplace_back( _externalInterface.Subscribe( ExternalInterface::MessageGameToEngineTag::PostAudioMusicState, callback ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioUnityInput::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  PRINT_CH_DEBUG(AudioMuxInput::kAudioLogChannel,
                 "AudioUnityInput.HandleGameEvents", "Handle game event of type %s !",
                 ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) );
  
  switch ( event.GetData().GetTag() ) {

    case ExternalInterface::MessageGameToEngineTag::PostAudioEvent:
      HandleMessage( event.GetData().Get_PostAudioEvent() );
      break;
      
    case ExternalInterface::MessageGameToEngineTag::StopAllAudioEvents:
      HandleMessage( event.GetData().Get_StopAllAudioEvents() );
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
      
    case ExternalInterface::MessageGameToEngineTag::PostAudioMusicState:
      HandleMessage( event.GetData().Get_PostAudioMusicState() );
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "AudioUnityInput.HandleGameEvents",
                         "Subscribed to unhandled event of type %s !",
                         ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioUnityInput::PostCallback( const AudioEngine::Multiplexer::AudioCallback& callbackMessage ) const
{
  const ExternalInterface::MessageEngineToGame msg( (AudioEngine::Multiplexer::AudioCallback( callbackMessage)) );
  _externalInterface.Broadcast( std::move( msg ) );
}


} // Audio
} // Cozmo
} // Anki
