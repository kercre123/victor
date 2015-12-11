/*
 * File: robotAudioClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/audio/robotAudioMessageStream.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/animation/animation.h"
#include "clad/audio/messageAudioClient.h"


#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::RobotAudioClient()
{
  _postEventTimerQueue = Util::Dispatch::Create( "PostEventTimerQueue" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::~RobotAudioClient()
{
  Util::Dispatch::Release( _postEventTimerQueue );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::CallbackIdType RobotAudioClient::PostCozmoEvent( EventType event, AudioCallbackFlag callbackFlag )
{
  const CallbackIdType callbackId = PostEvent( event, 0, callbackFlag );
  
  return callbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::LoadAnimationAudio(Animation* anAnimation)
{
  if ( nullptr == _audioBuffer ) {
    return false;
  }
  ASSERT_NAMED( _animationEventList.empty(), "Animation Event List should be empty, we only handle 1 animation at a time");

  // Set defaults
  _isPlayingAnimation = true;
  _isFirstBufferLoaded = false;
  _animationName = anAnimation->GetName();
  _firstAudioEventOffset = 0;
  
  // Clear Audio Buffer Data
  if ( _audioBuffer->HasAudioBufferStream() ) {
    PRINT_STREAM_WARNING( "RobotAudioClient.LoadAnimationAudio", "Should Not have anything in the audio buffer when \
                          loading the an animation" );
    _audioBuffer->ClearBufferStreams();
  }

  // Loop through tracks
  // Prep sound
  Animations::Track<RobotAudioKeyFrame>& audioTrack = anAnimation->GetTrack<RobotAudioKeyFrame>();

  while ( audioTrack.HasFramesLeft() ) {
    RobotAudioKeyFrame& aFrame = audioTrack.GetCurrentKeyFrame();
    audioTrack.MoveToNextKeyFrame();
    
    const EventType event = aFrame.GetAudioRef().audioEvent;
    if ( EventType::Invalid != event ) {
      _animationEventList.emplace_back( aFrame.GetAudioRef().audioEvent, aFrame.GetTriggerTime() );
    }
  }
  
  // Setup timers to process audio events relevant to each other
  if ( _animationEventList.size() > 0 ) {
    // Schedule the events
    bool firstEvent = true;
    for ( auto& anEvent : _animationEventList ) {
      if ( firstEvent ) {
        firstEvent = false;
        _firstAudioEventOffset = anEvent.TimeInMS;
      }
      
      const EventType event = anEvent.AudioEvent;
      Util::Dispatch::After( _postEventTimerQueue, std::chrono::milliseconds( anEvent.TimeInMS - _firstAudioEventOffset ), [this, event] ()
      {
        // Trigger events
        PostCozmoEvent( event );
      } );
    }
  }
  else {
    // There are no audio events
    ClearAnimation();
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::ClearAnimation()
{
  _isPlayingAnimation = false;
  _isFirstBufferLoaded = false;
  _animationName = "";
  _animationEventList.clear();
  _currentBufferStream = nullptr;
  if ( nullptr != _audioBuffer ) {
    _audioBuffer->ClearBufferStreams();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::PrepareRobotAudioMessage(TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms)
{
  // No audio buffer therefore no audio to play
  if ( nullptr == _audioBuffer ) {
    return true;
  }
    
  bool isMsgReady = false;
  const TimeStamp_t relevantTimeMS = streamingTime_ms - startTime_ms;
  if ( nullptr == _currentBufferStream ) {
    // Check if we need the next stream
    if ( !_animationEventList.empty() ) {
      // Check if it's time for the next audio buffer stream to start
      if ( _animationEventList.front().TimeInMS <= relevantTimeMS ) {
        // Get next buffer stream if available && pop pending animation event
        if ( _audioBuffer->HasAudioBufferStream() ) {
          _currentBufferStream = _audioBuffer->GetFrontAudioBufferStream();
          _animationEventList.pop_front();
        } // if ( _audioBuffer->HasAudioBufferStream() )
        
      } // if ( _animationEventList.front().TimeInMS <= streamingTime_ms )
      
      else if ( _isFirstBufferLoaded ) {
        isMsgReady = true;
      }
      else if ( !_isFirstBufferLoaded ) {
        // See if buffer is ready now
        IsFirstBufferReady();
      }
      
    } // if ( !_animationEventList.empty() )
    else {
      // Animation Event List is empty. Send last frame and clear animation.
      ClearAnimation();
      isMsgReady = true;
    } // _animationEventList is empty
    
  } // if ( nullptr == _currentBufferStream )
  
  // This can be set by code above  check if buffer has key frame
  if (nullptr != _currentBufferStream && _isFirstBufferLoaded) {
    // Have Current Stream
    isMsgReady = _currentBufferStream->HasRobotAudioMessage();
  }
  
  return isMsgReady;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                             TimeStamp_t startTime_ms,
                                             TimeStamp_t streamingTime_ms )
{
  // No audio buffer therefore no audio to play
  if ( nullptr == _audioBuffer ) {
    out_RobotAudioMessagePtr = nullptr;
    return true;
  }
  
  const TimeStamp_t relevantTimeMS = streamingTime_ms - startTime_ms;
  const bool isMsgReady = PrepareRobotAudioMessage( startTime_ms, streamingTime_ms );
  
  // Send Key Frame
  // Either new steam or continue sending stream
  if ( isMsgReady && nullptr != _currentBufferStream ) {
    // Try to pop key frame
    if ( _currentBufferStream->HasRobotAudioMessage() ) {
      out_RobotAudioMessagePtr = _currentBufferStream->PopRobotAudioMessage();
      
      // Check if sound events are mixed together
      if ( !_animationEventList.empty() && _animationEventList.front().TimeInMS <= relevantTimeMS ) {
        _animationEventList.pop_front();
      }
      
      // Check if stream is complete and if there are any key frames left
      if ( _currentBufferStream->IsComplete() && !_currentBufferStream->HasRobotAudioMessage() ) {
        _currentBufferStream = nullptr;
        _audioBuffer->PopAudioBufferStream();
      }
    }
    else {
      // Wait for more audio key frames to buffer, this will complete animation with silent frame
      out_RobotAudioMessagePtr = nullptr;
    }
  }
  else if ( isMsgReady ) {
    // Nothing to send, insert silence key frame
    out_RobotAudioMessagePtr = nullptr;
  }
  else {
    // Need to wait for next Audio buffer to be created
    out_RobotAudioMessagePtr = nullptr;
    return false;
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::IsFirstBufferReady()
{
  // Either the first buffer is loaded or there is no audio buffer, therefore, audio is ready
  if ( _isFirstBufferLoaded || nullptr == _audioBuffer ) {
    return true;
  }
  
  // Check if buffer is ready
  if ( _audioBuffer->HasAudioBufferStream() ) {
    RobotAudioMessageStream* stream = _audioBuffer->GetFrontAudioBufferStream();
    if ( stream->RobotAudioMessageCount() > _PreBufferRobotAudioMessageCount || stream->IsComplete() ) {
      _isFirstBufferLoaded = true;
    }
  }
  return _isFirstBufferLoaded;
}


// Protected
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackDuration& callbackMsg )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackMarker& callbackMsg )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackComplete& callbackMsg )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackError& callbackMsg )
{
}


} // Audio
} // Cozmo
} // Anki
