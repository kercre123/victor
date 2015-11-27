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
  _audioSampleCache( kAudioSampleBufferSize),
  _KeyFrameAudioSampleBuffer( kKeyFrameAudioSampleBufferSize )
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const uint8_t* samples, size_t sampleCount )
{
  // Store plug-in samples into cache
  _audioSampleCache.push_back( samples, sampleCount );
  
  // When there is enought cache create Key Frame Audio Sample Messages
  while ( _audioSampleCache.size() >= static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) ) {
    CopyAudioSampleCachToKeyFrameAudioSample( static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) );
  }
  
  _plugInIsActive = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimKeyFrame::AudioSample* RobotAudioBuffer::PopKeyFrameAudioSample()
{
  assert( !_KeyFrameAudioSampleBuffer.empty() );

  // Lock and Pop Key Frame Audio Sample message
  _KeyFrameAudioSampleBufferLock.lock();
  AnimKeyFrame::AudioSample* audioSample = _KeyFrameAudioSampleBuffer.front();
  _KeyFrameAudioSampleBuffer.pop_front();
  
  // If the last frame was poped plug-in is no longer active
  if ( _KeyFrameAudioSampleBuffer.empty() && _clearCache ) {
    _plugInIsActive = false;
    _clearCache = false;
  }
  _KeyFrameAudioSampleBufferLock.unlock();
  
  return audioSample;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearCache()
{
  // No more samples to cache, create final Key Frame Audio Sample Message
  size_t sampleCount = _audioSampleCache.size();
  CopyAudioSampleCachToKeyFrameAudioSample( sampleCount );
  
  _clearCache = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioBuffer::CopyAudioSampleCachToKeyFrameAudioSample( size_t blockSize )
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

  // Lock and push key frame audio sample message into buffer
  _KeyFrameAudioSampleBufferLock.lock();
  _KeyFrameAudioSampleBuffer.push_back( audioSample );
  _KeyFrameAudioSampleBufferLock.unlock();

  return returnSize;
}
  
} // Audio
} // Cozmo
} // Anki
