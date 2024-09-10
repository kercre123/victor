/*
 * File: audioMuxInput.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is the base class for Audio Mux Input which handle communication between the Multiplexer and the
 *              Mux inputs. It provides core audio functionality by using Audio CLAD messages as the transport
 *              interface.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#include "audioEngine/multiplexer/audioMuxInput.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


namespace Anki {
namespace AudioEngine {
namespace Multiplexer {

const char* AudioMuxInput::kAudioLogChannel = "Audio";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxInput::AudioMuxInput()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxInput::~AudioMuxInput()
{
  // We don't own the Multiplexer
  _mux = nullptr;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxInput::HandleMessage( const PostAudioEvent& eventMessage )
{
  if ( nullptr != _mux ) {
    _mux->ProcessMessage( eventMessage, GetInputId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioMuxInput.HandleMessage" , "AudioMultiplexer is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxInput::HandleMessage( const StopAllAudioEvents& stopEventMessage )
{
  if ( nullptr != _mux ) {
    _mux->ProcessMessage( stopEventMessage, GetInputId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioMuxInput.HandleMessage" , "AudioMultiplexer is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxInput::HandleMessage( const PostAudioGameState& gameStateMessage )
{
  if ( nullptr != _mux ) {
    _mux->ProcessMessage( gameStateMessage, GetInputId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioMuxInput.HandleMessage" , "AudioMultiplexer is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxInput::HandleMessage( const PostAudioSwitchState& switchStateMessage )
{
  if ( nullptr != _mux ) {
    _mux->ProcessMessage( switchStateMessage, GetInputId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioMuxInput.HandleMessage" , "AudioMultiplexer is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxInput::HandleMessage( const PostAudioParameter& parameterMessage )
{
  if ( nullptr != _mux ) {
    _mux->ProcessMessage( parameterMessage, GetInputId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioMuxInput.HandleMessage" , "AudioMultiplexer is NULL" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxInput::HandleMessage( const PostAudioMusicState& musicStateMessage )
{
  if ( nullptr != _mux ) {
    _mux->ProcessMessage( musicStateMessage, GetInputId() );
  }
  else {
    PRINT_NAMED_ERROR( "AudioMuxInput.HandleMessage" , "AudioMultiplexer is NULL" );
  }
}
  
} // Multiplexer
} // AudioEngine
} // Anki 
