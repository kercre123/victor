/*
 * File: standardWaveDataContainer.h
 *
 * Author: Jordan Rivas
 * Created: 04/29/16
 *
 * Description: This struct defines a standard PCM audio buffer and provides basic functionality.
 *
 * Copyright: Anki, Inc. 2016
 *
 */


#ifndef __AnkiAudio_StandardWaveDataContainer_H__
#define __AnkiAudio_StandardWaveDataContainer_H__

#include "audioEngine/audioExport.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include <vector>


namespace Anki {
namespace AudioEngine {

// Define a audio data in a standard form
struct AUDIOENGINE_EXPORT StandardWaveDataContainer
{
  uint32_t sampleRate       = 0;
  uint16_t numberOfChannels = 0;
  size_t   bufferSize       = 0;
  float*   audioBuffer      = nullptr;
  
  StandardWaveDataContainer( uint32_t sampleRate,
                             uint16_t numberOfChannels,
                             size_t   bufferSize = 0 )
  : sampleRate( sampleRate )
  , numberOfChannels( numberOfChannels )
  {
    if ( bufferSize > 0 ) {
      CreateDataBuffer( bufferSize );
    }
  }
  
  ~StandardWaveDataContainer()
  {
    Anki::Util::SafeDeleteArray(audioBuffer);
  }
  
  StandardWaveDataContainer(const StandardWaveDataContainer& other)
  {
    sampleRate = other.sampleRate;
    numberOfChannels = other.numberOfChannels;
    bufferSize = other.bufferSize;
    CreateDataBuffer( bufferSize );
    memcpy( audioBuffer, other.audioBuffer, bufferSize * sizeof(float) );
  }
  
  StandardWaveDataContainer(StandardWaveDataContainer&& other)
  {
    sampleRate = other.sampleRate;
    numberOfChannels = other.numberOfChannels;
    bufferSize = other.bufferSize;
    audioBuffer = other.audioBuffer;
    other.audioBuffer = nullptr;
  }
  
  StandardWaveDataContainer& operator= (StandardWaveDataContainer&& other)
  {
    if (audioBuffer != nullptr) {
      Anki::Util::SafeDeleteArray(audioBuffer);
    }
    sampleRate = other.sampleRate;
    numberOfChannels = other.numberOfChannels;
    bufferSize = other.bufferSize;
    audioBuffer = other.audioBuffer;
    other.audioBuffer = nullptr;
    
    return *this;
  }
  
  // Allocate necessary memory for audio buffer
  bool CreateDataBuffer(const size_t size)
  {
          DEV_ASSERT_MSG(audioBuffer == nullptr,
                         "StandardWaveDataContainer.CreateDataBuffer.AudioBufferNotNull",
                         "Can NOT allocate memory, Audio Buffer is not NULL");
          DEV_ASSERT_MSG(size > 0,
                         "StandardWaveDataContainer.CreateDataBuffer.SizeNotZero",
                         "Must set buffer size");
          
    audioBuffer = new (std::nothrow) float[size];
    if ( audioBuffer == nullptr ) {
      return false;
    }
    
    bufferSize = size;
    return true;
  }
  
  void CopyWaveData(const short* data, size_t sampleCount)
  {
    ASSERT_NAMED(sampleCount <= bufferSize, "StandardWaveDataContainer.CopyWaveData.Short.InvalidSampleCount");
    // Convert waveData format into StandardWaveDataContainer's format
    const float kOneOverSHRT_MAX = 1.0f / float(SHRT_MAX);
    for (size_t sampleIdx = 0; sampleIdx < sampleCount; ++sampleIdx) {
      audioBuffer[sampleIdx] = data[sampleIdx] * kOneOverSHRT_MAX;
    }
  }

  // Used for little-endian short data
  void CopyLittleEndianWaveData(const unsigned char* data, size_t sampleCount)
  {
    ASSERT_NAMED(sampleCount <= bufferSize, "StandardWaveDataContainer.CopyWaveData.Char.InvalidSampleCount");
    // Convert little-endian waveData format into StandardWaveDataContainer's format
    const float kOneOverSHRT_MAX = 1.0f / float(SHRT_MAX);
    for (size_t sampleIdx = 0; sampleIdx < sampleCount; ++sampleIdx) {
        audioBuffer[sampleIdx] = (signed short)((data[sampleIdx*2 +1] << 8) | data[sampleIdx*2]) * kOneOverSHRT_MAX;
    }
  }
  
  // Check if there is an audio buffer
  bool HasBuffer() const
  {
    return ( (bufferSize > 0) && (audioBuffer != nullptr) );
  }
  
  float ApproximateDuration_ms() const
  {
    const double sampleCount = bufferSize / static_cast<double>( numberOfChannels );
    return static_cast<float>( (sampleCount / (double)sampleRate) * 1000.0 );
  }
  
  void ReleaseAudioDataOwnership()
  {
    // Reset vars
    sampleRate = 0;
    numberOfChannels = 0;
    bufferSize = 0;
    audioBuffer = nullptr;
  }
};

} // AudioEngine
} // Anki

#endif /* __AnkiAudio_StandardWaveDataContainer_H__ */
