/*
 * File: robotAudioClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioClient_H__
#define __Basestation_Audio_RobotAudioClient_H__

#include "anki/cozmo/basestation/audio/audioEngineClient.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimation.h"


namespace Anki {
namespace Cozmo {

class Animation;
  
namespace Audio {
  
class AudioController;
  
class RobotAudioBuffer;
  
class RobotAudioClient : public AudioEngineClient
{
public:
  
  // Animation audio modes
  enum class AnimationMode : uint8_t {
    None,           // No audio
    PlayOnDevice,   // Play on Device - This is not perfectly synced to animations
    PlayOnRobot     // Play on Robot by using Hijack Audio plug-in to get audio stream from Wwise
  };
  
  // Set Audio Controller to provide access to robot audio buffers
  void SetAudioController( AudioController* audioController ) { _audioController = audioController; }
  
  // Get an available Audio Buffer from pool
  RobotAudioBuffer* GetAvailableAudioBuffer();
  
  // Post Cozmo specific Audio events
  CallbackIdType PostCozmoEvent( GameEvent::GenericEvent event, AudioEngineClient::CallbackFunc callback = nullptr );
  
  // Value between ( 0.0 - 1.0 )
  void SetRobotVolume(float volume);

  // Create an Audio Animation for a specific animation. Only one animation can be played at a time
  void CreateAudioAnimation( Animation* anAnimation, const AnimationMode mode );

  RobotAudioAnimation* GetCurrentAnimation() { return _currentAnimation; }
  
  // Delete audio animation
  // Note: This Does not Abort the animation
  void ClearCurrentAnimation();

  bool HasAnimation() const { return _currentAnimation != nullptr; }

  // Return true if there is no animation or animation is ready
  bool UpdateAnimationIsReady();
  
  // Check Animation States to see if it's completed
  bool AnimationIsComplete();
  
private:
  
  // Provides robot audio buffer
  AudioController* _audioController = nullptr;
  
  // Audio Animation Object to provide audio frames to Animation
  RobotAudioAnimation* _currentAnimation = nullptr;
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
