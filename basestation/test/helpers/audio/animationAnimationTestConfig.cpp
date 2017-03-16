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
  DEV_ASSERT(!_lockInsert, "TestAnimationConfig.Insert._lockInsert.IsTrue");
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
std::vector<AnimationAnimationTestConfig::TestAudioEvent> AnimationAnimationTestConfig::FrameAudioEvents( const int32_t frameStartTime_ms, const int32_t frameEndTime_ms )
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
std::vector<AnimationAnimationTestConfig::TestAudioEvent> AnimationAnimationTestConfig::GetCurrentPlayingEvents( const int32_t frameStartTime_ms, const int32_t frameEndTime_ms, const uint32_t framedrift_ms )
{
  std::vector<TestAudioEvent> frameEvents;
  
  for ( auto& anEvent : _events ) {
    // Check if it started in frame
    if ( anEvent.startTime_ms >= frameStartTime_ms && anEvent.startTime_ms <= frameEndTime_ms ) {
      frameEvents.emplace_back( anEvent );
      continue;
    }
    
    // Check if event is playing in frame
    // Started in previous frame and continues to play in this frame
    const uint32_t eventEndTime = anEvent.startTime_ms + anEvent.duration_ms - framedrift_ms;
    if ( anEvent.startTime_ms < frameStartTime_ms && frameStartTime_ms <= eventEndTime ) {
      frameEvents.emplace_back( anEvent );
      continue;
    }
  }
  return frameEvents;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAnimationTestConfig::LoadAudioKeyFrames( Anki::Cozmo::Animation& outAnimation )
{
  DEV_ASSERT(_lockInsert, "TestAnimationConfig.Insert._lockInsert.IsFalse");
  for ( auto& anEvent : _events ) {
    outAnimation.AddKeyFrameToBack( RobotAudioKeyFrame( anEvent.event, anEvent.startTime_ms ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationAnimationTestConfig::LoadAudioBuffer( Anki::Cozmo::Audio::RobotAudioTestBuffer& outBuffer )
{
  // Imitate how Wwise write event audio to buffer. ( This is the perfect condition )
  DEV_ASSERT(_lockInsert, "TestAnimationConfig.Insert._lockInsert.IsFalse");
  if ( _events.empty() ) {
    return;
  }
  
  using RemainingDuration_t = std::vector<uint32_t>;
  RemainingDuration_t eventRemainingDurations;
  
  bool audioComplete = false;
  bool activeStream = false;
  size_t eventIdx = 0;
  int32_t animationFrameBeginTime_ms = 0;
  int32_t animationFrameEndTime_ms = animationFrameBeginTime_ms + Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS - 1;

  // Shif events forward in stream to offset the difference frame "wiggle room"
  uint32_t streamEventOffset_ms = 0;
  uint32_t streamFirstEventStart_ms = 0;
  
  // Thick Frames
  while (!audioComplete) {
    
    bool newStream = !activeStream;
    bool startNewStream = false;
    
    // Start all events that start in this frame
    while (IsNextEventReady(eventIdx, animationFrameEndTime_ms)) {
      const auto& nextEvent = _events[eventIdx];
      if (newStream) {
        // First event in stream.
        newStream = false;
        startNewStream = true;
        // Because we don't breake frames into samples the first event shifts the remaing events forward in time in it's stream
        // I refer to this as "Frame Drift"
        streamFirstEventStart_ms = nextEvent.startTime_ms;
        streamEventOffset_ms = streamFirstEventStart_ms % Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
        eventRemainingDurations.push_back(nextEvent.duration_ms);
      }
      else {
        // Add offest to make up for "Frame drift"
        uint32_t eventFrameOffset_ms = nextEvent.startTime_ms - (animationFrameBeginTime_ms + streamEventOffset_ms);
        eventRemainingDurations.push_back(nextEvent.duration_ms + eventFrameOffset_ms);
      }
      ++eventIdx;
    }
    
    // Start new stream
    if (startNewStream) {
      // Add Animation time to mock the delay before a stream is created with
      outBuffer.PrepareAudioBuffer(Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() + animationFrameBeginTime_ms);
      activeStream = true;
    }
    
    // Check if stream is completed
    if (activeStream) {
      // Check if we need to add audio frame
      
      // FIXME: Doesn't create frame for Single frame stream
      
      for (auto it = eventRemainingDurations.begin(); it != eventRemainingDurations.end();) {
        const int32_t updatedRemainingDuration = *it - Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
        if (updatedRemainingDuration > 0) {
          // Update remaining duration, don't remove
          *it = updatedRemainingDuration;
          ++it;
        }
        else {
          // Remove
          it = eventRemainingDurations.erase(it);
        }
      }
      
      // Add Frame
      using namespace AudioEngine;
      const AudioSample* samples = new const AudioSample[ (size_t)Anki::Cozmo::AnimConstants::AUDIO_SAMPLE_SIZE ]();
      outBuffer.UpdateBuffer( samples, (size_t)Anki::Cozmo::AnimConstants::AUDIO_SAMPLE_SIZE );
      Util::SafeDeleteArray( samples );
      
      // Check if this is the last frame
      if (eventRemainingDurations.empty()) {
        // Close stream
        outBuffer.CloseAudioBuffer();
        activeStream = false;
        streamEventOffset_ms = 0;
      }
      
      // Check if animation is completed
      if (!activeStream && eventIdx >= _events.size()) {
        audioComplete = true;
      }
    }
    
    // Tick
    animationFrameBeginTime_ms += Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
    animationFrameEndTime_ms += Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper Function
bool AnimationAnimationTestConfig::IsNextEventReady(size_t idx, uint32_t untilTime_ms)
{
  return ( (idx < _events.size()) && (_events[idx].startTime_ms <= untilTime_ms) );
}
