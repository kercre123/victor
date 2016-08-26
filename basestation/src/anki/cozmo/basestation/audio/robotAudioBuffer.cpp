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

namespace Anki {
namespace Cozmo {
namespace Audio {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PrepareAudioBuffer()
{
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.PrepareAudioBuffer", "TimeStamp_s %f",
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  // Prep new Continuous Stream Buffer
  std::lock_guard<std::mutex> lock( _lock );
  _streamQueue.emplace( Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() );
  _isActive = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const AudioSample* samples, const size_t sampleCount )
{
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
    return;
  }
  std::lock_guard<std::mutex> lock( _lock );
  
  ASSERT_NAMED(!_streamQueue.empty(), "RobotAudioBuffer.UpdateBuffer._streamQueue.IsEmpty");
  if (_streamQueue.empty())
  {
    return;
  }
  
  // Copy audio samples into frame & push it into the queue
  AudioFrameData *audioFrame = new AudioFrameData( sampleCount );
  audioFrame->CopySamples( samples, sampleCount );
  _streamQueue.back().PushRobotAudioFrame( audioFrame );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::CloseAudioBuffer()
{
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.CloseAudioBuffer", "TimeStamp_s %f",
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }

  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_WARNING("RobotAudioBuffer.ClearCache", "CLEAR!");
  }
  
  // No more samples to cache, create final Audio Message
  std::lock_guard<std::mutex> lock( _lock );
  if ( !_isWaitingForReset ) {
    ASSERT_NAMED(!_streamQueue.empty(), "RobotAudioBuffer.CloseAudioBuffer._streamQueue.IsEmpty");
    if (_streamQueue.empty())
    {
      return;
    }
    _streamQueue.back().SetIsComplete();
  }
  _isActive = false;
  _isWaitingForReset = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioFrameStream* RobotAudioBuffer::GetFrontAudioBufferStream()
{
  std::lock_guard<std::mutex> lock( _lock );
  ASSERT_NAMED( !_streamQueue.empty(), "Must check if a Robot Audio Buffer Stream is in Queue befor calling this method") ;
  if (_streamQueue.empty())
  {
    return nullptr;
  }
  return &_streamQueue.front();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PopAudioBufferStream()
{
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.PopAudioBufferStream", "_StreamQueue Size: %lu  TimeStamp_s %f",
                       (unsigned long)_streamQueue.size(),
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  std::lock_guard<std::mutex> lock( _lock );
  if (_streamQueue.empty())
  {
    PRINT_NAMED_ERROR("RobotAudioBuffer.PopAudioBufferStream.EmptyQueue", "Tried to pop from an empty stream queue.");
    return;
  }
  
  ASSERT_NAMED(_streamQueue.front().IsComplete(), "RobotAudioBuffer.PopAudioBufferStream._streamQueue.front.IsNOTComplete");
  _streamQueue.pop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearBufferStreams()
{
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.ClearBufferStreams", "_StreamQueue Size: %lu  TimeStamp_s %f",
                       (unsigned long)_streamQueue.size(),
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  std::lock_guard<std::mutex> lock( _lock );
  while ( !_streamQueue.empty() ) {
    _streamQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ResetAudioBuffer()
{
  if ( DEBUG_ROBOT_AUDIO_BUFFER_LOG ) {
    PRINT_NAMED_ERROR( "RobotAudioBuffer.ResetAudioBuffer", "_isActive: %c  TimeStamp_s %f",
                       _isActive ? 'Y' : 'N',
                       Util::Time::UniversalTime::GetCurrentTimeInSeconds() );
  }
  
  if ( _isActive ) {
    _isWaitingForReset = true;
  }
}

bool RobotAudioBuffer::HasAudioBufferStream()
{
  std::lock_guard<std::mutex> lock( _lock );
  return !_streamQueue.empty();
}
  
} // Audio
} // Cozmo
} // Anki
