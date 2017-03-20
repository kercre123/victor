/*
 * File: robotAudioAnimation.h
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation Base class defines the core functionality for playing audio with animations. This
 *              class defines a single animation with one or more audio events. This is intended to be used as a base
 *              class, it will not perform audio animations. This runs in the animation run loop, first call update to
 *              determine the state of the animation for that frame. If the buffer is ready to perform the run loop,
 *              call PopRobotAudioMessage in the run loop to pull the audio frame message from the audio buffer.
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __Basestation_Audio_RobotAudioAnimation_H__
#define __Basestation_Audio_RobotAudioAnimation_H__

#include "anki/cozmo/basestation/animations/track.h"
#include "anki/cozmo/basestation/audio/audioEngineClient.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include <string>
#include <mutex>
#include <vector>


namespace Anki {

namespace AudioEngine {
struct AudioCallbackInfo;
}
  
namespace Util {
class RandomGenerator;
}
  
namespace Cozmo {

class Animation;

namespace RobotInterface {
class EngineToRobot;
}

namespace Audio {

class RobotAudioClient;
class RobotAudioBuffer;
class RobotAudioFrameStream;


class RobotAudioAnimation {
  
public:
  
  // Possible states of the animation
  enum class AnimationState : uint8_t {
    Preparing = 0,        // No Animation in progress
    LoadingStream,        // Animation is waiting for audio stream
    LoadingStreamFrames,  // Current Stream is waiting for audio frames
    AudioFramesReady,     // Has Animation audio frames are ready
    AnimationAbort,       // In progress of aborting animation
    AnimationCompleted,   // Animation is completed
    AnimationError,       // Error creating animation
    AnimationStateCount   // Number of Animation States
  };
  
  // Use subclass constructor, this class will return an invalid animation
  explicit RobotAudioAnimation( AudioMetaData ::GameObjectType gameObject, Util::RandomGenerator* randomGenerator );
  
  virtual ~RobotAudioAnimation();
  
  const std::string& GetAnimationName() const {return _animationName; };
  
  // Current animation state
  AnimationState GetAnimationState() const { return _state; }
  // Current animation state String
  static const std::string& GetStringForAnimationState( AnimationState state );
  
  // Call at the beginning of the update loop to update the animation's state for the upcoming loop cycle
  virtual AnimationState Update( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms ) = 0;
  
  // Pop the front EngineToRobot audio message
  // Will set out_RobotAudioMessagePtr to Null if there are no messages for provided current time. Use this to identify
  // when to send a AudioSilence message.
  // Note: EngineToRobot pointer memory needs to be manage or it will leak memory.
  virtual void PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                     TimeStamp_t startTime_ms,
                                     TimeStamp_t streamingTime_ms ) = 0;

  // Cancel all events and clear buffer
  void AbortAnimation();

  // Find out the next audio event's play time
  static constexpr uint32_t kInvalidEventTime = UINT32_MAX;
  uint32_t GetNextEventTime_ms();
  

protected:
  
  // Struct to sync audio buffer streams with animation
  struct AnimationEvent {
    using AnimationEventId = uint16_t;
    static constexpr AnimationEventId kInvalidAnimationEventId = 0;
    enum class AnimationEventState {
      None = 0,     // Not Set
      Posted,       // Posted audio event
      Completed,    // Completed event
      Error         // Event had play error
    };
    
    AnimationEventId eventId = kInvalidAnimationEventId;
    AudioMetaData::GameEvent::GenericEvent audioEvent = AudioMetaData::GameEvent::GenericEvent::Invalid;
    uint32_t time_ms = 0;
    float volume = 1.0f;
    AnimationEventState state = AnimationEventState::None;

    
    AnimationEvent( AnimationEventId eventId, AudioMetaData::GameEvent::GenericEvent audioEvent, uint32_t time_ms, float volume = 0.0f )
    : eventId( eventId )
    , audioEvent( audioEvent )
    , time_ms( time_ms )
    , volume( volume ) {}
  };

  // Set Animation State
  void SetAnimationState( AnimationState state );
  
  // Pointer to parent audio client and it's audio buffer
  RobotAudioClient* _audioClient = nullptr;
  RobotAudioBuffer* _audioBuffer = nullptr;
  
  // Animation Name for debugging
  std::string _animationName = "";
  
  // Dispatch event
  Util::Dispatch::Queue* _postEventTimerQueue = nullptr;
  // Provide weak pointers to help with callbacks
  std::shared_ptr<char> _isAliveSharedPtr = std::make_shared<char>();
  
  // Ordered list of animation audio event
  std::vector<AnimationEvent> _animationEvents;
  
  // Get next Audio Event
  // Return null if there are no more events
  const AnimationEvent* GetNextEvent();
  
  // Guard against event errors on different threads
  std::mutex _animationEventLock;
  
  // Track the current Event in the _animationEvents vector
  // Note: This is not thread safe, be sure to properly lock when using
  int GetEventIndex() const { return _eventIndex; }
  void IncrementEventIndex() { ++_eventIndex; }
  
  // Track how many events have been request to play
  // Note: This is not thread safe, be sure to properly lock when using
  int GetPostedEventCount() const { return _postedEventCount; }
  void IncrementPostedEventCount() { ++_postedEventCount; }
  
  // Track how many events have completed playback to track animation completion state
  int GetCompletedEventCount() const { return _completedEventCount; }
  void IncrementCompletedEventCount() { ++_completedEventCount; }
  
  // Hold on to the current stream frames are being pulled from
  RobotAudioFrameStream* _currentBufferStream = nullptr;
  bool HasCurrentBufferStream() const { return _currentBufferStream != nullptr; }

  // All the animations events have and completed
  virtual bool IsAnimationDone() const;
  
  // Call this from constructor in subclass to prepare for animation
  void InitAnimation( Animation* anAnimation, RobotAudioClient* audioClient );
  
  // Override this in subclass to perform specific preparation to animation
  virtual void PrepareAnimation() {};

  // Handle AudioClient's PostCozmo() callbacks from audio engine (Wwise)
  void HandleCozmoEventCallback( AnimationEvent* animationEvent, const AudioEngine::AudioCallbackInfo& callbackInfo );

  // Track what game obj to use for animation
  AudioMetaData::GameObjectType _gameObj = AudioMetaData::GameObjectType::Invalid;

  
private:

  Util::RandomGenerator* _randomGenerator = nullptr;
  
  // Current state of the animation
  AnimationState _state = AnimationState::Preparing;
  bool _hasAlts = false; // Audio Event has alternate audio playback
  
  // Track current event to sync with animation keyframes
  int _eventIndex = 0;
  // Track number of events that have been requested to play
  int _postedEventCount = 0;
  // Track number of events that have completed
  int _completedEventCount = 0;
  
};

  
  
} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioAnimation_H__ */
