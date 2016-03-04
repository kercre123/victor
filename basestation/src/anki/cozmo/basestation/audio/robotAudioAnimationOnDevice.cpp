/*
 * File: robotAudioAnimationOnDevice.cpp
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation On Device is a sub-class of RobotAudioAnimation, it post Audio events when the
 *              corresponding frame is being buffered. This is not synced to the animation due to the reality we don’t
 *              when the robot is playing this frame, in the future we hope to fix this. It is intend to use when
 *              working with Webots as your simulated robot.
 *
 * Copyright: Anki, Inc. 2016
 */

#include "anki/cozmo/basestation/audio/robotAudioAnimationOnDevice.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/audio/robotAudioMessageStream.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "clad/audio/messageAudioClient.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


// Calculate how much delay to add to events while playing audio on device instead of on the robot
#define PLAY_ROBOT_AUDIO_ON_DIVICE_FRAME_COUNT_DELAY 21     // This is just a guess to get webots to sync with audio
static long long kPlayRobotOnDeviceDelayMS = static_cast<f32>(Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS) * PLAY_ROBOT_AUDIO_ON_DIVICE_FRAME_COUNT_DELAY;


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnDevice::RobotAudioAnimationOnDevice( Animation* anAnimation,
                                                          RobotAudioClient* audioClient )
{
  InitAnimation( anAnimation, audioClient );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnDevice::AnimationState RobotAudioAnimationOnDevice::Update( TimeStamp_t startTime_ms,
                                                                                TimeStamp_t streamingTime_ms )
{
  switch ( _state ) {
      
    case RobotAudioAnimation::AnimationState::BufferReady:
    {
      // Check if animations have all completed
      if ( IsAnimationDone() ) {
        _state = AnimationState::AnimationCompleted;
      }
    }
      break;
      
    case RobotAudioAnimation::AnimationState::AnimationAbort:
    {
      // If in animation mode wait for buffer to be ready before completing
      // If you hit this assert it is safe to comment out, please just let me know - Jordan R.
      ASSERT_NAMED( _state != RobotAudioAnimation::AnimationState::AnimationAbort, "Don't expect to get update calls after abort has been called.");
      
      _state = AnimationState::AnimationCompleted;
    }
      break;
      
    default:
      break;
  }

  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimationOnDevice::Update", "EXIT State - %hhu", _state);
  }

  return _state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnDevice::PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                                        TimeStamp_t startTime_ms,
                                                        TimeStamp_t streamingTime_ms )
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimationOnDevice::PopRobotAudioMessagePlayOnDeviceMode", "StartTime: %d - StreamingTime: %d - RelevantTime: %d - eventIdx: %d / %lu", startTime_ms, streamingTime_ms, (streamingTime_ms - startTime_ms), GetEventIndex(), _animationEvents.size() );
  }
  
  // When playing on device we always send Silence messages
  out_RobotAudioMessagePtr = nullptr;

  // Return waiting for posted events to complete
  if ( GetEventIndex() >= _animationEvents.size() ) {
    return;
  }
  
  // Audio events are played in context of animation streaming time
  AnimationEvent* currentEvent = &_animationEvents[GetEventIndex()];
  const TimeStamp_t relevantTimeMS = streamingTime_ms - startTime_ms;
  while ( (currentEvent->TimeInMS <= relevantTimeMS) && (GetEventIndex() < _animationEvents.size()) ) {
    
    AnimationEvent* animationEvent = currentEvent;
    std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
    Util::Dispatch::After( _postEventTimerQueue,
                           std::chrono::milliseconds( kPlayRobotOnDeviceDelayMS ),
                           [this, animationEvent, isAliveWeakPtr] ()
                          {
                            // Trigger events
                            if ( isAliveWeakPtr.expired() ) { return; }
                            
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
                           "PostAudioEventToDeviceDelay" );
    
    
    // Update event ( pop event )
    IncrementEventIndex();
    if ( GetEventIndex() < _animationEvents.size() ) {
      currentEvent = &_animationEvents[GetEventIndex()];
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnDevice::PrepareAnimation()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimationOnDevice::PrepareAnimation", "Animation %s", _animationName.c_str());
  }
  
  // Set plug-in bypass state - Enable Bypass
  using namespace GameEvent;
  _audioClient->PostEvent( GenericEvent::Hijack_Audio_Enable_Bypass, GameObjectType::CozmoAnimation );
  
  // Use audio controller to play sounds therefore we are ready to go
  _state = AnimationState::BufferReady;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioAnimationOnDevice::IsAnimationDone() const
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimation::AllAnimationsPlayed", "eventCount: %lu  eventIdx: %d  completedCount: %d  hasAudioBufferStream: %d", _animationEvents.size(), GetEventIndex(), GetCompletedEventCount(), _audioBuffer->HasAudioBufferStream());
  }
  
  // Compare completed event count with number of events
  if ( GetCompletedEventCount() >= _animationEvents.size() ) {
    return true;
  }
  
  return false;
}

} // Audio
} // Cozmo
} // Anki
