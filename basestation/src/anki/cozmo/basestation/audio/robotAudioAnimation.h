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
#include <util/dispatchQueue/dispatchQueue.h>
#include <vector>
#include <mutex>



namespace Anki {
namespace Cozmo {

class Animation;

namespace RobotInterface {
class EngineToRobot;
}

namespace Audio {

class RobotAudioClient;
class RobotAudioBuffer;
class RobotAudioMessageStream;



class RobotAudioAnimation {
  
public:
  
  // Possible states of the animation
  enum class AnimationState : uint8_t {
    Preparing = 0,        // No Animation in progress
    BufferLoading,        // Has Animation waiting for buffer frames
    BufferReady,          // Has Animation buffer frames are ready
    AnimationAbort,       // In progress of aborting animation
    AnimationCompleted,   // Animation is completed
    AnimationError        // Error creating animation
  };
  
  // Use sub-class constructor, this class will return an invalid animation
  RobotAudioAnimation();
  
  virtual ~RobotAudioAnimation();
  
  // Cancel all events and clear buffer
  void AbortAnimation();
  
  // Current animation state
  AnimationState GetAnimationState() const { return _state; }
  
  // Call at the beginning of the update loop to update the animation's state for the upcoming loop cycle
  virtual AnimationState Update( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms ) = 0;
  
  // Pop the front EngineToRobot audio message
  // Will set out_RobotAudioMessagePtr to Null if there are no messages for provided current time. Use this to identify
  // when to send a AudioSilence message.
  // Note: EngineToRobot pointer memory needs to be manage or it will leak memory.
  virtual void PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                            TimeStamp_t startTime_ms,
                            TimeStamp_t streamingTime_ms ) = 0;


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
    
    AnimationEventId EventId = kInvalidAnimationEventId;
    GameEvent::GenericEvent AudioEvent = GameEvent::GenericEvent::Invalid;
    uint32_t TimeInMS = 0;
    AnimationEventState State = AnimationEventState::None;
    
    AnimationEvent( AnimationEventId eventId, GameEvent::GenericEvent audioEvent, uint32_t timeInMS ) :
    EventId( eventId ),
    AudioEvent( audioEvent ),
    TimeInMS( timeInMS ) {}
  };
  
  // Current state of the animation
  AnimationState _state = AnimationState::Preparing;
  
  // Pointer to parent audio client and it's audio buffer
  RobotAudioClient* _audioClient = nullptr;
  RobotAudioBuffer* _audioBuffer = nullptr;
  
  // Animation Name for debugging
  std::string _animationName = "";
  
  // Dispatch event
  Util::Dispatch::Queue* _postEventTimerQueue = nullptr;
  // Provide week pointers to help with callbacks
  std::shared_ptr<char> _isAliveSharedPtr = std::make_shared<char>();
  
  // Ordered list of animation audio event
  std::vector<AnimationEvent> _animationEvents;
  
  // Track the current Event in the _animationEvents vector
  int GetEventIndex() const { return _eventIndex; }
  void IncrementEventIndex() { ++_eventIndex; }
  // Guard against event errors on different threads
  std::mutex _animationEventLock;
  
  // Track how many events have completed playback to track animation completion state
  int GetCompletedEventCount() const { return _completedEventCount; }
  void IncrementCompletedEventCount() { std::lock_guard<std::mutex> lock(_completedEventLock);  ++_completedEventCount; }
  
  // Hold on to the current stream frames are being pulled from
  RobotAudioMessageStream* _currentBufferStream = nullptr;
  bool HasCurrentBufferStream() const { return _currentBufferStream != nullptr; }

  // All the animations events have and completed
  virtual bool IsAnimationDone() const = 0;
  
  // Call this from constructor in sub-class to prepare for animation
  void InitAnimation( Animation* anAnimation, RobotAudioClient* audioClient );
  
  // Override this in sub-class to perform specific preparation to animation
  virtual void PrepareAnimation() {};

  // Handle AudioClient's PostCozmo() callbacks from audio engine (Wwise)s
  void HandleCozmoEventCallback( AnimationEvent* animationEvent, AudioCallback& callback );

private:

  // Track current event
  int _eventIndex = 0;
  // Track number of events that have completed
  int _completedEventCount = 0;
  // Completed Event Count is updated on a different thread
  std::mutex _completedEventLock;
  
};

  
  
} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioAnimation_H__ */
