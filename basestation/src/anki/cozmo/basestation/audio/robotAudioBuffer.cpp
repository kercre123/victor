/*
 * File: robotAudioBuffer.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
 *
 * Description: This consists of a circular buffer to cache the audio samples from the Cozmo Plugin update. When there
 *              is enough cached the data packed into a EngineToRobot audio sample message and is pushed into a
 *              RobotAudioMessageStream. The RobotAudioMessageStreams are stored in a FIFO queue until they are ready
 *              to be sent to the robot.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer::RobotAudioBuffer() :
  _audioSampleCache( kAudioSampleBufferSize)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PrepareAudioBuffer()
{
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace();
  _currentStream = &_streamQueue.back();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const uint8_t* samples, size_t sampleCount )
{
  // Store plug-in samples into cache
  _audioSampleCache.push_back( samples, sampleCount );
  
  // When there is enought cache create Robot Audio Messages
  while ( _audioSampleCache.size() >= static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) ) {
    CopyAudioSampleCacheToRobotAudioMessage( static_cast<int32_t>(AnimConstants::AUDIO_SAMPLE_SIZE), _currentStream );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioMessageStream* RobotAudioBuffer::GetFrontAudioBufferStream()
{
  ASSERT_NAMED( !_streamQueue.empty(), "Must check if a Robot Audio Buffer Stream is in Queue befor calling this method") ;
  
  return &_streamQueue.front();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearBufferStreams()
{
  while (!_streamQueue.empty()) {
    _streamQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearCache()
{
  // No more samples to cache, create final Audio Message
  size_t sampleCount = _audioSampleCache.size();
  CopyAudioSampleCacheToRobotAudioMessage( sampleCount, _currentStream );
  
  _currentStream->SetIsComplete();
  
  _currentStream = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioBuffer::CopyAudioSampleCacheToRobotAudioMessage( size_t blockSize, RobotAudioMessageStream* stream )
{
  // TODO: Add support to write into directly into clad robot messages audio sample to save us a copy step.
  
  // Pop audio samples off cache buffer into an audio sample message
  AnimKeyFrame::AudioSample audioSample = AnimKeyFrame::AudioSample();
  ASSERT_NAMED(blockSize <= audioSample.Size(), "Block size must be less or equal to audioSameple size");
  const size_t returnSize = _audioSampleCache.front( audioSample.sample.data(), blockSize);
  _audioSampleCache.pop_front( returnSize );
  ASSERT_NAMED( returnSize == blockSize, "We expect that the returnSize is equal to blockSize" );

  // Pad the back of the buffer with 0s
  // This should only apply to the last sample
  if (audioSample.Size() > blockSize) {
    std::fill( audioSample.sample.begin() + blockSize, audioSample.sample.end(), 0 );
  }

  ASSERT_NAMED( nullptr!= stream, "Must pass a Robot Audio Buffer Stream object" );
  RobotInterface::EngineToRobot* audioMsg = new RobotInterface::EngineToRobot( std::move( audioSample ) );
  
  stream->PushRobotAudioMessage( audioMsg );

  return returnSize;
}

  
} // Audio
} // Cozmo
} // Anki
