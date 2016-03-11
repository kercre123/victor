/*
 * File: robotAudioAnimationOnRobot.cpp
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation On Robot is a sub-class of RobotAudioAnimation, it provides robot audio messages
 *              which are attached to Animation Frames. Each frame contains the synced audio data for that frame.
 *
 * Copyright: Anki, Inc. 2016
 */

#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>



namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::RobotAudioAnimationOnRobot( Animation* anAnimation, RobotAudioClient* audioClient )
{
  InitAnimation( anAnimation, audioClient );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::AnimationState RobotAudioAnimationOnRobot::Update( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  switch ( _state ) {
    case AnimationState::Preparing:
    {
      // Check if animation buffer is ready to start streaming audio
      if ( !_audioBuffer->IsWaitingForReset() ) {
        BeginBufferingAudioOnRobotMode();  // Set state to BufferLoading
      }
    }
      break;
      
    case AnimationState::BufferLoading:
    {
      UpdateBufferLoading( startTime_ms, streamingTime_ms );
    }
      break;
      
    case AnimationState::BufferReady:
    {
      UpdateBufferReady( startTime_ms, streamingTime_ms );
    }
      break;
      
    case AnimationState::AnimationAbort:
    {
      // If in animation mode wait for buffer to be ready before completing
      // If you hit this assert it is safe to comment out, please just let me know - Jordan R.
      ASSERT_NAMED( _state != RobotAudioAnimation::AnimationState::AnimationAbort, "Don't expect to get update calls after abort has been called.");
      _state = AnimationState::AnimationCompleted;
    }
      break;
      
    case AnimationState::AnimationCompleted:
      // Animation is done playing, do nothing
      break;
      
    case AnimationState::AnimationError:
    {
      // Should not end up in this state
      ASSERT_NAMED( _state != RobotAudioAnimation::AnimationState::AnimationError, "Should never fall into Error state!");
    }
      break;
      
    default:
      break;
  }
  
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    bool hasBuffer = false;
    size_t frameCount = 0;
    hasBuffer = _audioBuffer->HasAudioBufferStream();
    if ( hasBuffer ) {
      frameCount = _audioBuffer->GetFrontAudioBufferStream()->RobotAudioMessageCount();
    }
    
    PRINT_NAMED_INFO("RobotAudioAnimationOnRobot::Update", "EXIT State: %hhu - HasBuffer: %d - FrameCount: %zu - HasCurrentBuffer: %d", _state, hasBuffer, frameCount, HasCurrentBufferStream() );
  }
  
  return _state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateBufferLoading( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{  
  // Check if there is a buffer stream & it has messages
  if ( _audioBuffer->HasAudioBufferStream() && _audioBuffer->GetFrontAudioBufferStream()->HasRobotAudioMessage() ) {
    _state = AnimationState::BufferReady;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateBufferReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  // Check if current stream is ready for next update lap
  if ( HasCurrentBufferStream() ) {
    
    // Check if the current stream has frames left
    if ( !_currentBufferStream->HasRobotAudioMessage() ) {
      
      // No frames in buffer stream, check if buffer is completed
      if ( _currentBufferStream->IsComplete()) {
        // Buffer stream is completed
        _currentBufferStream = nullptr;
        _audioBuffer->PopAudioBufferStream();
        
        // Check if animation audio is completed
        if ( IsAnimationDone() ) {
          _state = AnimationState::AnimationCompleted;
        }
      }
      else {
        // Need to wait for more frames
        _state = AnimationState::BufferLoading;
      }
    }
    else {
      // Has frames, good to go!
      return;
    }
  }
  
  // If there isn't a current stream, check if it's time to send the next buffer or send silence messages
  if ( !HasCurrentBufferStream() ) {
    
    // Check if animation audio is done
    if ( IsAnimationDone() ) {
      _state = AnimationState::AnimationCompleted;
      return;
    }
    
    // Check if it's time for the next event
    const TimeStamp_t relevantTimeMS = streamingTime_ms - startTime_ms;
    while ( GetEventIndex() < _animationEvents.size() ) {

      const AnimationEvent* anEvent = &_animationEvents[GetEventIndex()];
      if ( anEvent->TimeInMS < relevantTimeMS ) {
       // Check if the event is valid
        if ( anEvent->State == AnimationEvent::AnimationEventState::Error ) {
          // Ignore invalid event, move on to next event
          IncrementEventIndex();
          continue;
        }
        
        // Check if there the buffer stream is ready
        if ( _audioBuffer->HasAudioBufferStream() && _audioBuffer->GetFrontAudioBufferStream()->HasRobotAudioMessage() ) {
          // Set current buffer stream
          _currentBufferStream = _audioBuffer->GetFrontAudioBufferStream();
          
          // Update the current event index
          IncrementEventIndex();
        }
        else {
          // Wait for buffer stream to get more frames
          _state = AnimationState::BufferLoading;
        }
        // Either we have set the current buffer or we are waiting for the buffer to load,
        // therefore break out of while loop
        break;
      }
      else {
        // It not time for the next event
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                                       TimeStamp_t startTime_ms,
                                                       TimeStamp_t streamingTime_ms )
{
  // Send silence frame by default
  out_RobotAudioMessagePtr = nullptr;
  const TimeStamp_t relevantTimeMS = streamingTime_ms - startTime_ms;
  
  // Get data from stream
  if ( HasCurrentBufferStream() ) {
    out_RobotAudioMessagePtr = _currentBufferStream->PopRobotAudioMessage();
    
    // Ignore any animation events that belong to this frame
    while ( GetEventIndex() < _animationEvents.size() ) {
      AnimationEvent* currentEvent = &_animationEvents[GetEventIndex()];
      if ( currentEvent->TimeInMS <= relevantTimeMS ) {
        IncrementEventIndex();
      }
      else {
        // Too early to play next Event
        break;
      }
    }
  }

  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    bool hasBuffer = _audioBuffer->HasAudioBufferStream();
    size_t frameCount = 0;
    if ( hasBuffer ) {
      
      frameCount = _audioBuffer->GetFrontAudioBufferStream()->RobotAudioMessageCount();
    }
    PRINT_NAMED_INFO("RobotAudioAnimationOnRobot::PopRobotAudioMessage", "EXIT PopMsg: %d - HasBuffer: %d - FrameCount: %zu - HasCurrentBuffer: %d", (out_RobotAudioMessagePtr != nullptr), hasBuffer, frameCount, HasCurrentBufferStream() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::PrepareAnimation()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimationOnRobot::PrepareAnimation", "Animation %s", _animationName.c_str());
  }
  
  // Check if buffer is ready
  if ( !_audioBuffer->IsWaitingForReset() ) {
    // Schedule the events
    BeginBufferingAudioOnRobotMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioAnimationOnRobot::IsAnimationDone() const
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimation::AllAnimationsPlayed", "eventCount: %lu  eventIdx: %d  completedCount: %d  hasAudioBufferStream: %d", _animationEvents.size(), GetEventIndex(), GetCompletedEventCount(), _audioBuffer->HasAudioBufferStream());
  }
  
  // Compare completed event count with number of events
  if ( GetCompletedEventCount() >= _animationEvents.size() && !_audioBuffer->HasAudioBufferStream() ) {
    return true;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::BeginBufferingAudioOnRobotMode()
{
  // Set plug-in bypass state - Disable bypass
  using namespace GameEvent;
  _audioClient->PostEvent( GenericEvent::Hijack_Audio_Disable_Bypass, GameObjectType::CozmoAnimation );
  
  _state = AnimationState::BufferLoading;
  
  const uint32_t firstAudioEventOffset = _animationEvents.front().TimeInMS;
  for ( auto& anEvent : _animationEvents ) {
    AnimationEvent* animationEvent = &anEvent;
    std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
    Util::Dispatch::After( _postEventTimerQueue,
                           std::chrono::milliseconds( anEvent.TimeInMS - firstAudioEventOffset ),
                           [this, animationEvent, isAliveWeakPtr] ()
                          {
                            // Trigger events if animation is still alive
                            if ( isAliveWeakPtr.expired() ) {
                              return;
                            }
                            
                            {
                              std::lock_guard<std::mutex> lock( _animationEventLock );
                              animationEvent->State = AnimationEvent::AnimationEventState::Posted;
                            }
                            
                            _audioClient->PostCozmoEvent( animationEvent->AudioEvent,
                                                          [this, animationEvent, isAliveWeakPtr] ( AudioCallback callback )
                                                           {
                                                             if ( !isAliveWeakPtr.expired() ) {
                                                               HandleCozmoEventCallback( animationEvent, callback );
                                                             }
                                                           } );
                            
                          },
                           "PostAudioEventToRobotDelay" );
  }
}

} // Audio
} // Cozmo
} // Anki
