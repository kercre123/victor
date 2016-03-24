//
//  audioWaveFileReader.hpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 3/23/16.
//
//  Notes:
//    - Read file into memory

#ifndef audioWaveFileReader_hpp
#define audioWaveFileReader_hpp

#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <util/logging/logging.h>
#include <util/helpers/templateHelpers.h>
#include <math.h>

namespace Anki {
namespace Cozmo {
namespace Audio {
  
class AudioWaveFileReader {
  
public:
  
  enum class AudioFormatType : uint16_t {
    None = 0,
    PCM = 1,
    mulaw = 6,
    alaw = 7,
    // There are many more ... but I don't care
  };
  
  
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
    
    float CalculateDuration_ms() {
      const double numberSamples = DataChunkSize / (NumberOfChannels * (BitsPerSample / 8));
      return static_cast<float>( (numberSamples / (double)SamplesPerSec) * 1000.0 );
    }
  };
  
  
  // TODO: This is only for 16-bit .wav files
  struct WaveDataContainer
  {
    WaveHeader Header;
    uint16_t* DataBuffer  = nullptr;

    bool CreateDataBuffer() {
      ASSERT_NAMED( DataBuffer == nullptr, "Can NOT allocate memory, Data Buffer is not Empty" );
      ASSERT_NAMED( Header.NumberOfChannels > 0, "Wave Header must have at least 1 channel" );
      ASSERT_NAMED( Header.SamplesPerSec > 0, "Wave Header must have samples" );
      DataBuffer = new (std::nothrow) uint16_t[Header.DataChunkSize];
      return ( DataBuffer != nullptr );
    }
    
    ~WaveDataContainer() {
      Util::SafeDeleteArray(DataBuffer);
    }
  };
  
  
  
  ~AudioWaveFileReader();
  
  bool LoadWaveFile( const std::string& filePath );
  
  void ClearLoadedWaveData() { Util::SafeDelete(_currentWaveData); }
  
  // TODO: Need to creat method to get Wave file metadata & buffer
  
private:
  
  // TODO: First step is to only open 1 file at a time
  WaveDataContainer* _currentWaveData = nullptr;
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* audioWaveFileReader_hpp */
