/*
 * File: robotAudioClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnDevice.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "clad/audio/messageAudioClient.h"
#include "DriveAudioEngine/audioCallback.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {
namespace Audio {
  
const char* RobotAudioClient::kRobotAudioLogChannelName = AudioController::kAudioLogChannelName;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::RobotAudioClient( Robot* robot )
: _robot( robot )
{
  if (_robot == nullptr) {
    return;
  }
  const CozmoContext* context = _robot->GetContext();
  // For Unit Test bale out if there is no Audio Server
  if ( context->GetAudioServer() == nullptr ) {
    return;
  }

  _audioController = context->GetAudioServer()->GetAudioController();
  
  auto robotVolumeCallback = [this] ( const AnkiEvent<ExternalInterface::MessageGameToEngine>& message ) {
    const ExternalInterface::SetRobotVolume& msg = message.GetData().Get_SetRobotVolume();
    SetRobotVolume( msg.volume );
  };

  auto robotAudioOutputSourceCallback = [this] ( const AnkiEvent<ExternalInterface::MessageGameToEngine>& message ){
    const ExternalInterface::SetRobotAudioOutputSource& msg = message.GetData().Get_SetRobotAudioOutputSource();

    RobotAudioOutputSource outputSource;


    // Switch case is needed to "cast" the CLAD generated RobotAudioOutputSource enum into
    // RobotAudioClient::RobotAudioOutputSource. This allows RobotAudioOutputSource to stay in
    // RobotAudioClient (instead of solely referencing the enum from the CLAD generated headers, in
    // order to limit CLAD facing code in the rest of the audio codebase).

    switch (msg.source)
    {
      case ExternalInterface::RobotAudioOutputSourceCLAD::NoDevice:
        outputSource = RobotAudioOutputSource::None;
        break;

      case ExternalInterface::RobotAudioOutputSourceCLAD::PlayOnDevice:
        outputSource = RobotAudioOutputSource::PlayOnDevice;
        break;

      case ExternalInterface::RobotAudioOutputSourceCLAD::PlayOnRobot:
        outputSource = RobotAudioOutputSource::PlayOnRobot;
        break;
    }

    SetOutputSource( outputSource );
    PRINT_CH_DEBUG(kRobotAudioLogChannelName,
                   "RobotAudioClient.RobotAudioClient.RobotAudioOutputSourceCallback",
                   "outputSource: %hhu", msg.source);
  };
  
  IExternalInterface* gameToEngineInterface = context->GetExternalInterface();
  if ( gameToEngineInterface ) {
    PRINT_CH_DEBUG(kRobotAudioLogChannelName,
                   "RobotAudioClient.RobotAudioClient", "gameToEngineInterface exists");

    _signalHandles.push_back(
      gameToEngineInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::SetRobotVolume,
                                       robotVolumeCallback)
    );

    _signalHandles.push_back(
      gameToEngineInterface->Subscribe(
        ExternalInterface::MessageGameToEngineTag::SetRobotAudioOutputSource,
        robotAudioOutputSourceCallback
      )
    );
  }

  // Configure Robot Audio buffers with Wwise buses. PlugIn Ids are set in Wwise project
  // Setup Robot Buffers
  // Note: This is only configured to work with a single robot
  RegisterRobotAudioBuffer( GameObjectType::CozmoAnimation, 1, Bus::BusType::Robot_Bus_1 );
  
  // TEMP: Setup other buses
  RegisterRobotAudioBuffer( GameObjectType::CozmoBus_2, 2, Bus::BusType::Robot_Bus_2 );
  RegisterRobotAudioBuffer( GameObjectType::CozmoBus_3, 3, Bus::BusType::Robot_Bus_3 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* RobotAudioClient::GetRobotAudiobuffer( GameObjectType gameObject )
{
  ASSERT_NAMED( _audioController != nullptr, "RobotAudioClient.GetRobotAudiobuffer.AudioControllerNull" );
  const AudioEngine::AudioGameObject aGameObject = static_cast<const AudioEngine::AudioGameObject>( gameObject );
  return _audioController->GetRobotAudioBufferWithGameObject( aGameObject );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::CozmoPlayId RobotAudioClient::PostCozmoEvent( GameEvent::GenericEvent event,
                                                                GameObjectType gameObjId,
                                                                const CozmoEventCallbackFunc& callbackFunc ) const
{
  const auto audioEventId = Util::numeric_cast<AudioEngine::AudioEventId>( event );
  const auto audioGameObjId = static_cast<AudioEngine::AudioGameObject>( gameObjId );
  AudioEngine::AudioCallbackContext* audioCallbackContext = nullptr;
  
  if ( callbackFunc != nullptr ) {
    audioCallbackContext = new AudioEngine::AudioCallbackContext();
    // Set callback flags
    audioCallbackContext->SetCallbackFlags( AudioEngine::AudioCallbackFlag::AllCallbacks );
    // Register callbacks for event
    audioCallbackContext->SetEventCallbackFunc ( [ callbackFunc ]
    ( const AudioEngine::AudioCallbackContext* thisContext, const AudioEngine::AudioCallbackInfo& callbackInfo )
    {
      callbackFunc( callbackInfo );
    } );
  }
  
  return _audioController->PostAudioEvent( audioEventId, audioGameObjId, audioCallbackContext );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::SetCozmoEventParameter( CozmoPlayId playId, GameParameter::ParameterType parameter, float value ) const
{
  using namespace AudioEngine;
  const AudioParameterId parameterId = Util::numeric_cast<AudioParameterId>( parameter );
  const AudioRTPCValue rtpcVal = Util::numeric_cast<AudioRTPCValue>( value );
  const AudioPlayingId audioPlayId = Util::numeric_cast<AudioPlayingId>( playId );
  return _audioController->SetParameterWithPlayingId( parameterId, rtpcVal, audioPlayId );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::StopCozmoEvent(GameObjectType gameObjId)
{
  const auto audioGameObjId = static_cast<AudioEngine::AudioGameObject>( gameObjId );
  _audioController->StopAllAudioEvents(audioGameObjId);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::ProcessEvents() const
{
  _audioController->ProcessAudioQueue();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::CreateAudioAnimation( Animation* anAnimation )
{
  // Check if there is a current animation, if so abort that animation and clean up correctly
  if ( _currentAnimation != nullptr ) {
    PRINT_NAMED_ERROR( "RobotAudioClient.CreateAudioAnimation",
                       "CurrentAnimation '%s' state: %s is NOT Null when creating a new animation",
                       _currentAnimation->GetAnimationName().c_str(),
                       RobotAudioAnimation::GetStringForAnimationState(_currentAnimation->GetAnimationState()).c_str() );
    _currentAnimation->AbortAnimation();
    ClearCurrentAnimation();
  }

  // Create appropriate animation type for mode
  RobotAudioAnimation* audioAnimation = nullptr;
  switch ( _outputSource ) {
  
    case RobotAudioOutputSource::PlayOnDevice:
    {
      audioAnimation = static_cast<RobotAudioAnimation*>( new RobotAudioAnimationOnDevice( anAnimation, this, &_robot->GetRNG() ) );
      break;
    }
      
    case RobotAudioOutputSource::PlayOnRobot:
    {
      audioAnimation = static_cast<RobotAudioAnimation*>( new RobotAudioAnimationOnRobot( anAnimation, this, &_robot->GetRNG() ) );
      break;
    }
      
    default:
      // Do Nothing
      break;
  }
  
  // Did not create animation
  if ( audioAnimation == nullptr ) {
    return;
  }
  
  // FIXME: This is a temp fix, will remove once we have an Audio Mixer
    audioAnimation->SetRobotVolume( _robotVolume );

  // Check if animation is valid
  const RobotAudioAnimation::AnimationState animationState = audioAnimation->GetAnimationState();
  if ( animationState != RobotAudioAnimation::AnimationState::AnimationCompleted &&
       animationState != RobotAudioAnimation::AnimationState::AnimationError ) {
    _currentAnimation = audioAnimation;
  }
  else {
    // Audio is not needed for this animation
    Util::SafeDelete( audioAnimation );
    _currentAnimation = nullptr;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::ClearCurrentAnimation()
{
  std::string animationState = "No Current Audio Animation";
  if ( _currentAnimation != nullptr ) {
    animationState = "Current Audio Animation '" + _currentAnimation->GetAnimationName() + "' State: " +
    _currentAnimation->GetStringForAnimationState( _currentAnimation->GetAnimationState() );
  }
  PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                "RobotAudioClient.ClearCurrentAnimation", "%s", animationState.c_str());
  Util::SafeDelete(_currentAnimation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::UpdateAnimationIsReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  // No Animation allow animation to proceed
  if ( !HasAnimation() ) {
    return true;
  }
  
  // Buffer is ready to get the next frame from
  if ( _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::AudioFramesReady ) {
    return true;
  }
  
  // Buffer is loading, however it's not yet time to play the next audio event
  if ( _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::LoadingStream ) {
    const TimeStamp_t relavantTime_ms = streamingTime_ms - startTime_ms;
    const TimeStamp_t nextEventTime_ms = _currentAnimation->GetNextEventTime_ms();
    // Check if the next event is in the future
    // This is also true when the result is kInvalidEventTime
    if ( relavantTime_ms < nextEventTime_ms ) {
      return true;
    }
  }
  
  // Animation is completed or has error, clear it and proceed
  if ( _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::AnimationCompleted ||
       _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::AnimationError ) {
    // Clear animation
    ClearCurrentAnimation();
    return true;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::AnimationIsComplete()
{
  // There is no animation
  if ( !HasAnimation() ) {
    return true;
  }
  // Animation state is completed or it has error
  else if ( _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::AnimationCompleted ||
            _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::AnimationError ) {
    return true;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::SetRobotVolume(float volume)
{
  // Keep On device robot volume (Wwise) in sync with robot volume
  PostParameter(GameParameter::ParameterType::Robot_Volume, volume, GameObjectType::Invalid);
  _robotVolume = volume;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float RobotAudioClient::GetRobotVolume() const
{
  return _robotVolume;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::SetOutputSource( RobotAudioOutputSource outputSource )
{
  ASSERT_NAMED( _audioController != nullptr, "RobotAudioClient.SetOutputSource.AudioControllerNull" );
  using namespace AudioEngine;
  
  if ( _outputSource == outputSource ) {
    // Do Nothing
    return;
  }
  _outputSource = outputSource;
  
  switch ( _outputSource ) {
    case RobotAudioOutputSource::None:
    case RobotAudioOutputSource::PlayOnDevice:
    {
      // Setup Audio engine to play audio through device
      // Remove GameObject Aux sends
      AudioController::AuxSendList emptyList;
      for ( auto& aKVP : _busConfigurationMap ) {
        const AudioGameObject aGameObject = static_cast<const AudioGameObject>( aKVP.second.gameObject );
        // Set Aux send settings in Audio Engine
        _audioController->SetGameObjectAuxSendValues( aGameObject, emptyList );
      }
    }
      break;
      
    case RobotAudioOutputSource::PlayOnRobot:
    {
      // Setup Audio engine to play audio through device
      // Setup GameObject Aux Sends
      for ( auto& aKVP : _busConfigurationMap ) {
        RobotBusConfiguration& busConfig = aKVP.second;
        const AudioGameObject aGameObject = static_cast<const AudioGameObject>( busConfig.gameObject );
        AudioAuxBusValue aBusValue( static_cast<AudioAuxBusId>( busConfig.bus ), 1.0f );
        AudioController::AuxSendList sendList;
        sendList.emplace_back( aBusValue );
        // Set Aux send settings in Audio Engine
        _audioController->SetGameObjectAuxSendValues( aGameObject, sendList );
        _audioController->SetGameObjectOutputBusVolume( aGameObject, 0.0f );
      }
    }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* RobotAudioClient::RegisterRobotAudioBuffer( GameObjectType gameObject,
                                                             PluginId_t pluginId,
                                                             Bus::BusType audioBus )
{
  ASSERT_NAMED( _audioController != nullptr, "RobotAudioClient.RegisterRobotAudioBuffer.AudioControllerNull" );
  
  // Create Configuration Struct
  RobotBusConfiguration busConfiguration = { gameObject, pluginId, audioBus };
  const auto it = _busConfigurationMap.emplace( gameObject, busConfiguration );
  if ( !it.second ) {
    // Bus configuration already exist
    PRINT_NAMED_ERROR("RobotAudioClient.RegisterRobotAudioBuffer", "Buss configuration already exist for GameObject: %d",
                      static_cast<uint32_t>(gameObject));
  }
  
  // Setup GameObject with Bus
  AudioEngine::AudioGameObject aGameObject = static_cast<const AudioEngine::AudioGameObject>( gameObject );
  
  // Create Buffer for buses
  return _audioController->RegisterRobotAudioBuffer( aGameObject, pluginId );
}


} // Audio
} // Cozmo
} // Anki
