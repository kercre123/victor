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


namespace Anki {
namespace Cozmo {
namespace Audio {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer::RobotAudioBuffer() :
  _buffer( kSampleBufferSize )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const uint8_t* samples, size_t sampleCount )
{
  if ( 0 == sampleCount ) {
    ++_emptyUpdateCount;
    return;
  }
  _emptyUpdateCount = 0;
  
  _bufferLock.lock();
  _buffer.push_back( samples, sampleCount);
  _bufferLock.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioBuffer::GetAudioSamples( uint8_t* outBuffer, size_t sampleCount )
{
  _bufferLock.lock();
  size_t returnSize = _buffer.front( outBuffer, sampleCount );
  _buffer.pop_front( returnSize );
  _bufferLock.unlock();
    
  return returnSize;
}

  
} // Audio
} // Cozmo
} // Anki
