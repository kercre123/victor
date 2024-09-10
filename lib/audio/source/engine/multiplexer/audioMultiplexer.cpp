/*
 * File: audioMultiplexer.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: The AudioMultiplexer multiplexes AudioMuxInputs messages to perform AudioEngineController
 *              functionality. The Input’s Audio Post Messages is decoded to perform audio tasks. When the
 *              AudioEngineController performs a callback it is encoded into Audio Callback Messages and is passed to
 *              the appropriate Input to transport to its Client. The AudioMultiplexer owns the Audio Input.
 *
 * Copyright: Anki, Inc. 2015
 */


#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "audioEngine/multiplexer/audioMuxInput.h"
#include "audioEngine/audioEngineController.h"
#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/musicConductor.h"
#include "clad/audio/audioMessage.h"
#include "audioEngine/audioCallback.h"
#include "clad/audio/audioCallbackMessage.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"


namespace Anki {
namespace AudioEngine {
namespace Multiplexer {

#define kLogChannelName AudioEngineController::kLogChannelName
AudioGameObject ConvertToAudioGameObject(AudioMetaData::GameObjectType gameObject);
  
AudioMultiplexer::AudioMultiplexer( AudioEngine::AudioEngineController* audioController )
: _audioController( audioController )
{
  DEV_ASSERT(nullptr != _audioController, "AudioMultiplexer Audio Controller is NULL");
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMultiplexer::~AudioMultiplexer()
{
  Anki::Util::SafeDelete( _audioController );
  
  for ( auto& kvp : _inputs ) {
    Anki::Util::SafeDelete( kvp.second );
  }
  _inputs.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMultiplexer::InputIdType AudioMultiplexer::RegisterInput( AudioMuxInput* input )
{
  // Setup Input with Multiplexer
  DEV_ASSERT(nullptr != input, "AudioMultiplexer.RegisterInput input is NULL!");
  InputIdType inputId = GetNewInputId();
  input->SetInputId( inputId );
  input->SetAudioMultiplexer( this );

  _inputs.emplace( inputId, input );
  
  return inputId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxInput* AudioMultiplexer::GetInput( uint8_t inputId ) const
{
  const auto it = _inputs.find( inputId );
  if ( it != _inputs.end() ) {
    return it->second;
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::ProcessMessage( const PostAudioEvent& message, InputIdType inputId )
{
  // Decode Event Message
  const AudioEventId eventId = ToAudioEventId( message.audioEvent );
  const AudioGameObject objectId = ConvertToAudioGameObject( message.gameObject );
  const uint16_t callbackId = message.callbackId;
  const AudioEngine::AudioCallbackFlag callbackFlags = (callbackId == 0) ?
                                                        AudioEngine::AudioCallbackFlag::NoCallbacks :
                                                        AudioEngine::AudioCallbackFlag::AllCallbacks;

  // Perform Event
  AudioPlayingId playingId = kInvalidAudioPlayingId;
  if ( AudioEngine::AudioCallbackFlag::NoCallbacks != callbackFlags ) {
    AudioCallbackContext* callbackContext = new AudioCallbackContext();
    // Set callback flags
    callbackContext->SetCallbackFlags( callbackFlags );
    // Execute callbacks synchronously (on main thread)
    callbackContext->SetExecuteAsync( false );
    // Register callbacks for event
    callbackContext->SetEventCallbackFunc( [this, inputId, callbackId]
                                           ( const AudioCallbackContext* thisContext,
                                             const AudioEngine::AudioCallbackInfo& callbackInfo )
    {
      PerformCallback( inputId, callbackId, callbackInfo );
    } );
    
    playingId = _audioController->PostAudioEvent( eventId, objectId, callbackContext );
  }
  else {
    playingId = _audioController->PostAudioEvent( eventId, objectId );
  }
  
  if ( playingId == kInvalidAudioPlayingId ) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioMultiplexer.ProcessMessage", "Unable to Play Event %s on GameObject %llu",
                   EnumToString( message.audioEvent ), message.gameObject );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::ProcessMessage( const StopAllAudioEvents& message, InputIdType inputId )
{
  if ( _audioController == nullptr ) { return; }
  const AudioGameObject objectId = ConvertToAudioGameObject( message.gameObject );
  _audioController->StopAllAudioEvents( objectId );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::ProcessMessage( const PostAudioGameState& message, InputIdType inputId )
{
  // Decode Game State Message
  if ( _audioController == nullptr ) { return; }
  const AudioStateGroupId groupId = ToAudioStateGroupId( message.stateGroup );
  const AudioStateId stateId = ToAudioStateId( message.stateValue );
  // Perform Game State
  const bool success = _audioController->SetState( groupId, stateId );
  if ( !success ) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioMultiplexer.ProcessMessage", "Unable to Set State %s : %s",
                   EnumToString( message.stateGroup ), EnumToString( message.stateValue ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::ProcessMessage( const PostAudioSwitchState& message, InputIdType inputId )
{
  // Decode Switch State Message
  if ( _audioController == nullptr ) { return; }
  const AudioSwitchGroupId groupId = ToAudioSwitchGroupId( message.switchStateGroup);
  const AudioSwitchStateId stateId = ToAudioSwitchStateId( message.switchStateValue );
  const AudioGameObject objectId = ConvertToAudioGameObject(message.gameObject );
  // Perform Switch State
  const bool success = _audioController->SetSwitchState( groupId, stateId, objectId );
  if ( !success ) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioMultiplexer.ProcessMessage", "Unable to Set Switch State %s : %s on GameObject %s",
                   EnumToString( message.switchStateGroup ), EnumToString( message.switchStateValue ),
                   EnumToString( message.gameObject ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::ProcessMessage( const PostAudioParameter& message, InputIdType inputId )
{
  // Decode Parameter Message
  if ( _audioController == nullptr ) { return; }
  const AudioParameterId parameterId = ToAudioParameterId( message.parameter );
  const AudioRTPCValue value = ToAudioRTPCValue( message.parameterValue );
  const AudioTimeMs duration = ToAudioTimeMs( message.timeInMilliSeconds );
  const AudioCurveType curve = ToAudioCurveType( message.curve );
  const AudioGameObject objectId = ConvertToAudioGameObject( message.gameObject );
  
  
  
  // Perform Parameter
  const bool success = _audioController->SetParameter( parameterId, value, objectId, duration, curve );
  if ( !success ) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioMultiplexer.ProcessMessage",
                   "Unable to Set Parameter %s to Value %f on GameObject %s with duration %d milliSeconds with \
                   curve type %s",
                   EnumToString( message.parameter ), message.parameterValue, EnumToString( message.gameObject ),
                   message.timeInMilliSeconds, EnumToString( message.curve ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::ProcessMessage( const PostAudioMusicState& message, InputIdType inputId )
{
  // Decode Music Message
  if ( _audioController == nullptr ) { return; }
  const AudioStateId stateId = ToAudioStateId( message.stateValue );
  const bool interrupt = message.interrupt;
  const uint32_t minDuration_ms = Anki::Util::numeric_cast<uint32_t>( message.minDurationInMilliSeconds );
  _audioController->GetMusicConductor()->SetMusicState( stateId, interrupt, minDuration_ms );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::UpdateAudioController()
{
  _audioController->Update();
}


// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMultiplexer::InputIdType AudioMultiplexer::GetNewInputId()
{
  // This only works for 255 Inputs
  ++_previousInputId;
  DEV_ASSERT(0 != _previousInputId,
             "AudioMultiplexer.GetNewInputId Invalid InputId, this can be caused by adding more then 255 inputs");
  
  return _previousInputId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMultiplexer::PerformCallback( InputIdType inputId,
                                        uint16_t callbackId,
                                        const AudioEngine::AudioCallbackInfo& callbackInfo )
{
  PRINT_CH_DEBUG(kLogChannelName,
                 "AudioMultiplexer.PerformCallback",
                 "Event Callback InputId %d CalbackId: %d - %s",
                 inputId, callbackId, callbackInfo.GetDescription().c_str());
  
  const AudioMuxInput* input = GetInput( inputId );
  if ( nullptr != input ) {    
    
    switch ( callbackInfo.callbackType ) {
        
      case AudioEngine::AudioCallbackType::Invalid:
        // Do nothing
        PRINT_NAMED_ERROR( "AudioMultiplexer.PerformCallback", "Invalid Callback" );
        break;
        
      case AudioCallbackType::Duration:
      {
        const AudioDurationCallbackInfo& info = static_cast<const AudioDurationCallbackInfo&>( callbackInfo );
        AudioCallbackDuration callbackType;
        callbackType.duration           = info.duration;
        callbackType.estimatedDuration  = info.estimatedDuration;
        callbackType.audioNodeId        = info.audioNodeId;
        callbackType.callbackId         = callbackId;
        callbackType.isStreaming        = info.isStreaming;
        input->PostCallback( std::move( callbackType ) );
      }
        break;
        
      case AudioCallbackType::Marker:
      {
        PRINT_NAMED_ERROR("AudioMultiplexer.PerformCallback",
                          "AudioMultiplexer.PerformCallback.AudioCallbackType.Marker.IsNotComplete");
        const AudioMarkerCallbackInfo& info = static_cast<const AudioMarkerCallbackInfo&>( callbackInfo );
        AudioCallbackMarker callbackType;
        callbackType.identifier = info.identifier;
        callbackType.position   = info.position,
        callbackType.callbackId = callbackId;
        // callbackType.labelTitle = info.labelStr; // TODO: Fix label tile string Jira ticket VIC-22
      }
        break;
        
      case AudioCallbackType::Complete:
      {
        const AudioCompletionCallbackInfo& info = static_cast<const AudioCompletionCallbackInfo&>( callbackInfo );
        AudioCallbackComplete callbackType;
        callbackType.eventType  = static_cast<AudioMetaData::GameEvent::GenericEvent>( info.eventId );
        callbackType.callbackId = callbackId;
        input->PostCallback( std::move( callbackType ) );
      }
        break;
        
      case AudioCallbackType::Error:
      {
        const AudioErrorCallbackInfo& info = static_cast<const AudioErrorCallbackInfo&>( callbackInfo );
        AudioCallbackError callbackType;
        callbackType.callbackId     = callbackId;
        callbackType.callbackError  = ConvertErrorCallbackType( info.error );
        input->PostCallback( std::move( callbackType ) );
      }
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CallbackErrorType AudioMultiplexer::ConvertErrorCallbackType( const AudioEngine::AudioCallbackErrorType errorType ) const
{
  CallbackErrorType error = CallbackErrorType::Invalid;
  
  switch ( errorType ) {
      
    case AudioEngine::AudioCallbackErrorType::Invalid:
      // Do nothing
      break;
      
    case AudioEngine::AudioCallbackErrorType::EventFailed:
      error = CallbackErrorType::EventFailed;
      break;
      
    case AudioEngine::AudioCallbackErrorType::Starvation:
      error = CallbackErrorType::Starvation;
      break;
  }
  
  return error;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioGameObject ConvertToAudioGameObject(AudioMetaData::GameObjectType gameObject)
{
  if (gameObject == AudioMetaData::GameObjectType::Invalid) {
    return kInvalidAudioGameObject;
  }
  return ToAudioGameObject(gameObject);
}
  
} // Multiplexer
} // AudioEngine
} // Anki
