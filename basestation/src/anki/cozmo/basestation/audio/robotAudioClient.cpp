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
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnDevice.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
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
void RobotAudioClient::CreateAudioAnimation( Animation* anAnimation, AnimationMode mode )
{
  // Create appropriate animation type for mode
  RobotAudioAnimation* audioAnimation = nullptr;
  switch ( mode ) {
  
    case AnimationMode::PlayOnDevice:
      audioAnimation = static_cast<RobotAudioAnimation*>( new RobotAudioAnimationOnDevice( anAnimation, this ) );
      break;
      
    case AnimationMode::PlayOnRobot:
      audioAnimation = static_cast<RobotAudioAnimation*>( new RobotAudioAnimationOnRobot( anAnimation, this ) );
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
