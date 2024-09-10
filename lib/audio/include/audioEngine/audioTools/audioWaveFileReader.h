/*
 * File: audioWaveFileReader.h
 *
 * Author: Jordan Rivas
 * Created: 3/23/2016
 *
 * Description: This class purpose is to read .wav files from disk and store them in memory. When the file is loaded the
 *              data is reformatted to be in a standard audio format, 32-bit floating point.
 *
 * Copyright: Anki, Inc. 2016
 *
 */


#ifndef __AnkiAudio_AudioWaveFileReader_H__
#define __AnkiAudio_AudioWaveFileReader_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include <cstdint>
#include <string>
#include <unordered_map>


namespace Anki {
namespace AudioEngine {
  
class AUDIOENGINE_EXPORT AudioWaveFileReader {
  
public:
  
  
  ~AudioWaveFileReader();

  // actually loads the specified .wav file into the StandardWaveDataContainer format
  static StandardWaveDataContainer* LoadWaveFile( const std::string& filePath );
  
  // Load .wav file from disk, reformat into standard format and store with key
  bool LoadAndStoreWaveFile( const std::string& filePath, const std::string& key );
  
  // Access audio data
  // Note: AudioWaveFileReader owns the memory for the audio data
  const StandardWaveDataContainer* GetCachedWaveDataWithKey( const std::string& key ) const;
  
  // Remove audio data from memroy
  void ClearCachedWaveDataWithKey( const std::string& key );
  
  // Remove all audio data from memory
  void ClearAllCachedWaveData();
  
  // Check if wave data is already cached
  bool IsWaveDataWithKeyCached( const std::string& key ) const;
  
  
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
    uint8_t         riff[4];                  // Chunk Id "RIFF" string
    uint32_t        chunkSize          = 0;   // Size of entire chunk
    uint8_t         wave[4];                  // Format "WAVE" string
    
    // Format "Chunk"
    uint8_t         fmt[4];                   // Chunk Id "fmt " string
    uint32_t        fmtChunkSize      = 0;    // Size of Format chunk
    AudioFormatType audioFormat       = AudioFormatType::None;  // Audio format type
    uint16_t        numberOfChannels  = 0;    // Number of audio channels
    uint32_t        samplesPerSec     = 0;    // Sample frequency in Hz
    uint32_t        bytesPerSec       = 0;    // Bytes per second
    uint16_t        blockAlign        = 0;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t        bitsPerSample     = 0;    // Number of bits per sample - Bit depth
    
    // Data "Chunk"
    uint8_t         dataChunkId[4];           // Chunk Id "data" string
    uint32_t        dataChunkSize     = 0;    // Size of Data
    
    float CalculateDuration_ms() const {
      const double numberSamples = dataChunkSize / (numberOfChannels * (bitsPerSample / 8));
      return static_cast<float>( (numberSamples / (double)samplesPerSec) * 1000.0 );
    }
    
    size_t CalculateNumberOfStandardSamples() const {
      const uint8_t numberOfBytesPerSample = (uint8_t)(bitsPerSample / 8);
      return dataChunkSize / numberOfBytesPerSample;
    }
  };

  // Audio file data held in memory
  std::unordered_map< std::string, StandardWaveDataContainer* > _cachedWaveData;
  
  // Convert PCM formatted data into 32-bit float
  // PCM is 8, 16, 24 or 32 bit signed data, little endian
  // TODO: This only works for 16-bit, need to implement 8, 24 & 32 bit data
  // Must have adequate size of source buffer to store source buffer data
  // Return number of bytes writen to standard buffer
  static size_t ConvertPCMDataStream( WaveHeader& waveHeader,
                                      unsigned char* sourceBuffer,
                                      size_t sourceBuffSize,
                                      size_t samplesPerChannel,
                                      float* out_standardBuffer );
  
};

} // AudioEngine
} // Anki


#endif /* __AnkiAudio_AudioWaveFileReader_H__ */
