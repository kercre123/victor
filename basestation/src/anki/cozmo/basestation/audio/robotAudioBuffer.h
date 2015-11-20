/*
 * File: robotAudioBuffer.h
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
 *
 * Description: This a circular buffer to hold audio samples from the Cozmo Plugin for the animation stream to pull out.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioBuffer_H__
#define __Basestation_Audio_RobotAudioBuffer_H__

#include <util/container/circularBuffer.h>
#include <stdint.h>
#include <stdio.h>
#include <mutex>


namespace Anki {
namespace Cozmo {
namespace Audio {
  

class RobotAudioBuffer
{
  
public:
  
  RobotAudioBuffer();
  
  // Write samples to buffer
  void UpdateBuffer( const uint8_t* samples, size_t sampleCount );
  
  // Read samples from buffer
  // Return the sample count added to out buffer array
  size_t GetAudioSamples( uint8_t* outBuffer, size_t sampleCount );
  
  size_t GetBufferSize() const { return _buffer.size(); }
  
  void ClearBuffer();
  
  bool ShouldClearBuffer() const { return _clearBuffer; }
  
private:
  
  static constexpr size_t kSampleBufferSize = 48000;
  
  Util::CircularBuffer<uint8_t> _buffer;
  
  std::mutex _bufferLock;
  
  bool _clearBuffer = false;
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBuffer_H__ */
