/*
 * File: robotAudioBuffer.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
 *
 * Description: This is a FIFO queue of RobotAudioFrameStreams which contain a continues stream of audio frames. The
 *              RobotAudioAnimation class will pop frames out of the RobotAudioFrameStreams and sync them with the rest
 *              of the animation tracks. Once a RobotAudioFrameStreams is empty it will be popped of the queue. The
 *              Audio Controller passes audio frames provided by the audio engine. First,  PrepareAudioBuffer() is
 *              called by the Audio Controller a new stream is created and pushed onto the back of the _streamQueue.
 *              Next, UpdateBuffer() is called by the Audio Controller to provide audio frames to the _currentStream.
 *              When all audio frames have been added to the stream the Audio Controller will called CloseAudioBuffer()
 *              to complete that stream.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#define DEBUG_ROBOT_AUDIO_BUFFER_LOG  0

// This value is not specific, it is based off data from game play logs.
// The timeout is intended to catch a race condition when an audio event has been posted then immediately stopped. This
// situation will sometimes produce some audio buffer data, however in the circumstance that it doesn't we need to
// timeout and reset the buffer so the next animation can use it.
#define BUFFER_RESET_TIMEOUT_MILLISEC  250
#define BUFFER_RESET_TIMEOUT_NANOSEC   BUFFER_RESET_TIMEOUT_MILLISEC * 1000000

namespace Anki {
namespace Cozmo {
namespace Audio {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PrepareAudioBuffer()
{
  std::lock_guard<std::mutex> lock( _lock );
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.PrepareAudioBuffer", "TimeStamp_s %f",
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace( Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() );
  _isActive = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const AudioEngine::AudioSample* samples, const size_t sampleCount )
{
  std::lock_guard<std::mutex> lock( _lock );
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.UpdateBuffer", "isWaitingForRest %s  TimeStamp_s %f",
                       _isWaitingForReset ? "Y" : "N",
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  // Ignore updates if we are waiting for the plug-in to reset
  if ( _isWaitingForReset ) {
    if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
      PRINT_NAMED_WARNING("RobotAudioBuffer.UpdateBuffer", "Ignore buffer update!");
    }
    // Clear timeout after receiving data from audio engine, if data is received it will eventually close the buffer.
    _beginResetTimeVal = kInvalidResetTimeVal;
    return;
  }
  
  DEV_ASSERT(!_streamQueue.empty(), "RobotAudioBuffer.UpdateBuffer._streamQueue.IsEmpty");
  if ( _streamQueue.empty() ) {
    return;
  }
  
  // Copy audio samples into frame & push it into the queue
  AudioEngine::AudioFrameData *audioFrame = new AudioEngine::AudioFrameData( sampleCount );
  audioFrame->CopySamples( samples, sampleCount );
  _streamQueue.back().PushRobotAudioFrame( audioFrame );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::CloseAudioBuffer()
{
  std::lock_guard<std::mutex> lock( _lock );
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG || DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.CloseAudioBuffer", "TimeStamp_s %f",
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }


  // Mark the last stream in the queue completed
  if ( !_streamQueue.empty() ) {
    _streamQueue.back().SetIsComplete();
  }
  
  _isActive = false;
  _isWaitingForReset = false;
  _beginResetTimeVal = kInvalidResetTimeVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::IsWaitingForReset() const
{
  // Test if reset flag has been set and RobotAudioBuffer has not received any data from audio engine since
  if ( _isWaitingForReset && _beginResetTimeVal != kInvalidResetTimeVal ) {
    const auto elapsed_ns = Util::Time::UniversalTime::GetNanosecondsElapsedSince(_beginResetTimeVal);
    
    if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
      PRINT_NAMED_WARNING("RobotAudioBuffer.IsWaitingForReset", "_isWaitingForReset is True - Elapsed ns: %llu > %d is %c",
                          elapsed_ns, BUFFER_RESET_TIMEOUT_NANOSEC,
                          (elapsed_ns > (BUFFER_RESET_TIMEOUT_NANOSEC) ? 'T' : 'F'));
    }
    
    if ( elapsed_ns > BUFFER_RESET_TIMEOUT_NANOSEC ) {
      // Reset flag after timeout duration has elapsed
      _isWaitingForReset = false;
      _beginResetTimeVal = kInvalidResetTimeVal;
      PRINT_NAMED_WARNING( "RobotAudioBuffer.IsWaitingForReset", "IsWaitingForReset flag has timed out" );
    }
  }
  return _isWaitingForReset;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::HasAudioBufferStream() const
{
  std::lock_guard<std::mutex> lock( _lock );
  return !_streamQueue.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioFrameStream* RobotAudioBuffer::GetFrontAudioBufferStream()
{
  std::lock_guard<std::mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "Must check if a Robot Audio Buffer Stream is in Queue before calling this method") ;
  if (_streamQueue.empty()) {
    return nullptr;
  }
  return &_streamQueue.front();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PopAudioBufferStream()
{
  std::lock_guard<std::mutex> lock( _lock );
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.PopAudioBufferStream", "_StreamQueue Size: %lu  TimeStamp_s %f",
                       (unsigned long)_streamQueue.size(),
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  if ( _streamQueue.empty() ) {
    PRINT_NAMED_ERROR("RobotAudioBuffer.PopAudioBufferStream.EmptyQueue", "Tried to pop from an empty stream queue.");
    return;
  }
  
  DEV_ASSERT(_streamQueue.front().IsComplete(), "RobotAudioBuffer.PopAudioBufferStream._streamQueue.front.IsNOTComplete");
  _streamQueue.pop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearBufferStreams()
{
  std::lock_guard<std::mutex> lock( _lock );
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.ClearBufferStreams", "_StreamQueue Size: %lu  TimeStamp_s %f",
                       (unsigned long)_streamQueue.size(),
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  while ( !_streamQueue.empty() ) {
    _streamQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ResetAudioBufferAnimationCompleted( bool completed )
{
  // Set waiting for reset flag
  {
    std::lock_guard<std::mutex> lock( _lock );
    
    _isWaitingForReset = ( _isActive || !completed );
    
    if ( _isWaitingForReset ) {
      _beginResetTimeVal = Util::Time::UniversalTime::GetCurrentTimeValue();
    }
  }
  
  ClearBufferStreams();
  
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.ResetAudioBufferAnimationCompleted",
                       "_isActive: %c completed: %c reset: %c TimeStamp_s %f",
                       _isActive ? 'Y' : 'N', completed ? 'Y' : 'N', _isWaitingForReset ? 'Y' : 'N',
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
}

  
} // Audio
} // Cozmo
} // Anki
