/*
 * File: animationAudioClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 09/12/17
 *
 * Description: Animation Audio Client is the interface to perform animation audio specific tasks. Provided a
 *              RobotAudioKeyFrame to handle the necessary audio functionality for that frame. It also provides an
 *              interface to abort animation audio and update (a.k.a. “tick”) the Audio Engine each frame.
 *
 * Copyright: Anki, Inc. 2017
 */


#include "cozmoAnim/audio/animationAudioClient.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/animation/keyframe.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"


#define kEnableAudioEventProbability 1

#define ENABLE_DEBUG_LOG 0
#if ENABLE_DEBUG_LOG
  #define AUDIO_DEBUG_LOG( name, format, args... ) PRINT_CH_DEBUG( "Audio", name, format, ##args )
#else
  #define AUDIO_DEBUG_LOG( name, format, args... )
#endif

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioEngine;
using namespace AudioMetaData;

static const AudioGameObject kAnimGameObj = static_cast<AudioGameObject>(GameObjectType::Cozmo_OnDevice);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationAudioClient::AnimationAudioClient( CozmoAudioController* audioController,
                                            Util::RandomGenerator* randomGenerator )
: _audioController( audioController )
, _randomGenerator( randomGenerator )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationAudioClient::~AnimationAudioClient()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::Update() const
{
  if ( _audioController == nullptr ) { return; }
  _audioController->Update();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::PlayAudioKeyFrame( const RobotAudioKeyFrame& keyFrame )
{
  const auto& audioRef = keyFrame.GetAudioRef();
  
  // Post Audio Event
  // First, check if the audio event is valid
  if ( AudioMetaData::GameEvent::GenericEvent::Invalid == audioRef.audioEvent ) {
    PRINT_NAMED_ERROR("AnimationAudioClient.PlayAudioKeyFrame", "Invalid Audio event in RobotAudioKeyFrame");
    return;
  }
  
  // Check event probability
  static bool allowEventProbability = ( kEnableAudioEventProbability && ( nullptr != _randomGenerator ) );
  auto audioEvent = audioRef.audioEvent;
  if ( allowEventProbability &&
       !Util::IsFltNear( audioRef.probability, 1.0f ) &&
       audioRef.probability < _randomGenerator->RandDbl( 1.0 ) ) {
    // Chance has chosen not to play event
    audioEvent = AudioMetaData::GameEvent::GenericEvent::Invalid;
  }
  
  // Play valid event
  if ( AudioMetaData::GameEvent::GenericEvent::Invalid != audioEvent ) {
    const auto playId = PostCozmoEvent( audioEvent );
    
    // Apply volume to event
    if ( playId != kInvalidAudioPlayingId ) {
      SetCozmoEventParameter(playId, GameParameter::ParameterType::Event_Volume, audioRef.volume);
    }
    AUDIO_DEBUG_LOG("AnimationAudioClient.PlayAudioKeyFrame", "Post Event '%s' volume %f probability %f",
                    EnumToString(audioEvent), audioRef.volume, audioRef.probability);
  }
  else {
    AUDIO_DEBUG_LOG("AnimationAudioClient.PlayAudioKeyFrame",
                    "Probability failed to Post Event '%s' volume %f probability %f",
                    EnumToString(audioRef.audioEvent), audioRef.volume, audioRef.probability);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::StopCozmoEvent()
{
  if ( _audioController == nullptr ) { return; }
  _audioController->StopAllAudioEvents( kAnimGameObj );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationAudioClient::HasActiveEvents() const
{
  std::lock_guard<std::mutex> lock( _lock );
  return !_activeEvents.empty();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationAudioClient::AnimPlayId AnimationAudioClient::PostCozmoEvent( AudioMetaData::GameEvent::GenericEvent event )
{
  if ( _audioController == nullptr ) { return kInvalidAudioPlayingId; }

  const auto audioEventId = Util::numeric_cast<AudioEventId>( event );
  AudioCallbackContext* audioCallbackContext = nullptr;
  const auto callbackFunc = std::bind(&AnimationAudioClient::CozmoEventCallback, this, std::placeholders::_1);
  audioCallbackContext = new AudioCallbackContext();
  // Set callback flags
  audioCallbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
  // Execute callbacks synchronously (on main thread)
  audioCallbackContext->SetExecuteAsync( false );
  // Register callbacks for event
  audioCallbackContext->SetEventCallbackFunc ( [ callbackFunc = std::move(callbackFunc) ]
                                               ( const AudioCallbackContext* thisContext, const AudioCallbackInfo& callbackInfo )
                                               {
                                                 callbackFunc( callbackInfo );
                                               } );

  const AudioEngine::AudioPlayingId playId = _audioController->PostAudioEvent( audioEventId,
                                                                               kAnimGameObj,
                                                                               audioCallbackContext );
  
  // Track event playback
  AddActiveEvent( playId );
  
  return playId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationAudioClient::SetCozmoEventParameter( AnimPlayId playId, AudioMetaData::GameParameter::ParameterType parameter, float value ) const
{
  if ( _audioController == nullptr ) { return false; }
  const AudioParameterId parameterId = Util::numeric_cast<AudioParameterId>( parameter );
  const AudioRTPCValue rtpcVal = Util::numeric_cast<AudioRTPCValue>( value );
  const AudioPlayingId audioPlayId = Util::numeric_cast<AudioPlayingId>( playId );
  return _audioController->SetParameterWithPlayingId( parameterId, rtpcVal, audioPlayId );
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::CozmoEventCallback( const AudioEngine::AudioCallbackInfo& callbackInfo )
{
  switch (callbackInfo.callbackType) {
      
    case AudioEngine::AudioCallbackType::Complete:
    {
      const auto& info = static_cast<const AudioCompletionCallbackInfo&>( callbackInfo );
      RemoveActiveEvent( info.playId );
      AUDIO_DEBUG_LOG("AnimationAudioClient.PostCozmoEvent.Callback", "%s", info.GetDescription().c_str());
    }
      break;
      
    case AudioCallbackType::Error:
    {
      const auto& info = static_cast<const AudioErrorCallbackInfo&>( callbackInfo );
      RemoveActiveEvent( info.playId );
      PRINT_NAMED_WARNING("AnimationAudioClient.PostCozmoEvent.CallbackError", "%s", info.GetDescription().c_str());
    }
      break;
      
    case AudioEngine::AudioCallbackType::Duration:
    {
      const auto& info = static_cast<const AudioDurationCallbackInfo&>( callbackInfo );
      PRINT_NAMED_WARNING("AnimationAudioClient.PostCozmoEvent.CallbackUnexpected", "%s", info.GetDescription().c_str());
    }
      break;
      
    case AudioEngine::AudioCallbackType::Marker:
    {
      const auto& info = static_cast<const AudioMarkerCallbackInfo&>( callbackInfo );
      PRINT_NAMED_WARNING("AnimationAudioClient.PostCozmoEvent.CallbackUnexpected", "%s", info.GetDescription().c_str());
    }
      break;
      
    case AudioEngine::AudioCallbackType::Invalid:
    {
      PRINT_NAMED_WARNING("AnimationAudioClient.PostCozmoEvent.CallbackInvalid", "%s", callbackInfo.GetDescription().c_str());
    }
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::AddActiveEvent( AnimPlayId playId )
{
  if ( playId != kInvalidAudioPlayingId ) {
    std::lock_guard<std::mutex> lock( _lock );
    _activeEvents.emplace( playId );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::RemoveActiveEvent( AnimPlayId playId )
{
  std::lock_guard<std::mutex> lock( _lock );
  _activeEvents.erase( playId );
}

}
}
}
