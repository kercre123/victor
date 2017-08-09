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

#include "engine/audio/robotAudioBuffer.h"
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
  std::lock_guard<std::recursive_mutex> lock( _lock );
  ANKI_VERIFY(!IsActive(),
              "RobotAudioBuffer.PrepareAudioBuffer.UnexpectedNewStream",
              "Starting new AudioStream while existing stream is still active.");
  
  ANKI_VERIFY(!_isWaitingForReset,
              "RobotAudioBuffer.PrepareAudioBuffer.UnexpectedNewStream",
              "Starting new AudioStream while waiting for the Close callback associated with an aborted stream.");
  
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace( Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const AudioEngine::AudioSample* samples, const size_t sampleCount )
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  
  // Ignore updates if we are waiting for the plug-in to reset
  if ( _isWaitingForReset ) {
    PRINT_CH_INFO("Audio",
                  "RobotAudioBuffer.UpdateBuffer.IgnoringDataWhileResetting",
                  "Ignoring audio data while waiting for plugin to reset.");
    return;
  }
  
  const bool isActive = ANKI_VERIFY(IsActive(),
                                    "RobotAudioBuffer.UpdateBuffer.NoActiveStreamsAvailable",
                                    "Audio being delivered with no active Audio Streams to add to.");
  if ( !isActive ) {
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
  std::lock_guard<std::recursive_mutex> lock( _lock );
  // Mark the last stream in the queue completed
  if ( !_streamQueue.empty() ) {
    ANKI_VERIFY(!_streamQueue.back().IsComplete(),
                "RobotAudioBuffer.CloseAudioBuffer.AudioStreamAlreadyComplete",
                "Trying to close a stream that has already been marked complete.");
    _streamQueue.back().SetIsComplete();
  }
  
  _isWaitingForReset = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::IsWaitingForReset() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  return _isWaitingForReset;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::HasAudioBufferStream() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  return !_streamQueue.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::AudioStreamHasData() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "RobotAudioBuffer.AudioStreamHasData.NoAudioStreamAvailable");
  return _streamQueue.front().HasAudioFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::AudioStreamIsComplete() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "RobotAudioBuffer.AudioStreamIsComplete.NoAudioStreamAvailable");
  return _streamQueue.front().IsComplete();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double RobotAudioBuffer::GetAudioStreamCreatedTime_ms() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "RobotAudioBuffer.GetAudioStreamCreatedTime_ms.NoAudioStreamAvailable");
  return _streamQueue.front().GetCreatedTime_ms();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioBuffer::GetAudioStreamFrameCount() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "RobotAudioBuffer.GetAudioStreamFrameCount.NoAudioStreamAvailable");
  return _streamQueue.front().AudioFrameCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AudioEngine::AudioFrameData* RobotAudioBuffer::PopNextAudioFrameData()
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "RobotAudioBuffer.PopNextAudioFrameData.NoAudioStreamAvailable");
  return _streamQueue.front().PopRobotAudioFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioFrameStream* RobotAudioBuffer::GetFrontAudioBufferStream()
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  DEV_ASSERT(!_streamQueue.empty(), "Must check if a Robot Audio Buffer Stream is in Queue before calling this method");
  if (_streamQueue.empty()) {
    return nullptr;
  }
  return &_streamQueue.front();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PopAudioBufferStream()
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  
  if ( _streamQueue.empty() ) {
    PRINT_NAMED_ERROR("RobotAudioBuffer.PopAudioBufferStream.EmptyQueue", "Tried to pop from an empty stream queue.");
    return;
  }
  
  ANKI_VERIFY(_streamQueue.front().IsComplete(),
              "RobotAudioBuffer.PopAudioBufferStream.PoppingIncompleteStream",
              "We should not be popping incomplete audio streams unless calling ResetAudioBufferAnimationCompleted.");
  
  _streamQueue.pop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearBufferStreams()
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  while ( !_streamQueue.empty() ) {
    _streamQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ResetAudioBufferAnimationCompleted()
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  
  // Set waiting for reset flag
  _isWaitingForReset = IsActive();
  
  if ( _isWaitingForReset ) {
    PRINT_CH_INFO("Audio",
                  "RobotAudioBuffer.ResetAudioBufferAnimationCompleted.ResettingEarly",
                  "Resetting early with %zu streams in queue",
                  _streamQueue.size());
  }
  
  ClearBufferStreams();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioBuffer::IsActive() const
{
  std::lock_guard<std::recursive_mutex> lock( _lock );
  return !_streamQueue.empty() && !_streamQueue.back().IsComplete();
}

} // Audio
} // Cozmo
} // Anki
