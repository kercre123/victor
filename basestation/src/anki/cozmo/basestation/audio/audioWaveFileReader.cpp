//
//  audioWaveFileReader.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 3/23/16.
//
//

#include "anki/cozmo/basestation/audio/audioWaveFileReader.h"


namespace Anki {
namespace Cozmo {
namespace Audio {

AudioWaveFileReader::~AudioWaveFileReader()
{
  Util::SafeDelete(_currentWaveData);
}
  

bool AudioWaveFileReader::LoadWaveFile(const std::string& filePath)
{
  // Open file
  std::FILE* wavFile = fopen( filePath.c_str(), "r" );
  
  if ( wavFile == nullptr ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Unable to open wave file: %s", filePath.c_str());
    return false;
  }
  
  // Crate Data Container
  WaveDataContainer* waveDataContainer = new WaveDataContainer();
  
  // Load header
  int headerSize = sizeof(waveDataContainer->Header);
  size_t bytesRead = fread(&waveDataContainer->Header, 1, headerSize, wavFile);
  bool success = (bytesRead == headerSize);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Failed to read Wave file header metadata");
    Util::SafeDelete(waveDataContainer);
    fclose(wavFile);
    return false;
  }
  
  // Only read expected .wav file types
  success = memcmp(waveDataContainer->Header.Riff, "RIFF", 4) == 0;
  success = success && (memcmp(waveDataContainer->Header.Riff, "RIFF", 4) == 0);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "File is not RIFF WAVE format");
    Util::SafeDelete(waveDataContainer);
    fclose(wavFile);
    return false;
  }
  // Check .wav file audio format
  success = waveDataContainer->Header.AudioFormat == AudioFormatType::PCM;
  success = success && (waveDataContainer->Header.BitsPerSample == 16);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Wave file is NOT PCM 16-bit format");
    Util::SafeDelete(waveDataContainer);
    fclose(wavFile);
    return false;
  }
  
  // Read file contents to buffer
  success = waveDataContainer->CreateDataBuffer();
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Failed to alloc Data Buffer!");
    Util::SafeDelete(waveDataContainer);
    fclose(wavFile);
    return false;
  }
  
  // Check that all the audio data has been writen into a buffer
  bytesRead = fread(waveDataContainer->DataBuffer, 1, waveDataContainer->Header.DataChunkSize, wavFile);
  success = (bytesRead == waveDataContainer->Header.DataChunkSize);
  success = success && (fgetc(wavFile) == EOF);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Failed to write file into Audio Buffer!");
    Util::SafeDelete(waveDataContainer);
    fclose(wavFile);
    return false;
  }
  
  
  fclose(wavFile);
  
  ASSERT_NAMED(_currentWaveData == nullptr, "Must delete _currentWaveData before loading a new file");
  _currentWaveData = waveDataContainer;
  
  return true;
}


} // Audio
} // Cozmo
} // Anki