/*
 * File: audioEngineInput.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a subclass of AudioMuxInput which provides communication between itself and an
 *              AudioEngineClient by means of AudioEngineMessageHandler.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/audioEngineInput.h"
#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioEngine::Multiplexer;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineInput::AudioEngineInput( AudioEngineMessageHandler* messageHandler ) :
  _messageHandler( messageHandler )
{
  DEV_ASSERT(nullptr != messageHandler, "Message Handler is NULL!");
  
  // Subscribe to Connection Side Messages
  auto callback = std::bind(&AudioEngineInput::HandleEvents, this, std::placeholders::_1);
  AddSignalHandle( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioEvent, callback ) );
  AddSignalHandle( _messageHandler->Subscribe( MessageAudioClientTag::StopAllAudioEvents, callback ) );
  AddSignalHandle( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioGameState, callback ) );
  AddSignalHandle( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioSwitchState, callback ) );
  AddSignalHandle( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioParameter, callback ) );
  AddSignalHandle( _messageHandler->Subscribe( MessageAudioClientTag::PostAudioMusicState, callback ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineInput::~AudioEngineInput()
{
  ClearSignalHandles();
  Util::SafeDelete( _messageHandler );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineInput::PostCallback( const AudioEngine::Multiplexer::AudioCallback& callbackMessage ) const
{
  const MessageAudioClient msg(( AudioCallback( callbackMessage ) ));
  _messageHandler->Broadcast( std::move( msg ) );
}


// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineInput::HandleEvents(const AnkiEvent<AudioEngine::Multiplexer::MessageAudioClient>& event)
{
  PRINT_CH_DEBUG(AudioMuxInput::kAudioLogChannel,
                 "AudioEngineInput.HandleGameEvents",
                 "Handle game event of type %s !",
                 MessageAudioClientTagToString(event.GetData().GetTag()));
  switch ( event.GetData().GetTag() ) {
      
    case MessageAudioClientTag::PostAudioEvent:
      HandleMessage( event.GetData().Get_PostAudioEvent() );
      break;
      
    case MessageAudioClientTag::StopAllAudioEvents:
      HandleMessage( event.GetData().Get_StopAllAudioEvents() );
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
      
    case MessageAudioClientTag::PostAudioMusicState:
      HandleMessage( event.GetData().Get_PostAudioMusicState() );
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "AudioEngineInput.HandleGameEvents",
                         "Subscribed to unhandled event of type %s !",
                         MessageAudioClientTagToString(event.GetData().GetTag()) );
    }
  }
}

} // Audio
} // Cozmo
} // Anki
