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


// Calculate how much delay to add to events while playing audio on device instead of on the robot
#define FRAME_LENGTH_SEC 1.0/30.0
#define PLAY_ROBOT_AUDIO_ON_DIVICE_FRAME_COUNT_DELAY 22     // This is just a guess to get webots to sync with audio
static long long kPlayRobotOnDeviceDelayMS = FRAME_LENGTH_SEC * PLAY_ROBOT_AUDIO_ON_DIVICE_FRAME_COUNT_DELAY * 1000;


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
AudioEngineClient::CallbackIdType RobotAudioClient::PostCozmoEvent( GameEvent::GenericEvent event, AudioEngineClient::CallbackFunc callback )
{
  const CallbackIdType callbackId = PostEvent( event, GameObjectType::CozmoAnimation, callback );
  
  return callbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::LoadAnimationAudio( Animation* anAnimation, bool streamAudioToRobot )
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_WARNING("RobotAudioClient.LoadAnimationAudio", "Animation %s", anAnimation->GetName().c_str());
  }
  
  if ( nullptr == _audioBuffer ) {
    return false;
  }
  ASSERT_NAMED( _animationEventList.empty(), "Animation Event List should be empty, we only handle 1 animation at a time");

  // Set defaults
  _isPlayingAnimation = true;
  _isFirstBufferLoaded = false;
  _didBeginPostingEvents = false;
  _animationName = anAnimation->GetName();
  
  // Set PlugIn bypass state for playback type
  _streamAudioToRobot = streamAudioToRobot;
  using namespace GameEvent;
  const GenericEvent plugInBypassEvent = _streamAudioToRobot ? GenericEvent::Hijack_Audio_Disable_Bypass : GenericEvent::Hijack_Audio_Enable_Bypass;
  PostEvent( plugInBypassEvent, GameObjectType::CozmoAnimation );
  
  // Note: It's okay if this value rolls over
  ++_currentAnimationPlayId;
  
  // Clear Audio Buffer Data
  if ( _audioBuffer->HasAudioBufferStream() ) {
    PRINT_STREAM_WARNING( "RobotAudioClient.LoadAnimationAudio", "Should Not have anything in the audio buffer when \
                          loading the an animation" );
    _audioBuffer->ClearBufferStreams();
  }

  // Loop through tracks
  // Prep animation audio events
  Animations::Track<RobotAudioKeyFrame>& audioTrack = anAnimation->GetTrack<RobotAudioKeyFrame>();

  AnimationEvent::AnimationEventId eventId = AnimationEvent::kInvalidAnimationEventId;
  while ( audioTrack.HasFramesLeft() ) {
    RobotAudioKeyFrame& aFrame = audioTrack.GetCurrentKeyFrame();
    audioTrack.MoveToNextKeyFrame();
    
    const GameEvent::GenericEvent event = aFrame.GetAudioRef().audioEvent;
    if ( GameEvent::GenericEvent::Invalid != event ) {
      _animationEventList.emplace_back( ++eventId, aFrame.GetAudioRef().audioEvent, aFrame.GetTriggerTime() );
    }
  }
  
  if ( _animationEventList.empty() ) {
    ClearAnimation();
  }
  else if ( !_streamAudioToRobot ) {
    // Bypass queing events to play through robot buffer
    _didBeginPostingEvents = true;
    _isFirstBufferLoaded = true;
  }
  else if ( !_audioBuffer->IsWaitingForReset() ) {
    BeginBufferingAudioEvents();
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::AbortAnimation()
{
  // Ignore about animation calls if audio client isn't active
  if ( !_isPlayingAnimation ) {
    return;
  }
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_WARNING("RobotAudioClient.AbortAnimation", "Animation: %s", _animationName.c_str());
  }

  // Tell buffer to reset
  if ( nullptr != _audioBuffer ) {
    _audioBuffer->ResetAudioBuffer();
  }
  
  // Clear current animations and silence current playing audio
  ClearAnimation();
  StopAllEvents( GameObjectType::CozmoAnimation );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::PrepareRobotAudioMessage(TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms)
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO && DEBUG_ROBOT_ANIMATION_AUDIO_LEVEL > 0 ) {
    PRINT_NAMED_WARNING("RobotAudioClient.PrepareRobotAudioMessage", "Animation %s - StartTime: %d ms - streamingTime: %d ms", _animationName.c_str(), startTime_ms, streamingTime_ms);
  }
  
  // No audio buffer therefore no audio to play
  if ( nullptr == _audioBuffer ) {
    return true;
  }
  
  bool isMsgReady = false;
  const TimeStamp_t relevantTimeMS = streamingTime_ms - startTime_ms;
  if ( _streamAudioToRobot ) {
    // Prepare audio stream for events if using audio plug-in buffer
    if ( nullptr == _currentBufferStream ) {
      // Check if we need the next stream
      if ( !_animationEventList.empty() ) {
        // Check for invalid audio events
        RemoveInvalidEvents();
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
          UpdateFirstBuffer();
        }
        
      } // if ( !_animationEventList.empty() )
      else {
        // Animation Event List is empty. Send last frame and clear animation.
        ClearAnimation();
        isMsgReady = true;
      } // _animationEventList is empty
    } // if ( nullptr == _currentBufferStream )
    
    // This can be set by code above, check if buffer has key frame
    if ( nullptr != _currentBufferStream && _isFirstBufferLoaded ) {
      // Have Current Stream
      isMsgReady = _currentBufferStream->HasRobotAudioMessage();
    }
  }
  else {
    // Play audio on device
    if ( !_animationEventList.empty() ) {
      // Check if it's time for the next audio buffer stream to start
      if ( _animationEventList.front().TimeInMS <= relevantTimeMS ) {
        const AnimationPlayId animationPlayId = _currentAnimationPlayId;
        const GameEvent::GenericEvent event = _animationEventList.front().AudioEvent;
        Util::Dispatch::After( _postEventTimerQueue, std::chrono::milliseconds( kPlayRobotOnDeviceDelayMS ), [this, animationPlayId, event] ()
                               {
                                 // Trigger events
                                 if (  _currentAnimationPlayId == animationPlayId ) {
                                   PostCozmoEvent( event, [this] ( AudioCallback callback )
                                                   {
                                                     if ( callback.callbackInfo.GetTag() == AudioCallbackInfoTag::callbackError ||
                                                          callback.callbackInfo.GetTag() == AudioCallbackInfoTag::callbackComplete ) {
                                                       if (_animationEventList.empty()) {
                                                         ClearAnimation();
                                                       }
                                                     }
                                                   } );
                                 }
                               } );
        _animationEventList.pop_front();
      }
    }
    isMsgReady = true;
  }
  
  return isMsgReady;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                             TimeStamp_t startTime_ms,
                                             TimeStamp_t streamingTime_ms )
{
  
  if ( DEBUG_ROBOT_ANIMATION_AUDIO && DEBUG_ROBOT_ANIMATION_AUDIO_LEVEL > 0 ) {
    PRINT_NAMED_WARNING("RobotAudioClient.PopRobotAudioMessage", "Animation %s - StartTime: %d ms - streamingTime: %d ms", _animationName.c_str(), startTime_ms, streamingTime_ms);
  }
  
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
bool RobotAudioClient::UpdateFirstBuffer()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO && DEBUG_ROBOT_ANIMATION_AUDIO_LEVEL > 0 ) {
    PRINT_NAMED_WARNING("RobotAudioClient.UpdateFirstBuffer", "Animation %s", _animationName.c_str());
  }
  
  // Either the first buffer is loaded or there is no audio buffer, therefore, audio is ready
  if ( _isFirstBufferLoaded || nullptr == _audioBuffer ) {
    return true;
  }
  
  // Wait until audio buffer is ready for next animation sessions
  if ( _audioBuffer->IsWaitingForReset() ) {
    return false;
  }
  // Now that the audio buffer is ready start buffering the audio events
  else if ( _isPlayingAnimation && !_didBeginPostingEvents ) {
    BeginBufferingAudioEvents();
    return false;
  }
  
  // Check if buffer is ready
  if ( _audioBuffer->HasAudioBufferStream() ) {
    RobotAudioMessageStream* stream = _audioBuffer->GetFrontAudioBufferStream();
    if ( stream->RobotAudioMessageCount() > _PreBufferRobotAudioMessageCount || stream->IsComplete() ) {
      
      _isFirstBufferLoaded = true;
      
      if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
        PRINT_NAMED_WARNING("RobotAudioClient.UpdateFirstBuffer", "Set _isFirstBufferLoaded = true");
      }
    }
  }
  return _isFirstBufferLoaded;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::SetRobotVolume(float volume)
{
  PostParameter(GameParameter::ParameterType::Robot_Volume, volume, GameObjectType::Invalid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::BeginBufferingAudioEvents()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_WARNING("RobotAudioClient.BeginBufferingAudioEvents", "Animation %s", _animationName.c_str());
  }
  // Setup timers to process audio events relevant to each other
  ASSERT_NAMED( !_animationEventList.empty(), "Must have animation evnts!");
  
  // Schedule the events
  const AnimationPlayId animationPlayId = _currentAnimationPlayId;
  const uint32_t firstAudioEventOffset = _animationEventList.front().TimeInMS;
  for ( auto& anEvent : _animationEventList ) {
    const GameEvent::GenericEvent audioEvent = anEvent.AudioEvent;
    const AnimationEvent::AnimationEventId animationEventId = anEvent.EventId;
    Util::Dispatch::After( _postEventTimerQueue,
                           std::chrono::milliseconds( anEvent.TimeInMS - firstAudioEventOffset ),
                           [this, animationPlayId, audioEvent, animationEventId] ()
                           {
                             // Trigger events if aniation is still active
                             if ( _isPlayingAnimation && ( _currentAnimationPlayId == animationPlayId ) ) {
                               PostCozmoEvent( audioEvent,
                                              [this, animationEventId, audioEvent]( AudioCallback callback )
                                              { HandleCozmoEventCallback( animationEventId, audioEvent, callback ); } );
                             }
                           } );
  }
  
  _didBeginPostingEvents = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::ClearAnimation()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_WARNING("RobotAudioClient.ClearAnimation", "Animation %s", _animationName.c_str());\
  }
  
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
void RobotAudioClient::HandleCozmoEventCallback( AnimationEvent::AnimationEventId eventId,
                                                 GameEvent::GenericEvent audioEvent,
                                                 AudioCallback& callback )
{
  // Cancel this animation event if the audio event has an error.
  if ( callback.callbackInfo.GetTag() == AudioCallbackInfoTag::callbackError ) {
    PRINT_STREAM_WARNING( "RobotAudioClient.HandleCozmoEventCallback",
                          "Animation Event Id: " + std::to_string(eventId) +
                          " - Audio Event Id: " + std::to_string( static_cast<uint32_t>( audioEvent ) ) +
                          " failed to play" );
  
    // Add event to Invalid Id list
    std::lock_guard<std::mutex> lock ( _invalidAnimationIdLock );
    _invalidAnimationIds.emplace_back(eventId);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::RemoveInvalidEvents()
{
  std::lock_guard<std::mutex> lock ( _invalidAnimationIdLock );
  if ( !_invalidAnimationIds.empty() ) {
    // Remove events from animation event list
    for ( auto& anInvalidEventId : _invalidAnimationIds ) {
      // Find and remove animation event
      for ( auto animationEventIt = _animationEventList.begin(); animationEventIt != _animationEventList.end(); ++animationEventIt ) {
        if ( animationEventIt->EventId == anInvalidEventId ) {
          _animationEventList.erase( animationEventIt );
          break;
        }
      } // iterate _animationEventList
    } // iterate _invalidAnimationIds
    _invalidAnimationIds.clear();
  }
}
  
} // Audio
} // Cozmo
} // Anki
