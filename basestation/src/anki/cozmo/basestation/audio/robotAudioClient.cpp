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
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnDevice.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::SetupRobotAudio()
{
  // Configure Robot Audio buffers with Wwise buses. PlugIn Ids are set in Wwise project
  // Setup Robot Buffers
  RegisterRobotAudioBuffer( GameObjectType::CozmoAnimation, 1, Bus::BusType::Robot_Bus_1 );
  
  // TEMP: Setup other buses
  RegisterRobotAudioBuffer( GameObjectType::CozmoBus_2, 2, Bus::BusType::Robot_Bus_2 );
  RegisterRobotAudioBuffer( GameObjectType::CozmoBus_3, 3, Bus::BusType::Robot_Bus_3 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* RobotAudioClient::RegisterRobotAudioBuffer( GameObjectType gameObject,
                                                              PluginId_t pluginId,
                                                              Bus::BusType audioBus )
{
  ASSERT_NAMED( _audioController != nullptr, "RobotAudioClient.RegisterRobotAudioBuffer.AudioControllerNull" );
  
  // Setup GameObject with Bus
  using namespace AudioEngine;
  AudioEngine::AudioGameObject gameObj = static_cast<const AudioEngine::AudioGameObject>( gameObject );
  std::vector<AudioEngine::AudioAuxBusValue> busValues;
  AudioAuxBusValue aBusValue( static_cast<AudioAuxBusId>( audioBus ), 1.0f );
  busValues.emplace_back( aBusValue );
  _audioController->SetGameObjectAuxSendValues( gameObj, busValues );
  
  // Create Buffer for bus
  return _audioController->RegisterRobotAudioBuffer( gameObj, pluginId );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* RobotAudioClient::GetRobotAudiobuffer( GameObjectType gameObject )
{
  ASSERT_NAMED( _audioController != nullptr, "RobotAudioClient.GetRobotAudiobuffer.AudioControllerNull" );
  const AudioEngine::AudioGameObject gameObj = static_cast<const AudioEngine::AudioGameObject>( gameObject );
  return _audioController->GetRobotAudioBufferWithGameObject( gameObj );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::CallbackIdType RobotAudioClient::PostCozmoEvent( GameEvent::GenericEvent event, AudioEngineClient::CallbackFunc callback )
{
  const CallbackIdType callbackId = PostEvent( event, GameObjectType::CozmoAnimation, callback );
  
  return callbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::SetRobotVolume(float volume)
{
  PostParameter(GameParameter::ParameterType::Robot_Volume, volume, GameObjectType::Invalid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::CreateAudioAnimation( Animation* anAnimation, const AnimationMode mode )
{
  // Create appropriate animation type for mode
  RobotAudioAnimation* audioAnimation = nullptr;
  switch ( mode ) {
  
    case AnimationMode::PlayOnDevice:
      audioAnimation = dynamic_cast<RobotAudioAnimation*>( new RobotAudioAnimationOnDevice( anAnimation, this ) );
      break;
      
    case AnimationMode::PlayOnRobot:
      audioAnimation = dynamic_cast<RobotAudioAnimation*>( new RobotAudioAnimationOnRobot( anAnimation, this ) );
      break;
      
    default:
      break;
  }
  
  // Did not create animation
  if ( audioAnimation == nullptr ) {
    return;
  }
  
  // Check if animation is valid
  const RobotAudioAnimation::AnimationState animationState = audioAnimation->GetAnimationState();
  if ( animationState != RobotAudioAnimation::AnimationState::AnimationCompleted &&
      animationState != RobotAudioAnimation::AnimationState::AnimationError) {
    
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
  Util::SafeDelete(_currentAnimation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::UpdateAnimationIsReady()
{
  // No Animation allow animation to proceed
  if ( !HasAnimation() ) {
    return true;
  }
  
  // Buffer is ready to get the next frame from
  if ( _currentAnimation->GetAnimationState() == RobotAudioAnimation::AnimationState::BufferReady ) {
    return true;
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


} // Audio
} // Cozmo
} // Anki
