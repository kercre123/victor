/*
 * File: robotAudioAnimationOnRobot.cpp
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation On Robot is a subclass of RobotAudioAnimation. It provides robot audio messages
 *              which are attached to Animation Frames. Each frame contains the synced audio data for that frame.
 *
 * Copyright: Anki, Inc. 2016
 */

// TEMP Include
#include "clad/robotInterface/messageEngineToRobot.h"

#include "engine/audio/robotAudioAnimationOnRobot.h"
#include "engine/audio/robotAudioClient.h"
#include "engine/audio/robotAudioBuffer.h"
#include "clad/audio/messageAudioClient.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/math/math.h"

#include <cmath>

namespace Anki {
namespace Cozmo {
namespace Audio {
  
// TEMP Solution to converting audio frame into robot audio message
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr float INT16_MAX_FLT = (float)std::numeric_limits<int16_t>::max();

uint8_t encodeMuLaw(float in_val)
{
  static const uint8_t MuLawCompressTable[] =
  {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
  };
  
  // protect against the occasional NaN sample
  if (std::isnan(in_val))
  {
    PRINT_NAMED_WARNING("RobotAudioAnimationOnRobot.encodeMuLaw.sampleNaN", "Audio sample from current stream is NaN");
    return 0;
  }
  
  // Convert float (-1.0, 1.0) to int16

  int16_t sample = Util::numeric_cast<int16_t>( Util::Clamp(in_val, -1.f, 1.f) * INT16_MAX_FLT );
  const bool sign = (sample < 0);
  
  if (sign) {
    sample = ~sample;
  }
  
  const uint8_t exponent = MuLawCompressTable[sample >> 8];
  uint8_t mantissa;
  
  if (exponent) {
    mantissa = (sample >> (exponent + 3)) & 0xF;
  } else {
    mantissa = (uint8_t)(sample >> 4);
  }
  
  return (uint8_t)((sign ? 0x80 : 0) | (exponent << 4) | mantissa);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::RobotAudioAnimationOnRobot( Animation* anAnimation,
                                                        RobotAudioClient* audioClient,
                                                        AudioMetaData::GameObjectType gameObject,
                                                        Util::RandomGenerator* randomGenerator )
: Anki::Cozmo::Audio::RobotAudioAnimation( gameObject, randomGenerator )
{
  InitAnimation( anAnimation, audioClient );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::AnimationState RobotAudioAnimationOnRobot::Update( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  switch ( GetAnimationState() ) {
    case AnimationState::Preparing:
    {
      // Check if animation buffer is ready to start streaming audio
      if ( !_audioBuffer->IsWaitingForReset() ) {
        BeginBufferingAudioOnRobotMode();  // Set state to LoadingStream
      }
      break;
    }
      
    case AnimationState::LoadingStream:
    case AnimationState::LoadingStreamFrames:
    {
      UpdateLoading( startTime_ms, streamingTime_ms );
      break;
    }
      
    case AnimationState::AudioFramesReady:
    {
      UpdateAudioFramesReady( startTime_ms, streamingTime_ms );
      break;
    }
      
    case AnimationState::AnimationCompleted:
    {
      // Animation is done playing, do nothing
      break;
    }
      
    case AnimationState::AnimationError:
    {
      // Should not end up in this state
      DEV_ASSERT(GetAnimationState() != RobotAudioAnimation::AnimationState::AnimationError,
                 "Should never fall into Error state!");
      break;
    }
      
    case AnimationState::AnimationStateCount:
    {
      // Should not end up in this state
      DEV_ASSERT(GetAnimationState() != RobotAudioAnimation::AnimationState::AnimationStateCount,
                 "Should never fall into AnimationStateCount state!");
      break;
    }
  }
  
  return GetAnimationState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateLoading( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  if (!_audioBuffer->HasAudioBufferStream())
  {
    // Check if animation audio is completed
    if ( IsAnimationDone() ) {
      SetAnimationState( AnimationState::AnimationCompleted );
    }
    // no stream yet so we wait
    return;
  }
  
  if (_audioBuffer->AudioStreamHasData())
  {
    const TimeStamp_t relevantTime_ms = streamingTime_ms - startTime_ms;
    const AnimationEvent* nextEvent = GetNextEvent();
    const bool nextEventIsReady = (nextEvent != nullptr) && (nextEvent->time_ms <= relevantTime_ms);
    
    if (!_didPlayFirstStream)
    {
      if (nextEventIsReady)
      {
        // Setup initial condition for the first event
        _didPlayFirstStream = true;
        // Calculate the time when we started playing this animation on the audio streaming timeline. This means we
        // start time when delivery of the first chunk of audio on the first event occurred, then count backwards assuming
        // it was delivered exactly when it was supposed to be consumed/played
        _streamAnimationOffsetTime_ms = _audioBuffer->GetAudioStreamCreatedTime_ms() - nextEvent->time_ms;
        SetAnimationState( AnimationState::AudioFramesReady );
      }
    }
    else
    {
      const uint32_t streamRelevantTime_ms = floor( _audioBuffer->GetAudioStreamCreatedTime_ms() - _streamAnimationOffsetTime_ms );
      if (streamRelevantTime_ms <= relevantTime_ms || nextEventIsReady)
      {
        SetAnimationState( AnimationState::AudioFramesReady );
      }
    }
  }
  else if (_audioBuffer->AudioStreamIsComplete())
  {
    _audioBuffer->PopAudioBufferStream();
    SetAnimationState(RobotAudioAnimation::AnimationState::LoadingStream);
  }
  // otherwise we wait for the stream to get data or be marked complete
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateAudioFramesReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  DEV_ASSERT(_audioBuffer->HasAudioBufferStream(),
             "RobotAudioAnimationOnRobot.UpdateAudioFramesReady.UpdatingWhenStreamIsGone");
  if (!_audioBuffer->AudioStreamHasData())
  {
    SetAnimationState(AnimationState::LoadingStreamFrames);
    return;
  }
  
  // Loop thorugh events to remove invalid events and get next valid event
  const AnimationEvent* nextEvent = GetNextEvent();
  while ( nextEvent != nullptr ) {
    if ( nextEvent->state != AnimationEvent::AnimationEventState::Error ) {
      // Valid event
      break;
    }
    // Else
    // Ignore invalid event, move on to next event
    IncrementEventIndex();
    nextEvent = GetNextEvent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                                       TimeStamp_t startTime_ms,
                                                       TimeStamp_t streamingTime_ms )
{
  // Send silence frame by default
  out_RobotAudioMessagePtr = nullptr;
  const TimeStamp_t relevantTime_ms = streamingTime_ms - startTime_ms;
  
  
  // Get data from stream
  if (GetAnimationState() == AnimationState::AudioFramesReady) {
    
    const AudioEngine::AudioFrameData* audioFrame = _audioBuffer->PopNextAudioFrameData();
    if (nullptr != audioFrame)
    {
      // Convert audio frame into correct robot keyframe type
      AnimKeyFrame::AudioSample keyFrame;
      DEV_ASSERT(audioFrame->samples.size() <= keyFrame.sample.size(),
                 "Block size must be less or equal to audioSample size");
      // Convert audio format to robot format
      for ( size_t idx = 0; idx < audioFrame->samples.size(); ++idx ) {
        keyFrame.sample[idx] = encodeMuLaw( audioFrame->samples[idx] );
      }
      
      // Pad the back of the keyframe with 0s
      // This should only apply to the last frame
      if (audioFrame->samples.size() < keyFrame.sample.size()) {
        std::fill( keyFrame.sample.begin() + audioFrame->samples.size(), keyFrame.sample.end(), 0 );
      }
      
      // After adding audio data to robot audio keyframe delete frame
      Util::SafeDelete( audioFrame );
      // Create Audio message
      RobotInterface::EngineToRobot* audioMsg = new RobotInterface::EngineToRobot( std::move( keyFrame ) );
      out_RobotAudioMessagePtr = audioMsg;
      
      // Ignore any animation events that belong to this frame
      while ( GetEventIndex() < _animationEvents.size() ) {
        AnimationEvent* currentEvent = &_animationEvents[GetEventIndex()];
        if ( currentEvent->time_ms < relevantTime_ms ) {
          IncrementEventIndex();
        }
        else {
          // Too early to play next Event
          break;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::RobotAudioAnimationOnRobot()
: Anki::Cozmo::Audio::RobotAudioAnimation( AudioMetaData::GameObjectType::Invalid, nullptr )
{ }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::PrepareAnimation()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                  "RobotAudioAnimationOnRobot.PrepareAnimation", "Animation %s", _animationName.c_str());
  }
  
  // Check if buffer is ready
  if ( !_audioBuffer->IsWaitingForReset() ) {
    // Schedule the events
    BeginBufferingAudioOnRobotMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::BeginBufferingAudioOnRobotMode()
{
  if ( _animationEvents.empty() ) {
    SetAnimationState( Anki::Cozmo::Audio::RobotAudioAnimation::AnimationState::AnimationCompleted );
    PRINT_NAMED_WARNING( "RobotAudioAnimationOnRobot.BeginBufferingAudioOnRobotMode", "_animationEvents.IsEmpty" );
    return;
  }
  
  // Begin Loading robot audio buffer by posting animation audio events
  SetAnimationState( AnimationState::LoadingStream );
  
  const uint32_t firstAudioEventOffset = _animationEvents.front().time_ms;
  for ( auto& anEvent : _animationEvents ) {
    AnimationEvent* animationEvent = &anEvent;
    std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
    Util::Dispatch::After( _postEventTimerQueue,
                           std::chrono::milliseconds( anEvent.time_ms - firstAudioEventOffset ),
                           [this, animationEvent, isAliveWeakPtr = std::move(isAliveWeakPtr)] ()
      {
        // Trigger events if animation is still alive
        if ( isAliveWeakPtr.expired() ) {
          return;
        }
        
        // Post Event
        using namespace AudioEngine;
        using PlayId = RobotAudioClient::CozmoPlayId;
        const RobotAudioClient::CozmoEventCallbackFunc callbackFunc = [this, animationEvent, isAliveWeakPtr = std::move(isAliveWeakPtr)]
        ( const AudioEngine::AudioCallbackInfo& callbackInfo )
        {
          if ( !isAliveWeakPtr.expired() ) {
            std::lock_guard<std::mutex> lock(_animationEventLock);
            HandleCozmoEventCallback( animationEvent, callbackInfo );
          }
        };
        
        PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                      "RobotAudioAnimationOnRobot.PostEvent",
                      "Anim: '%s' EventId: %u TimeMs: %u",
                      GetAnimationName().c_str(), animationEvent->audioEvent, animationEvent->time_ms);
        
        // Ready to post event update state
        {
          std::lock_guard<std::mutex> lock( _animationEventLock );
          animationEvent->state = AnimationEvent::AnimationEventState::Posted;
          IncrementPostedEventCount();
        }
        
        const PlayId playId = _audioClient->PostCozmoEvent( animationEvent->audioEvent,
                                                            _gameObj,
                                                            std::move(callbackFunc) );
        // Set event's volume RTPC
        if (RobotAudioClient::kInvalidCozmoPlayId != playId) {
          _audioClient->SetCozmoEventParameter( playId,
                                                AudioMetaData::GameParameter::ParameterType::Event_Volume,
                                                animationEvent->volume );
        }
        
        // Processes event NOW, minimize buffering latency
        _audioClient->ProcessEvents();
      },
                           "PostAudioEventToRobotDelay" );
  }
}

} // Audio
} // Cozmo
} // Anki
