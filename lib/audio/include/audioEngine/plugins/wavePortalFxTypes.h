/*
 * File: wavePortalFxTypes.h
 *
 * Author: Jordan Rivas
 * Created: 3/24/2016
 *
 * Description: WavePortal plug-in types
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __AnkiAudio_PlugIns_WavePortalFxTypes_H__
#define __AnkiAudio_PlugIns_WavePortalFxTypes_H__

#include <cstdint>
#include <memory>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Continuous stream of audio data
struct __attribute__((visibility("default"))) AudioDataStream
{
  // Samples per second
  uint32_t sampleRate       = 0;
  // Channel Count
  uint16_t numberOfChannels = 0;
  // Duration MilliSeconds
  float    duration_ms      = 0.0f;
  // Number of samples in audio buffer
  uint32_t bufferSize       = 0;
  // Audio sample data buffer
  // Note: Always allocate memory using new[] if useing float* to create unique_ptr
  std::unique_ptr<float[]> audioBuffer;
  
  AudioDataStream() {}
  
  // Note: Constructor doesn't copy audio buffer
  AudioDataStream(uint32_t                    sampleRate,
                  uint16_t                    numberOfChannels,
                  float                       duration_ms,
                  uint32_t                    bufferSize,
                  std::unique_ptr<float[]>&&  audioBuffer)
  : sampleRate(sampleRate)
  , numberOfChannels(numberOfChannels)
  , duration_ms(duration_ms)
  , bufferSize(bufferSize)
  , audioBuffer(std::move(audioBuffer))
  {}
  
  AudioDataStream(const AudioDataStream& other)
  {
    sampleRate = other.sampleRate;
    numberOfChannels = other.numberOfChannels;
    duration_ms = other.duration_ms;
    bufferSize = other.bufferSize;
    audioBuffer = std::make_unique<float[]>(bufferSize);
    memcpy(audioBuffer.get(), other.audioBuffer.get(), bufferSize * sizeof(float));
  }
  
  AudioDataStream(AudioDataStream&& other)
  {
    sampleRate = other.sampleRate;
    numberOfChannels = other.numberOfChannels;
    duration_ms = other.duration_ms;
    bufferSize = other.bufferSize;
    audioBuffer = std::move(other.audioBuffer);
  }
  
  AudioDataStream& operator= (AudioDataStream&& other)
  {
    sampleRate = other.sampleRate;
    numberOfChannels = other.numberOfChannels;
    duration_ms = other.duration_ms;
    bufferSize = other.bufferSize;
    audioBuffer = std::move(other.audioBuffer);
    return *this;
  }
  
};


} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_WavePortalFxTypes_H__ */
