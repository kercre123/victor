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
#include <util/logging/logging.h>


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
  _bufferLock.lock();
  _clearBuffer = false;
  _buffer.push_back( samples, sampleCount );
//  for (size_t idx = 0; idx < sampleCount; ++idx) {
//    _buffer.push_back(samples[idx]);
//  }
 
  PRINT_NAMED_INFO("RobotAudioBuffer.UpdateBuffer", "UpdateSize: %zu Post Size : %zu", sampleCount, _buffer.size());
  
  _bufferLock.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioBuffer::GetAudioSamples( uint8_t* outBuffer, size_t sampleCount )
{
  _bufferLock.lock();

//  size_t returnSize = 0;
//  for (size_t idx = 0; idx < sampleCount; ++idx) {
//    if (_buffer.empty()) {
//      break;
//    }
//    outBuffer[idx] = _buffer.front();
//    _buffer.pop_front();
//    ++returnSize;
//  }
  
  const size_t returnSize = _buffer.front( outBuffer, sampleCount );
  _buffer.pop_front( returnSize );
  _bufferLock.unlock();
  
  return returnSize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearBuffer()
{
  _bufferLock.lock();
  _clearBuffer = true;
  _bufferLock.unlock();
}

  
} // Audio
} // Cozmo
} // Anki
