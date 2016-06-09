/*
 * File: robotAudioAnimation.cpp
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


#include "anki/cozmo/basestation/audio/robotAudioAnimation.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/animation/animation.h"
#include "clad/audio/messageAudioClient.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {
namespace Audio {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimation::RobotAudioAnimation()
{
  // This base class is not intended to be used, use a sub-class
  SetAnimationState( AnimationState::AnimationError );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimation::~RobotAudioAnimation()
{
  if ( _postEventTimerQueue != nullptr ) {
    Util::Dispatch::Stop( _postEventTimerQueue );
    Util::Dispatch::Release( _postEventTimerQueue );
  }
  if ( _audioBuffer != nullptr ) {
    _audioBuffer->ClearBufferStreams();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimation::AbortAnimation()
{  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimation.AbortAnimation", "Animation: %s", _animationName.c_str());
  }
  
  // Always stop other events from posting to Audio Client
  if ( _postEventTimerQueue != nullptr ) {
    Util::Dispatch::Stop(_postEventTimerQueue );
  }
  
  // Notify buffer
  _audioBuffer->ResetAudioBuffer();
  
  // Stop all events play
  _audioClient->StopAllEvents( GameObjectType::CozmoAnimation );
  
  _audioBuffer->ClearBufferStreams();
  
  SetAnimationState( AnimationState::AnimationAbort );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& RobotAudioAnimation::GetStringForAnimationState( AnimationState state )
{
  static const std::vector<std::string> animationStateStrings = {
    "Preparing",
    "LoadingStream",
    "LoadingStreamFrames",
    "AudioFramesReady",
    "AnimationAbort",
    "AnimationCompleted",
    "AnimationError",
    "AnimationStateCount"
  };
  ASSERT_NAMED(animationStateStrings.size() == (size_t)AnimationState::AnimationStateCount + 1,
               "animationStateStrings.missingEnum");
  
  return animationStateStrings[ (size_t)state ];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t RobotAudioAnimation::GetNextEventTime_ms()
{
  const AnimationEvent* event = GetNextEvent();
  if ( event != nullptr ) {
    return event->TimeInMS;
  }
  
  return kInvalidEventTime;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimation::SetAnimationState( AnimationState state )
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimation.SetAnimationState", "State: %s", GetStringForAnimationState(state).c_str() );
  }
  _state = state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const RobotAudioAnimation::AnimationEvent* RobotAudioAnimation::GetNextEvent()
{
  if ( GetEventIndex() < _animationEvents.size() ) {
    return &_animationEvents[ GetEventIndex() ];
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimation::InitAnimation( Animation* anAnimation, RobotAudioClient* audioClient )
{
  // Load animation audio events
  _animationName = anAnimation->GetName();
 
  SetAnimationState( AnimationState::Preparing );
  
  // Loop through tracks
  // Prep animation audio events
  Animations::Track<RobotAudioKeyFrame>& audioTrack = anAnimation->GetTrack<RobotAudioKeyFrame>();
  
  AnimationEvent::AnimationEventId eventId = AnimationEvent::kInvalidAnimationEventId;
  while ( audioTrack.HasFramesLeft() ) {
    RobotAudioKeyFrame& aFrame = audioTrack.GetCurrentKeyFrame();
    
    const GameEvent::GenericEvent event = aFrame.GetAudioRef().audioEvent;
    if ( GameEvent::GenericEvent::Invalid != event ) {
      _animationEvents.emplace_back( ++eventId, aFrame.GetAudioRef().audioEvent, aFrame.GetTriggerTime() );
    }
    
    // Don't advance before we've had a chance to use aFrame, because it could be "live"
    audioTrack.MoveToNextKeyFrame();
  }
  
  if ( _animationEvents.empty() ) {
    SetAnimationState( AnimationState::AnimationCompleted );
    return;
  }
  
  _audioClient = audioClient;
  if ( _audioClient != nullptr ) {
    _audioBuffer = _audioClient->GetRobotAudiobuffer( GameObjectType::CozmoAnimation );
  }
  
  // Return error
  if ( _audioClient == nullptr || _audioBuffer == nullptr ) {
    SetAnimationState( AnimationState::AnimationError );
    PRINT_NAMED_ERROR("RobotAudioAnimation.InitAnimation", "Must set _audioClient and _audioBuffer pointers");
    return;
  }
  
  // Sort events by start time
  std::sort( _animationEvents.begin(),
             _animationEvents.end(),
             [] (const AnimationEvent& lhs, const AnimationEvent& rhs) { return lhs.TimeInMS < rhs.TimeInMS; } );
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation", "Audio Events Size: %lu - Enter", (unsigned long)_animationEvents.size());
    for (size_t idx = 0; idx < _animationEvents.size(); ++idx ) {
      PRINT_NAMED_INFO( "RobotAudioAnimation::LoadAnimation",
                        "Event Id: %d AudioEvent: %s TimeInMS: %d",
                        _animationEvents[idx].EventId,
                        EnumToString( _animationEvents[idx].AudioEvent ),
                        _animationEvents[idx].TimeInMS );
    }
    PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation", "Audio Events - Exit");
  }
  
  // Setup Dispatch
  _postEventTimerQueue = Util::Dispatch::Create( "BotAudioAnimation" );
  
  // Call sub-class specific code
  PrepareAnimation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimation::HandleCozmoEventCallback( AnimationEvent* animationEvent, AudioCallback& callback )
{
  // Mark events completed or error event callbacks
  switch ( callback.callbackInfo.GetTag() ) {
      
    case AudioCallbackInfoTag::callbackError:
    {
      PRINT_NAMED_INFO( "RobotAudioAnimation.HandleCozmoEventCallback", "ErrorType: %s",
                        EnumToString(callback.callbackInfo.Get_callbackError().callbackError) );
      IncrementCompletedEventCount();
      std::lock_guard<std::mutex> lock(_animationEventLock);
      animationEvent->State = AnimationEvent::AnimationEventState::Error;
    }
      break;
      
    case AudioCallbackInfoTag::callbackComplete:
    {
      IncrementCompletedEventCount();
      std::lock_guard<std::mutex> lock(_animationEventLock);
      animationEvent->State = AnimationEvent::AnimationEventState::Completed;
    }
      break;
      
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioAnimation::IsAnimationDone() const
{
  // Compare completed event count with number of events && there are no more audio streams
  const bool isDone = GetCompletedEventCount() >= _animationEvents.size() && !_audioBuffer->HasAudioBufferStream();
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO( "RobotAudioAnimation.IsAnimationDone",
                      "eventCount: %zu  eventIdx: %d  completedCount: %d  hasAudioBufferStream: %d | Result %s",
                      _animationEvents.size(),
                      GetEventIndex(),
                      GetCompletedEventCount(),
                      _audioBuffer->HasAudioBufferStream(),
                      isDone ? "T" : "F" );
  }
  
  return isDone;
}

} // Audio
} // Cozmo
} // Anki
