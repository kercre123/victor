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
#include "clad/audio/audioCallbackMessage.h"
#include "clad/audio/messageAudioClient.h"
#include "DriveAudioEngine/audioCallback.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"


#define DEBUG_ROBOT_AUDIO_ANIMATION_OVERRIDE 1


namespace Anki {
namespace Cozmo {
namespace Audio {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimation::RobotAudioAnimation( Util::RandomGenerator* randomGenerator )
: _randomGenerator( randomGenerator )
{
  // This base class is not intended to be used, use a sub-class
  SetAnimationState( AnimationState::AnimationError );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimation::~RobotAudioAnimation()
{
  _randomGenerator = nullptr;
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
  if ( DEBUG_ROBOT_ANIMATION_AUDIO || DEBUG_ROBOT_AUDIO_ANIMATION_OVERRIDE ) {
    PRINT_NAMED_INFO("RobotAudioAnimation.AbortAnimation", "Animation: %s", _animationName.c_str());
  }
  
  // Always stop other events from posting to Audio Client
  if ( _postEventTimerQueue != nullptr ) {
    Util::Dispatch::Stop(_postEventTimerQueue );
  }
  
  // Stop all animation audio events being played
  _audioClient->StopCozmoEvent( GameObjectType::CozmoAnimation );
  
  // Notify buffer
  _audioBuffer->ResetAudioBuffer();
    
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
    return event->time_ms;
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
    const RobotAudioKeyFrame& aFrame = audioTrack.GetCurrentKeyFrame();
    const RobotAudioKeyFrame::AudioRef& audioRef = aFrame.GetAudioRef();
    const GameEvent::GenericEvent event = audioRef.audioEvent;
    if ( GameEvent::GenericEvent::Invalid != event ) {
      
      // Apply random weight
      bool playEvent = Util::IsFltNear( audioRef.probability, 1.0f );
      if ( _randomGenerator != nullptr && !playEvent ) {
        playEvent = audioRef.probability >= _randomGenerator->RandDbl( 1.0 );
      }
      else {
        // No random generator
        playEvent = true;
      }
      
      if ( playEvent ) {
        // Add Event to queue
        _animationEvents.emplace_back( ++eventId, event, aFrame.GetTriggerTime(), audioRef.volume );
        // Check if buffer should only play once
        if ( !_hasAlts && audioRef.audioAlts ) {
          // Don't replay buffer
          _hasAlts = true;
        }
      }
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
             [] (const AnimationEvent& lhs, const AnimationEvent& rhs) { return lhs.time_ms < rhs.time_ms; } );
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation", "Audio Events Size: %lu - Enter", (unsigned long)_animationEvents.size());
    for (size_t idx = 0; idx < _animationEvents.size(); ++idx ) {
      PRINT_NAMED_INFO( "RobotAudioAnimation::LoadAnimation",
                        "Event Id: %d AudioEvent: %s TimeInMS: %d",
                        _animationEvents[idx].eventId,
                        EnumToString( _animationEvents[idx].audioEvent ),
                        _animationEvents[idx].time_ms );
    }
    PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation", "Audio Events - Exit");
  }
  
  // Setup Dispatch
  _postEventTimerQueue = Util::Dispatch::Create( "BotAudioAnimation" );
  
  // Call sub-class specific code
  PrepareAnimation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimation::HandleCozmoEventCallback( AnimationEvent* animationEvent, const AudioEngine::AudioCallbackInfo& callbackInfo )
{
  // Mark events completed or error event callbacks
  switch ( callbackInfo.callbackType ) {
      
    case AudioEngine::AudioCallbackType::Error:
    {
      PRINT_NAMED_INFO( "RobotAudioAnimation.HandleCozmoEventCallback", "Error: %s",
                        callbackInfo.GetDescription().c_str() );
      IncrementCompletedEventCount();
      std::lock_guard<std::mutex> lock(_animationEventLock);
      animationEvent->state = AnimationEvent::AnimationEventState::Error;
    }
      break;
      
    case AudioEngine::AudioCallbackType::Complete:
    {
      IncrementCompletedEventCount();
      std::lock_guard<std::mutex> lock(_animationEventLock);
      animationEvent->state = AnimationEvent::AnimationEventState::Completed;
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
