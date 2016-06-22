/*
 * File: animationAnimationTestConfig.h
 *
 * Author: Jordan Rivas
 * Created: 6/17/16
 *
 * Description: Configure an audio animation scenario by inserting each audio event. Generate Animation &
 *              RobotAudioBuffer for given config.
 *
 * Copyright: Anki, Inc. 2016
 *
 */


#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "helpers/audio/animationAnimationTestConfig.h"
#include "helpers/audio/robotAudioTestClient.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/time/universalTime.h"


using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::Audio;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAnimationTestConfig::Insert( TestAudioEvent&& audioEvent )
{
  ASSERT_NAMED(!_lockInsert, "TestAnimationConfig.Insert._lockInsert.IsTrue");
  _events.emplace_back( audioEvent );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAnimationTestConfig::InsertComplete()
{
  _lockInsert = true;
  
  // Sort by start time
  std::sort( _events.begin(),
            _events.end(),
            []( const TestAudioEvent& lhs, const TestAudioEvent& rhs )
            { return lhs.startTime_ms < rhs.startTime_ms; } );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<AnimationAnimationTestConfig::TestAudioEvent> AnimationAnimationTestConfig::FrameAudioEvents( const uint32_t frameStartTime_ms, const uint32_t frameEndTime_ms )
{
  std::vector<TestAudioEvent> frameEvents;
  for ( auto& anEvent : _events ) {
    // Ignore events before frame
    if ( anEvent.startTime_ms < frameStartTime_ms ) {
      continue;
    }
    // Events within frame
    if ( anEvent.startTime_ms <= frameEndTime_ms ) {
      frameEvents.emplace_back( anEvent );
    }
    // Exit loop when events are after frame
    if ( anEvent.startTime_ms > frameEndTime_ms ) {
      break;
    }
  }
  
  return frameEvents;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<AnimationAnimationTestConfig::TestAudioEvent> AnimationAnimationTestConfig::GetCurrentPlayingEvents( const uint32_t frameStartTime_ms, const uint32_t frameEndTime_ms )
{
  std::vector<TestAudioEvent> frameEvents;
  
  for ( auto& anEvent : _events ) {
    // Check if it started in frame
    if ( anEvent.startTime_ms >= frameStartTime_ms && anEvent.startTime_ms <= frameEndTime_ms ) {
      frameEvents.emplace_back( anEvent );
      continue;
    }
    
    // Check if event is playing in frame
    const uint32_t eventEndTime = anEvent.startTime_ms + anEvent.duration_ms;
    if ( anEvent.startTime_ms < frameStartTime_ms && eventEndTime > frameStartTime_ms ) {
      frameEvents.emplace_back( anEvent );
      continue;
    }
  }
  return frameEvents;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAnimationTestConfig::LoadAudioKeyFrames( Anki::Cozmo::Animation& outAnimation )
{
  ASSERT_NAMED(_lockInsert, "TestAnimationConfig.Insert._lockInsert.IsFalse");
  for ( auto& anEvent : _events ) {
    outAnimation.AddKeyFrameToBack( RobotAudioKeyFrame( anEvent.event, anEvent.startTime_ms ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAnimationTestConfig::LoadAudioBuffer( Anki::Cozmo::Audio::RobotAudioTestBuffer& outBuffer )
{
  // Imitate how Wwise write event audio to buffer. ( This is the perfect condition )
  ASSERT_NAMED(_lockInsert, "TestAnimationConfig.Insert._lockInsert.IsFalse");
  if ( _events.empty() ) {
    return;
  }
  
  std::vector<uint32_t> _eventRemainingDurations;
  bool audioComplete = false;
  bool activeFrame = false;
  bool activeStream = false;
  size_t eventIdx = 0;
  uint32_t animationTime_ms = 0;
  uint32_t streamTime_ms = 0;
  uint32_t frameTime_ms = 0;
  
  uint32_t firstEventOffset = _events.front().startTime_ms;
  
  
  while ( !audioComplete ) {
    
    // Start New stream
    if ( !activeStream ) {
      
      if ( eventIdx < _events.size() ) {
        // Create new stream
        // This is a test method, normally creation time is set when wwise creates the stream
        // + 10 ms to represent delay in WWise
        double eventDelay = ( _events[eventIdx].startTime_ms - firstEventOffset ) + 10;
        outBuffer.PrepareAudioBuffer( Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() + eventDelay );
        // Skip foward to next event
        animationTime_ms = _events[eventIdx].startTime_ms;
        
        // Reset Frame
        activeFrame = true;
        activeStream = true;
        streamTime_ms = 1;
      }
      else {
        // No more events
        audioComplete = true;
        break;
      }
    }
    
    frameTime_ms = streamTime_ms % Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
    
    // Check frame state
    if ( frameTime_ms == 0 ) {
      // End of last frame
      activeFrame = false;
      // Post Frame
      const AudioSample* samples = new const AudioSample[ (size_t)Anki::Cozmo::AnimConstants::AUDIO_SAMPLE_SIZE ]();
      outBuffer.UpdateBuffer( samples, (size_t)Anki::Cozmo::AnimConstants::AUDIO_SAMPLE_SIZE );
      Util::SafeDeleteArray( samples );
    }
    else if ( frameTime_ms == 1 ) {
      // New frame
      activeFrame = true;
    }
    
    // If not the last check if the next event starts this
    uint32_t nextEventIdx = (uint32_t)eventIdx + 1;
    while ( nextEventIdx < _events.size() ) {
      if ( _events[ nextEventIdx ].startTime_ms == animationTime_ms ) {
        ++nextEventIdx;
        ++eventIdx;
      }
      else {
        // Breake out of loop
        break;
      }
    }
    
    // Check if an event is producing audio
    bool activeEvent = false;
    for ( size_t idx = 0; idx <= eventIdx; ++idx ) {
      activeEvent = _events[idx].InAnimationTimeRange( animationTime_ms );
      if ( activeEvent ) {
        // Only 1 event needs to be playing (Only way out)
        break;
      }
    }
    
    // Check if audio is over for animation time
    if ( !activeEvent ) {
      // End of partial frame & stream
      activeFrame = false;
      activeStream = false;
      // Progress to next event, all current events are completed
      ++eventIdx;
      
      // Send samples of last milliSec
      uint32_t sampleCount = (size_t)Anki::Cozmo::AnimConstants::AUDIO_SAMPLE_SIZE;
      if ( frameTime_ms < Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS ) {
        // Last milli sec in frame send entire frame count
        sampleCount = numberOfSamples_ms( frameTime_ms -  1 ); // sift to zero index
        
      }
      
      const AudioSample* samples = new const AudioSample[ sampleCount ]();
      outBuffer.UpdateBuffer( samples, sampleCount );
      outBuffer.CloseAudioBuffer();
      Util::SafeDeleteArray( samples );
    }
    
    // Tick
    ++animationTime_ms;
    ++streamTime_ms;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t AnimationAnimationTestConfig::numberOfSamples_ms( uint32_t milliSec )
{
  // 33.3333/1000*22320
  return ceilf( (float)milliSec / 1000.0f * (float)Anki::Cozmo::AnimConstants::AUDIO_SAMPLE_RATE );
}



