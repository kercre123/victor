/*
 * File: robotAudioBuffer.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
 *
 * Description: This a circular buffer to hold audio samples from the Cozmo Plugin for the animation stream to pull out.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "clad/types/animationKeyFrames.h"
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
  
  // When there is enought cache create Key Frame Audio Sample Messages
  while ( _audioSampleCache.size() >= static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) ) {
    CopyAudioSampleCacheToKeyFrameBuffer( static_cast<int32_t>(AnimConstants::AUDIO_SAMPLE_SIZE), _currentStream );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioBufferStream* RobotAudioBuffer::GetFrontAudioBufferStream()
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
  // No more samples to cache, create final Key Frame Audio Sample Message
  size_t sampleCount = _audioSampleCache.size();
  CopyAudioSampleCacheToKeyFrameBuffer( sampleCount, _currentStream );
  
  _currentStream->SetIsComplete();
  
  _currentStream = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioBuffer::CopyAudioSampleCacheToKeyFrameBuffer( size_t blockSize, RobotAudioBufferStream* stream )
{
  // Pop audio samples off cache buffer into a Key frame audio sample message
  AnimKeyFrame::AudioSample* audioSample = new AnimKeyFrame::AudioSample();
  const size_t returnSize = _audioSampleCache.front( &audioSample->sample[0], blockSize);
  _audioSampleCache.pop_front( returnSize );
  assert(returnSize == blockSize);  // We should have already confirmed that we can get this number of itmes

  // Pad the back of the buffer with 0s
  // This should only apply to the last sample
  if (audioSample->Size() > blockSize) {
    std::fill( audioSample->sample.begin() + blockSize, audioSample->sample.end(), 0 );
  }

  ASSERT_NAMED( nullptr!= stream, "Must pass a Robot Audio Buffer Stream object" );
  stream->PushKeyFrame( audioSample );

  return returnSize;
}

  
} // Audio
} // Cozmo
} // Anki
