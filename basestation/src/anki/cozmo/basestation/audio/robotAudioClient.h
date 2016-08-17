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
#include "util/helpers/templateHelpers.h"
#include <unordered_map>
#include <queue>


namespace Anki {
  
namespace Util {
class RandomGenerator;
namespace Dispatch {
class Queue;
}
}
namespace Cozmo {
class Robot;
class Animation;
  
namespace Audio {
class AudioController;
class RobotAudioBuffer;
  
class RobotAudioClient : public AudioEngineClient {

public:

  // !!! Be sure to update RobotAudioOutputSourceCLAD in messageGameToEngine.clad if this is changed !!!
  // Animation audio modes
  enum class RobotAudioOutputSource : uint8_t {
    None,           // No audio
    PlayOnDevice,   // Play on Device - This is not perfectly synced to animations
    PlayOnRobot     // Play on Robot by using Hijack Audio plug-in to get audio stream from Wwise
  };
  
  static const char* kRobotAudioLogChannelName;

  // Default Constructor
  RobotAudioClient( Robot* robot );

  // Destructor
  ~RobotAudioClient();
    
  // The the audio buffer for the corresponding Game Object
  virtual RobotAudioBuffer* GetRobotAudiobuffer( GameObjectType gameObject );

  // Post Cozmo specific Audio events
  using CozmoPlayId = uint32_t;
  using CozmoEventCallbackFunc = std::function<void( const AudioEngine::AudioCallbackInfo& callbackInfo )>;
  CozmoPlayId PostCozmoEvent( GameEvent::GenericEvent event,
                              GameObjectType GameObjId = GameObjectType::Invalid,
                              const CozmoEventCallbackFunc& callbackFunc = nullptr ) const;
  
  bool SetCozmoEventParameter( CozmoPlayId playId, GameParameter::ParameterType parameter, float value ) const;
  
  void StopCozmoEvent(GameObjectType gameObjId);
  
  void ProcessEvents() const;

  
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// vvvvvvvvvvvv Depercated vvvvvvvvvvvvvvvv
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
   // Create an Audio Animation for a specific animation. Only one animation can be played at a time
  // FIXME: Remove after new animtion code goes in
  void CreateAudioAnimation( Animation* anAnimation );

  // FIXME: Remove after new animtion code goes in
  RobotAudioAnimation* GetCurrentAnimation() { return _currentAnimation; }

  // Delete audio animation
  // Note: This Does not Abort the animation
  // FIXME: Remove after new animtion code goes in
  void ClearCurrentAnimation();

  // FIXME: Remove after new animtion code goes in
  bool HasAnimation() const { return _currentAnimation != nullptr; }

  // Return true if there is no animation or animation is ready
  // FIXME: Remove after new animtion code goes in
  bool UpdateAnimationIsReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );

  // Check Animation States to see if it's completed
  // FIXME: Remove after new animtion code goes in
  bool AnimationIsComplete();

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ^^^^^^^^^^^^ Depercated ^^^^^^^^^^^^^
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Robot Volume Value is between ( 0.0 - 1.0 )
  void SetRobotVolume(float volume);
  float GetRobotVolume() const;

  // Must be called after RegisterRobotAudioBuffer() to properly setup robot audio signal flow
  void SetOutputSource( RobotAudioOutputSource outputSource );
  RobotAudioOutputSource GetOutputSource() const { return _outputSource; }
  
  
  bool AvailableGameObjectAndAudioBufferInPool() const { return !_robotBufferGameObjectPool.empty(); }
  
  // Set gameObj & audio buffer out_vars for current output source
  // Return false if buffer is not available
  // Remove gameObj/buffer from pool
  bool GetGameObjectAndAudioBufferFromPool(GameObjectType& out_gameObj, RobotAudioBuffer*& out_buffer);

  // Add gameObj/buffer back into pool
  void ReturnGameObjectToPool(GameObjectType gameObject);
  
  Util::Dispatch::Queue* GetAudioQueue() const { return _dispatchQueue; }

  // Get shared random generator
  Util::RandomGenerator& GetRandomGenerator() const;

private:
  
  using PluginId_t = uint32_t;
  struct RobotBusConfiguration {
    GameObjectType  gameObject;
    PluginId_t      pluginId;
    Bus::BusType    bus;
  };
  
  // Handle to parent Robot
  Robot* _robot = nullptr;
  
  // Provides robot audio buffer
  AudioController* _audioController = nullptr;
  
  // Animation Audio Event queue
  Util::Dispatch::Queue* _dispatchQueue = nullptr;
  
  // Audio Animation Object to provide audio frames to Animation
  RobotAudioAnimation* _currentAnimation = nullptr;
  
  // Current Output source
  RobotAudioOutputSource _outputSource = RobotAudioOutputSource::None;
  
  // Store Bus configurations
  std::unordered_map<GameObjectType, RobotBusConfiguration, Util::EnumHasher> _busConfigurationMap;
  
  // Keep track of available Game Objects with Audio Buffers
  std::queue<GameObjectType> _robotBufferGameObjectPool;
  
  // Create Audio Buffer for the corresponding Game Object
  RobotAudioBuffer* RegisterRobotAudioBuffer( GameObjectType gameObject, PluginId_t pluginId, Bus::BusType bus );
  void UnregisterRobotAudioBuffer( GameObjectType gameObject, PluginId_t pluginId, Bus::BusType bus );
  
  // Keep current robot volume
  float _robotVolume = 0.0f;
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
