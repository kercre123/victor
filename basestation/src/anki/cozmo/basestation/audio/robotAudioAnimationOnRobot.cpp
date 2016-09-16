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

// TEMP Include
#include "clad/robotInterface/messageEngineToRobot.h"

#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "clad/audio/messageAudioClient.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/math/math.h"


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
  
  // Convert float (-1.0, 1.0) to int16

  int16_t sample = Util::numeric_cast<int16_t>( Util::Clamp(in_val, -1.f, 1.f) * INT16_MAX_FLT );
  
  bool sign = sample < 0;
  
  if (sign)	{
    sample = ~sample;
  }
  
  uint8_t exponent = MuLawCompressTable[sample >> 8];
  uint8_t mantessa;
  
  if (exponent) {
    mantessa = (sample >> (exponent + 3)) & 0xF;
  } else {
    mantessa = sample >> 4;
  }
  
  return (sign ? 0x80 : 0) | (exponent << 4) | mantessa;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::RobotAudioAnimationOnRobot( Animation* anAnimation,
                                                        RobotAudioClient* audioClient,
                                                        GameObjectType gameObject,
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
    }
      break;
      
    case AnimationState::LoadingStream:
    {
      UpdateLoadingStream( startTime_ms, streamingTime_ms );
    }
      break;
      
    case AnimationState::LoadingStreamFrames:
    {
      UpdateLoadingStreamFrames( startTime_ms, streamingTime_ms );
    }
      break;
      
    case AnimationState::AudioFramesReady:
    {
      UpdateAudioFramesReady( startTime_ms, streamingTime_ms );
    }
      break;
      
    case AnimationState::AnimationAbort:
    {
      // If in animation mode wait for buffer to be ready before completing
      // If you hit this assert it is safe to comment out, please just let me know - Jordan R.
      ASSERT_NAMED( GetAnimationState() != RobotAudioAnimation::AnimationState::AnimationAbort,
                    "Don't expect to get update calls after abort has been called.");
      SetAnimationState( AnimationState::AnimationCompleted );
    }
      break;
      
    case AnimationState::AnimationCompleted:
      // Animation is done playing, do nothing
      break;
      
    case AnimationState::AnimationError:
    {
      // Should not end up in this state
      ASSERT_NAMED( GetAnimationState() != RobotAudioAnimation::AnimationState::AnimationError,
                    "Should never fall into Error state!" );
    }
      break;
      
    case AnimationState::AnimationStateCount:
    {
      // Should not end up in this state
      ASSERT_NAMED( GetAnimationState() != RobotAudioAnimation::AnimationState::AnimationStateCount,
                    "Should never fall into AnimationStateCount state!" );
    }
      break;
  }
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    bool hasBuffer = false;
    size_t frameCount = 0;
    hasBuffer = _audioBuffer->HasAudioBufferStream();
    if ( hasBuffer ) {
      frameCount = _audioBuffer->GetFrontAudioBufferStream()->AudioFrameCount();
    }
    
    PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                  "RobotAudioAnimationOnRobot.Update",
                  "EXIT time_ms: %d State: %s - HasBufferStream: %d <- FrameCount: %zu | HasCurrentStream: %d \
                  <- FrameCount: %zu",
                  streamingTime_ms - startTime_ms,
                  GetStringForAnimationState( GetAnimationState() ).c_str(),
                  hasBuffer,
                  frameCount,
                  HasCurrentBufferStream(),
                  HasCurrentBufferStream() ? _currentBufferStream->AudioFrameCount() : 0 );
  }
  
  return GetAnimationState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateLoadingStream( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  // Wait for new stream
  if ( _audioBuffer->HasAudioBufferStream() &&
      _audioBuffer->GetFrontAudioBufferStream()->HasAudioFrame() ) {
    // Has audio frames
    SetAnimationState( AnimationState::AudioFramesReady );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateLoadingStreamFrames( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  ASSERT_NAMED( HasCurrentBufferStream(),
                "RobotAudioAnimationOnRobot.UpdateLoadingStreamFrames._currentBufferStream.IsNull" );
  if ( _currentBufferStream->HasAudioFrame() ) {
    // Has audio frames
    SetAnimationState( AnimationState::AudioFramesReady );
  }
  else if ( _currentBufferStream->IsComplete() ) {
    // Buffer stream is completed
    _currentBufferStream = nullptr;
    _audioBuffer->PopAudioBufferStream();
    
    // Check if animation audio is completed
    if ( IsAnimationDone() ) {
      SetAnimationState( AnimationState::AnimationCompleted );
    }
    else {
      // Waiting for the next audio stream to load
      SetAnimationState( AnimationState::LoadingStream );
    }
  }
  // Else wait for more frames
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobot::UpdateAudioFramesReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
{
  // Check if current stream is ready for next update lap
  if ( HasCurrentBufferStream() ) {
    
    // Check if the current stream has frames left
    if ( !_currentBufferStream->HasAudioFrame() ) {
      
      // No frames in buffer stream, check if buffer is completed
      if ( _currentBufferStream->IsComplete() ) {
        // Buffer stream is completed
        _currentBufferStream = nullptr;
        _audioBuffer->PopAudioBufferStream();
      }
      else {
        // Need to wait for more frames
        SetAnimationState( AnimationState::LoadingStreamFrames );
      }
    }
    else {
      // Has frames, good to go!
      return;
    }
  }
  
  // If there isn't a current stream, check if it's time to set the next audio stream or send silence messages
  if ( !HasCurrentBufferStream() ) {
    // Check if animation audio is done
    if ( IsAnimationDone() ) {
      SetAnimationState( AnimationState::AnimationCompleted );
      return;
    }
    
    // Check if it's time for the next event
    const TimeStamp_t relevantTime_ms = streamingTime_ms - startTime_ms;
    
    // Prepare next audio stream
    const bool isAudioStreamReady = _audioBuffer->HasAudioBufferStream() && _audioBuffer->GetFrontAudioBufferStream()->HasAudioFrame();
    RobotAudioFrameStream* nextStream = nullptr;
    if ( isAudioStreamReady ) {
      nextStream = _audioBuffer->GetFrontAudioBufferStream();
    }
    
    // First check if there is a stream that is overdue to play
    if ( _didPlayFirstStream && isAudioStreamReady ) {
      const uint32_t streamRelevantTime_ms = floor( nextStream->GetCreatedTime_ms() - _streamAnimationOffsetTime_ms );
      if ( streamRelevantTime_ms <= relevantTime_ms ) {
        // Start playing this stream
        if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
          PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                        "RobotAudioAnimationOnRobot.UpdateAudioFramesReady",
                        "Set Next Stream | SteamTime - StartTime_ms %d | AnimTime_ms %d",
                        streamRelevantTime_ms, relevantTime_ms );
        }
        
        _currentBufferStream = nextStream;
        return;
      }
    }
    
    // Second if there is not an overdue stream check if it's time to play the next stream
    const AnimationEvent* nextEvent = GetNextEvent();
    // Loop thorugh events to remove invalid events and get next vaile event
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
    
    if ( nextEvent != nullptr ) {
      // Have next valid event
      // Check if it's time for play the next valid event
      if ( nextEvent->time_ms <= relevantTime_ms ) {
        if ( isAudioStreamReady ) {
          // Determine the first valid event
          if ( !_didPlayFirstStream ) {
            // Setup inital contition for the fist event
            _didPlayFirstStream = true;
            _streamAnimationOffsetTime_ms = nextStream->GetCreatedTime_ms() - nextEvent->time_ms;
          }
          // Setup stream
          _currentBufferStream = nextStream;
          
          if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
            PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                          "RobotAudioAnimationOnRobot.UpdateAudioFramesReady",
                          "Set Next Stream | EventTime - StartTime_ms %d | AnimTime_ms %d",
                          nextEvent->time_ms, relevantTime_ms );
          }
        }
        else {
          // Wait for next buffer stream
          SetAnimationState( AnimationState::LoadingStream );
        }
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
  const TimeStamp_t relevantTime_ms = streamingTime_ms - startTime_ms;
  
  // Get data from stream
  if ( HasCurrentBufferStream() ) {
    
    AudioFrameData* audioFrame = _currentBufferStream->PopRobotAudioFrame();
    if (nullptr != audioFrame)
    {
      // TEMP: Convert audio frame into correct robot output, this will done in the Mixing Console at some point
      // Create Audio Frame
      AnimKeyFrame::AudioSample keyFrame;
      ASSERT_NAMED(static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) <= keyFrame.Size(),
                   "Block size must be less or equal to audioSameple size");
      // Convert audio format to robot format
      for ( size_t idx = 0; idx < audioFrame->sampleCount; ++idx ) {
        keyFrame.sample[idx] = encodeMuLaw( audioFrame->samples[idx] );
      }
      
      // Pad the back of the buffer with 0s
      // This should only apply to the last frame
      if (audioFrame->sampleCount < static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE )) {
        std::fill( keyFrame.sample.begin() + audioFrame->sampleCount, keyFrame.sample.end(), 0 );
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

  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    bool hasBuffer = _audioBuffer->HasAudioBufferStream();
    size_t frameCount = 0;
    if ( hasBuffer ) {
      frameCount = _audioBuffer->GetFrontAudioBufferStream()->AudioFrameCount();
    }
    PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                  "RobotAudioAnimationOnRobot.PopRobotAudioMessage",
                  "EXIT PopMsg: %d - EXIT State: %s - HasBufferStream: %d <- FrameCount: %zu | HasCurrentStream: \
                  %d <- FrameCount: %zu",
                  (out_RobotAudioMessagePtr != nullptr),
                  GetStringForAnimationState( GetAnimationState() ).c_str(),
                  hasBuffer,
                  frameCount,
                  HasCurrentBufferStream(),
                  HasCurrentBufferStream() ? _currentBufferStream->AudioFrameCount() : 0 );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobot::RobotAudioAnimationOnRobot()
: Anki::Cozmo::Audio::RobotAudioAnimation( GameObjectType::Invalid, nullptr )
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
  
  // Clear the audio buffers now that we are preparing to get new audio data
  _audioBuffer->ClearBufferStreams();
  
  
 // Begin Loading robot audio buffer by posting animation audio events
  SetAnimationState( AnimationState::LoadingStream );
  
  const uint32_t firstAudioEventOffset = _animationEvents.front().time_ms;
  for ( auto& anEvent : _animationEvents ) {
    AnimationEvent* animationEvent = &anEvent;
    std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
    Util::Dispatch::After( _postEventTimerQueue,
                           std::chrono::milliseconds( anEvent.time_ms - firstAudioEventOffset ),
                           [this, animationEvent, isAliveWeakPtr] ()
      {
        // Trigger events if animation is still alive
        if ( isAliveWeakPtr.expired() ) {
          return;
        }
        
        {
          std::lock_guard<std::mutex> lock( _animationEventLock );
          animationEvent->state = AnimationEvent::AnimationEventState::Posted;
        }
        
        // Post Event
        using namespace AudioEngine;
        using PlayId = RobotAudioClient::CozmoPlayId;
        const RobotAudioClient::CozmoEventCallbackFunc callbackFunc = [this, animationEvent, isAliveWeakPtr]
        ( const AudioEngine::AudioCallbackInfo& callbackInfo )
        {
          if ( !isAliveWeakPtr.expired() ) {
            HandleCozmoEventCallback( animationEvent, callbackInfo );
          }
        };
        
        PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                      "RobotAudioAnimationOnRobot.PostEvent",
                      "Anim: '%s' EventId: %u",
                      GetAnimationName().c_str(), animationEvent->audioEvent);
        
        const PlayId playId = _audioClient->PostCozmoEvent( animationEvent->audioEvent,
                                                            _gameObj,
                                                            callbackFunc );
        // Set event's volume RTPC
        if (RobotAudioClient::kInvalidCozmoPlayId != playId) {
          _audioClient->SetCozmoEventParameter( playId,
                                                GameParameter::ParameterType::Event_Volume,
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
