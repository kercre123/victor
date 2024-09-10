/*
 * File: audioMuxClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a base class that provides communication between its self and an AudioMuxInput by means of that
 *              is defined in the sub-class. It provides core audio functionality by using Audio CLAD messages as the
 *              transport interface.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#include "audioEngine/multiplexer/audioMuxClient.h"
#include "clad/audio/messageAudioClient.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


namespace Anki {
namespace AudioEngine {
namespace Multiplexer {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AudioMuxClient class
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxClient::AudioMuxClient()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxClient::~AudioMuxClient()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxClient::CallbackIdType AudioMuxClient::PostEvent( AudioMetaData::GameEvent::GenericEvent event,
                                                          AudioMetaData::GameObjectType gameObject,
                                                          CallbackFunc&& callback )
{
  PRINT_NAMED_WARNING("AudioMuxClient.PostEvent", "Base class can NOT post Event");
  return kInvalidCallbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::StopAllEvents( AudioMetaData::GameObjectType gameObject )
{
    PRINT_NAMED_WARNING("AudioMuxClient.StopAllEvents", "Base class can NOT Stop All Events");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::PostGameState( AudioMetaData::GameState::StateGroupType gameStateGroup,
                                    AudioMetaData::GameState::GenericState gameState )
{
    PRINT_NAMED_WARNING("AudioMuxClient.PostGameState", "Base class can NOT post Game State");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::PostSwitchState( AudioMetaData::SwitchState::SwitchGroupType switchGroup,
                                      AudioMetaData::SwitchState::GenericSwitch switchState,
                                      AudioMetaData::GameObjectType gameObject )
{
    PRINT_NAMED_WARNING("AudioMuxClient.PostSwitchState", "Base class can NOT post Switch State");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::PostParameter( AudioMetaData::GameParameter::ParameterType parameter,
                                    float parameterValue,
                                    AudioMetaData::GameObjectType gameObject,
                                    int32_t timeInMilliSeconds,
                                    CurveType curve ) const
{
    PRINT_NAMED_WARNING("AudioMuxClient.PostParameter", "Base class can NOT post Parameter w/ GameObject");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::PostMusicState( AudioMetaData::GameState::GenericState musicState,
                                     bool interrupt,
                                     uint32_t minDuration_ms )
{
    PRINT_NAMED_WARNING("AudioMuxClient.PostMusicState", "Base class can NOT post Music State");
}
  
  
// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::HandleCallbackEvent( const AudioCallbackDuration& callbackMsg )
{
  const AudioCallback callback( &callbackMsg );
  PerformCallback( callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::HandleCallbackEvent( const AudioCallbackMarker& callbackMsg )
{
  const AudioCallback callback( &callbackMsg );
  PerformCallback( callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::HandleCallbackEvent( const AudioCallbackComplete& callbackMsg )
{
  const AudioCallback callback( &callbackMsg );
  PerformCallback( callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::HandleCallbackEvent( const AudioCallbackError& callbackMsg )
{
  const AudioCallback callback( &callbackMsg );
  PerformCallback( callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxClient::CallbackIdType AudioMuxClient::ManageCallback( CallbackFunc&& callbackFunc )
{
  if ( nullptr != callbackFunc ) {
    const auto callbackId = GetNewCallbackId();
    _callbackMap.emplace( callbackId, std::move( callbackFunc ) );
    return callbackId;
  }
  return kInvalidCallbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMuxClient::PerformCallback( const AudioCallback& callbackData )
{
  const auto it = _callbackMap.find( callbackData.GetId() );
  if ( it == _callbackMap.end() ) {
    PRINT_NAMED_WARNING("AudioMuxClient.PerformCalback", "Can't find CallbackId %d for callback type %hhu",
                        callbackData.GetId(), callbackData.GetType());
    return;
  }
  // Perform callback
  it->second(callbackData);
  // Clean up callback func
  if ( callbackData.GetType() == CallbackType::Complete ||
       callbackData.GetType() == CallbackType::Error ) {
    _callbackMap.erase( it );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMuxClient::CallbackIdType AudioMuxClient::GetNewCallbackId()
{
  // We intentionally let the callback id roll over
  CallbackIdType callbackId = ++_previousCallbackId;
  if ( kInvalidCallbackId == callbackId ) {
    ++callbackId;
  }
  
  return callbackId;
}


} // Multiplexer
} // AudioEngine
} // Anki
