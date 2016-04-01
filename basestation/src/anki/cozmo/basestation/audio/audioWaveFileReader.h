//
//  audioWaveFileReader.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 3/23/16.
//
//  Notes:
//    - Read file into memory

#ifndef __Basestation_Audio_AudioWaveFileReader_H__
#define __Basestation_Audio_AudioWaveFileReader_H__

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Anki {
namespace Cozmo {
namespace Audio {
  
class AudioWaveFileReader {
  
public:
  
  // Define a audio data in a standard form
  struct StandardWaveDataContainer
  {
    uint32_t SampleRate       = 0;
    uint16_t NumberOfChannels = 0;
    float    Duration_ms      = 0.0;
    size_t   BufferSize       = 0;
    float*   AudioBuffer      = nullptr;
    
    StandardWaveDataContainer( uint32_t sampleRate,
                               uint16_t numberOfChannels,
                               float    duration_ms,
                               size_t   bufferSize = 0 ) :
      SampleRate( sampleRate ),
      NumberOfChannels( numberOfChannels ),
      Duration_ms( duration_ms )
    {
      if ( bufferSize > 0 ) {
        CreateDataBuffer( bufferSize );
      }
    }
    
    ~StandardWaveDataContainer()
    {
      Util::SafeDelete(AudioBuffer);
    }
    
    // Allocate nessary memory for audio buffer
    bool CreateDataBuffer(const size_t bufferSize) {
      ASSERT_NAMED( AudioBuffer == nullptr, "Can NOT allocate memory, Audio Buffer is not NULL" );
      ASSERT_NAMED( bufferSize > 0, "Must set buffer size" );
      
      AudioBuffer = new (std::nothrow) float[bufferSize];
      if ( AudioBuffer == nullptr ) {
        return false;
      }
      
      BufferSize = bufferSize;
      return true;
    }
    
    // Cheic there is an audio buffer
    bool HasBuffer()
    {
      return ( (BufferSize > 0) && (AudioBuffer != nullptr) );
    }
  };
  
  ~AudioWaveFileReader();
  
  bool LoadWaveFile( const std::string& filePath );
  
  const StandardWaveDataContainer* GetCachedWaveDataWithKey( const std::string& key );
  
  void ClearChachedWaveDataWithKey( const std::string& key );
  
  void ClearCallChachedWaveData();
  
  
private:
  
  // .wav audio format types
  enum class AudioFormatType : uint16_t {
    None = 0,
    PCM = 1,
    mulaw = 6,
    alaw = 7,
    // There are many more ... but I don't care
  };
  
  // .wav header informations
  struct WaveHeader
  {
    // RIFF "Chunk" Description
    uint8_t         Riff[4];                  // Chunk Id "RIFF" string
    uint32_t        ChunkSize          = 0;   // Size of entire chunk
    uint8_t         Wave[4];                  // Format "WAVE" string
    
    // Format "Chunk"
    uint8_t         Fmt[4];                   // Chunk Id "fmt " string
    uint32_t        FmtChunkSize      = 0;    // Size of Format chunk
    AudioFormatType AudioFormat       = AudioFormatType::None;  // Audio format type
    uint16_t        NumberOfChannels  = 0;    // Number of audio channels
    uint32_t        SamplesPerSec     = 0;    // Sample frequency in Hz
    uint32_t        BytesPerSec       = 0;    // Bytes per second
    uint16_t        BlockAlign        = 0;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t        BitsPerSample     = 0;    // Number of bits per sample - Bit depth
    
    // Data "Chunk"
    uint8_t         DataChunkId[4];           // Chunk Id "data" string
    uint32_t        DataChunkSize     = 0;    // Size of Data
    
    float CalculateDuration_ms() const {
      const double numberSamples = DataChunkSize / (NumberOfChannels * (BitsPerSample / 8));
      return static_cast<float>( (numberSamples / (double)SamplesPerSec) * 1000.0 );
    }
    
    size_t CalculateNumberOfStandardSamples() const {
      const uint8_t numberOfBytesPerSample = BitsPerSample / 8;
      return DataChunkSize / numberOfBytesPerSample;
    }
  };

  // Audio file data held in memory
  std::unordered_map< std::string, StandardWaveDataContainer* > _cachedWaveData;
  
  // Convert PCM formatted data into 32-bit float
  // PCM is 8, 16, 24 or 32 bit signed data, little endian
  // TODO: This only works for 16-bit, need to implement 8, 24 & 32 bit data
  // Must have adequate size of source buffer to store source buffer data
  // Return number of bytes writen to standard buffer
  size_t ConvertPCMDataStream( WaveHeader& waveHeader,
                               unsigned char* sourceBuffer,
                               size_t sourceBuffSize,
                               size_t samplesPerChannel,
                               float* out_standardBuffer );
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioWaveFileReader_H__ */
