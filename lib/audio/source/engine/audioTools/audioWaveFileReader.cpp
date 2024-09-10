/*
 * File: audioWaveFileReader.cpp
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


#include "audioEngine/audioTools/audioWaveFileReader.h"
#include <algorithm>
#include <iostream>


namespace Anki {
namespace AudioEngine {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioWaveFileReader::~AudioWaveFileReader()
{
  ClearAllCachedWaveData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StandardWaveDataContainer* AudioWaveFileReader::LoadWaveFile( const std::string& filePath )
{
  // Open file
  std::FILE* wavFile = fopen( filePath.c_str(), "r" );
  if ( wavFile == nullptr ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Unable to open wave file: %s", filePath.c_str());
    return nullptr;
  }

  // Load header
  WaveHeader header;
  int headerSize = sizeof(header);
  size_t bytesRead = fread(&header, 1, headerSize, wavFile);
  bool success = (bytesRead == headerSize);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Failed to read Wave file header metadata");
    fclose(wavFile);
    return nullptr;
  }

  // Only read expected .wav file types
  success = memcmp(header.riff, "RIFF", 4) == 0;
  success = success && (memcmp(header.wave, "WAVE", 4) == 0);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "File is not RIFF WAVE format");
    fclose(wavFile);
    return nullptr;
  }

  // Only read .wav files where the data is directly behind the format sub-chunk
  success = (memcmp(header.dataChunkId, "data", 4) == 0);
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Data chunk is not where it's expected");
    fclose(wavFile);
    return nullptr;
  }

  // Check .wav file audio format
  success = header.audioFormat == AudioFormatType::PCM;
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Wave file is NOT PCM format");
    fclose(wavFile);
    return nullptr;
  }


  // Read file contents to buffer
  size_t sourceBufferSize = 4 * 1024; // 4 KB
  unsigned char* sourceBuffer = new (std::nothrow) unsigned char[ sourceBufferSize] ;
  success = ( sourceBuffer != nullptr );
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Failed to alloc Data Buffer!");
    fclose(wavFile);
    return nullptr;
  }

  // Create Standard Buffer to write formated data into
  StandardWaveDataContainer* standardData = new StandardWaveDataContainer( header.samplesPerSec,
                                                                          header.numberOfChannels,
                                                                          header.CalculateNumberOfStandardSamples() );
  success = standardData->HasBuffer();
  if ( !success ) {
    PRINT_NAMED_ERROR("AudioWaveFileReader.LoadWaveFile", "Failed to alloc Standard Audio Data Buffer!");
    Anki::Util::SafeDeleteArray( sourceBuffer );
    Anki::Util::SafeDelete( standardData );
    fclose(wavFile);
    return nullptr;
  }

  // Read audio data from disk
  size_t sourceDataIdx = 0;
  size_t standardDataIdx = 0;
  size_t samplesPerChannel = standardData->bufferSize / standardData->numberOfChannels;
  // Read the file in chunks
  while ( sourceDataIdx < header.dataChunkSize ) {
    // Read bytes form file
    size_t readsize = std::min( sourceBufferSize, (static_cast<size_t>(header.dataChunkSize) - sourceDataIdx) );
    bytesRead = fread(sourceBuffer, 1, readsize, wavFile);
    sourceDataIdx += bytesRead;

    // Write audio sample into Standard format
    float* standardDataPointer = (standardData->audioBuffer + standardDataIdx);
    standardDataIdx += ConvertPCMDataStream( header, sourceBuffer, bytesRead, samplesPerChannel, standardDataPointer );
  }

  DEV_ASSERT(standardDataIdx == standardData->bufferSize,
             ("Didn't store samples correctly - SampleCount " + std::to_string(standardDataIdx) +
              " | TotalSamples " + std::to_string(standardDataIdx)).c_str());

  // Clean up
  Anki::Util::SafeDeleteArray( sourceBuffer );

  return standardData;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioWaveFileReader::LoadAndStoreWaveFile( const std::string& filePath, const std::string& key )
{
  // Create Standard Buffer to write formated data into
  StandardWaveDataContainer* standardData = AudioWaveFileReader::LoadWaveFile( filePath );
  if ( standardData != nullptr )
  {
    _cachedWaveData[ key ] = standardData;
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StandardWaveDataContainer* AudioWaveFileReader::GetCachedWaveDataWithKey( const std::string& key ) const
{
  const auto it = _cachedWaveData.find( key );
  if ( it != _cachedWaveData.end() ) {
    return it->second;
  }
  
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWaveFileReader::ClearCachedWaveDataWithKey( const std::string& key )
{
  const auto it = _cachedWaveData.find( key );
  if ( it != _cachedWaveData.end() ) {
    Anki::Util::SafeDelete( it->second );
    _cachedWaveData.erase( it );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWaveFileReader::ClearAllCachedWaveData()
{
  for ( auto& anItem : _cachedWaveData ) {
    Anki::Util::SafeDelete( anItem.second );
  }
  _cachedWaveData.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t AudioWaveFileReader::ConvertPCMDataStream( WaveHeader& waveHeader,
                                                  unsigned char* sourceBuffer,
                                                  size_t sourceBuffSize,
                                                  size_t samplesPerChannel,
                                                  float* out_standardBuffer )
{
  // Convert wave data into standard format, float 32-bit  normalize into range [-1.0, 1.0]
  DEV_ASSERT(waveHeader.bitsPerSample == 16, "Only read signed 16-bit .wav files");
  DEV_ASSERT(waveHeader.numberOfChannels == 1, "Only read single channel .wav files");
  
  uint bytesPerSample  = waveHeader.bitsPerSample / 8;
  const size_t sampleCount = sourceBuffSize / bytesPerSample;
  
  // Convert 2 bytes into uint16
  int16_t* unpackedValues = reinterpret_cast<int16_t*>(sourceBuffer);
  // Write into out buffer
  for (size_t sampleIdx = 0; sampleIdx < sampleCount; ++sampleIdx) {
    out_standardBuffer[sampleIdx] = unpackedValues[sampleIdx]/(float)(INT16_MAX);
  }
  
  return sampleCount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioWaveFileReader::IsWaveDataWithKeyCached( const std::string& key ) const
{
  const auto it = _cachedWaveData.find( key );
  return ( it != _cachedWaveData.end() );
}


} // AudioEngine
} // Anki
