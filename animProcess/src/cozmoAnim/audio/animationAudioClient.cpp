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


#include "audioEngine/audioTypeTranslator.h"
#include "cozmoAnim/audio/animationAudioClient.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cannedAnimLib/keyframe.h"
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
using namespace AudioKeyFrameType;

static const AudioGameObject kAnimGameObj = ToAudioGameObject(GameObjectType::Cozmo_OnDevice);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationAudioClient::AnimationAudioClient( CozmoAudioController* audioController )
: _audioController( audioController )
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
void AnimationAudioClient::PlayAudioKeyFrame( const RobotAudioKeyFrame& keyFrame, Util::RandomGenerator* randomGen )
{
  // Set states, switches and parameters before events events.
  // NOTE: The order is corrected when loading the animation. (see keyframe.cpp RobotAudioKeyFrame methods)
  // NOTE: If the same State, Switch or Parameter is set more then once on a single frame, last one in wins!
  const RobotAudioKeyFrame::AudioRefList& audioRefs = keyFrame.GetAudioReferencesList();
  for ( const auto& ref : audioRefs ) {
    switch ( ref.Tag ) {
      case AudioRefTag::EventGroup:
        HandleAudioRef( ref.EventGroup, randomGen );
        break;
      case AudioRefTag::State:
        HandleAudioRef( ref.State );
        break;
      case AudioRefTag::Switch:
        HandleAudioRef( ref.Switch );
        break;
      case AudioRefTag::Parameter:
        HandleAudioRef( ref.Parameter );
        break;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::HandleAudioRef( const AudioEventGroupRef& eventRef, Util::RandomGenerator* randomGen )
{
  const AudioEventGroupRef::EventDef* anEvent = eventRef.RetrieveEvent( true, randomGen );
  if (nullptr == anEvent) {
    // Chance has chosen not to play an event
    return;
  }

  // Play valid event
  const auto playId = PostCozmoEvent( anEvent->AudioEvent );
  if ( playId != kInvalidAudioPlayingId ) {
    // Apply volume to event
    SetCozmoEventParameter( playId, GameParameter::ParameterType::Event_Volume, anEvent->Volume );
  }
  AUDIO_DEBUG_LOG("AnimationAudioClient.PlayAudioKeyFrame",
                  "Posted audio event '%s' (volume %f, probability %f)",
                  EnumToString(audioRef.audioEvent), audioRef.volume, audioRef.probability);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::HandleAudioRef( const AudioStateRef& stateRef )
{
  _audioController->SetState( ToAudioStateGroupId( stateRef.StateGroup ), ToAudioStateId( stateRef.State ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::HandleAudioRef( const AudioSwitchRef& switchRef )
{
  _audioController->SetSwitchState( ToAudioSwitchGroupId( switchRef.SwitchGroup ),
                                    ToAudioSwitchStateId( switchRef.State ),
                                    kAnimGameObj );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::HandleAudioRef( const AudioParameterRef& parameterRef )
{
  _audioController->SetParameter( ToAudioParameterId( parameterRef.Parameter ),
                                  ToAudioRTPCValue( parameterRef.Value ),
                                  kAnimGameObj,
                                  ToAudioTimeMs( parameterRef.Time_ms ),
                                  ToAudioCurveType( parameterRef.Curve ) );
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
AudioEngine::AudioPlayingId AnimationAudioClient::PostCozmoEvent( AudioMetaData::GameEvent::GenericEvent event )
{
  if ( _audioController == nullptr ) { return kInvalidAudioPlayingId; }

  const auto audioEventId = ToAudioEventId( event );
  AudioCallbackContext* audioCallbackContext = nullptr;
  const auto callbackFunc = std::bind(&AnimationAudioClient::CozmoEventCallback, this, std::placeholders::_1);
  audioCallbackContext = new AudioCallbackContext();
  // Set callback flags
  audioCallbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
  // Execute callbacks synchronously (on main thread)
  audioCallbackContext->SetExecuteAsync( false );
  // Register callbacks for event
  audioCallbackContext->SetEventCallbackFunc ( [ callbackFunc = std::move(callbackFunc) ]
                                               ( const AudioCallbackContext* thisContext,
                                                 const AudioCallbackInfo& callbackInfo )
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
bool AnimationAudioClient::SetCozmoEventParameter( AudioEngine::AudioPlayingId playId,
                                                   AudioMetaData::GameParameter::ParameterType parameter,
                                                   AudioEngine::AudioRTPCValue value ) const
{
  if ( _audioController == nullptr ) { return false; }
  return _audioController->SetParameterWithPlayingId( ToAudioParameterId( parameter ), value, playId );
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
void AnimationAudioClient::AddActiveEvent( AudioEngine::AudioPlayingId playId )
{
  if ( playId != kInvalidAudioPlayingId ) {
    std::lock_guard<std::mutex> lock( _lock );
    _activeEvents.emplace( playId );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAudioClient::RemoveActiveEvent( AudioEngine::AudioPlayingId playId )
{
  std::lock_guard<std::mutex> lock( _lock );
  _activeEvents.erase( playId );
}

}
}
}
