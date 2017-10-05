/**
* File: waveFile.cpp
*
* Author: Lee Crippen
* Created: 07/03/17
*
* Description: Simple wave file saving functionality.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "audioUtil/waveFile.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#include <array>
#include <cstdint>

namespace Anki {
namespace AudioUtil {

template<typename T>
static void PackData(T data, std::vector<uint8_t>& dataOut)
{
  const auto* dataBegin = reinterpret_cast<const uint8_t*>(&data);
  const auto* dataEnd = reinterpret_cast<const uint8_t*>(&data) + sizeof(T);
  dataOut.insert(dataOut.end(), dataBegin, dataEnd);
}

struct WaveFileHeaderData
{
  static const std::array<uint8_t, 4> _chunkID;
  static const std::array<uint8_t, 4> _format;
  
  static const std::array<uint8_t, 4> _subchunk1ID;
  static constexpr uint32_t           _subchunk1Size = 16; // For PCM this omits a couple optional pieces
  static constexpr uint16_t           _audioFormat = 1;
  static constexpr uint32_t           _sampleRate = kSampleRate_hz;
  
  static constexpr uint16_t           _bitsPerSample = 16; // This is out of order in header for reuse
  
  static const std::array<uint8_t, 4> _subchunk2ID;
  static constexpr uint32_t           _headerDataSize = _chunkID.size() + sizeof(uint32_t) + _format.size() +
                                                        _subchunk1ID.size() + sizeof(_subchunk1Size) + _subchunk1Size +
                                                        _subchunk2ID.size() + sizeof(uint32_t); // Should be 44
  
  static constexpr std::size_t _numChannelsOffset = 22;
  
  static std::vector<uint8_t> GetHeaderData(uint32_t numSamples, uint16_t numChannels)
  {
    const auto& subchunk2Size = Util::numeric_cast<uint32_t>(numSamples * numChannels * (_bitsPerSample / 8));
    
    const auto& chunkSize = Util::numeric_cast<uint32_t>(_format.size() +
                                                         (_subchunk1ID.size() + sizeof(_subchunk1Size) + _subchunk1Size) +
                                                         (_subchunk2ID.size() + sizeof(subchunk2Size) + subchunk2Size));
    
    DEV_ASSERT(_headerDataSize == 44, "WaveFileHeaderData.Size");
    std::vector<uint8_t> headerData;
    
    // ChunkID
    headerData.insert(headerData.end(), _chunkID.begin(), _chunkID.end());
    
    // ChunkSize
    PackData(chunkSize, headerData);
    
    // Format
    headerData.insert(headerData.end(), _format.begin(), _format.end());
    
    // Subchunk1ID
    headerData.insert(headerData.end(), _subchunk1ID.begin(), _subchunk1ID.end());
    
    // Subchunk1Size
    PackData(_subchunk1Size, headerData);
    
    // AudioFormat
    PackData(_audioFormat, headerData);
    
    // NumChannels
    PackData(numChannels, headerData);
    
    // SampleRate
    PackData(_sampleRate, headerData);
    
    // ByteRate
    const auto& byteRate = Util::numeric_cast<uint32_t>(_sampleRate * numChannels * (_bitsPerSample / 8));
    PackData(byteRate, headerData);
    
    // BlockAlign
    const auto& blockAlign = Util::numeric_cast<uint16_t>(numChannels * (_bitsPerSample / 8));
    PackData(blockAlign, headerData);
    
    // BitsPerSample
    PackData(_bitsPerSample, headerData);
    
    // Subchunk2ID
    headerData.insert(headerData.end(), _subchunk2ID.begin(), _subchunk2ID.end());
    
    // Subchunk2Size
    PackData(subchunk2Size, headerData);
    
    if (!ANKI_VERIFY(headerData.size() == _headerDataSize,
                "WaveFileHeaderData.GetHeaderData",
                "Expected size: %d Got size: %zu",
                _headerDataSize, headerData.size()))
    {
      return std::vector<uint8_t>();
    }
    
    return headerData;
  }
};

// In C++17 These could even be defined as constexpr, but we're only on C++14 currently
const std::array<uint8_t, 4> WaveFileHeaderData::_chunkID = {{'R', 'I', 'F', 'F'}};
const std::array<uint8_t, 4> WaveFileHeaderData::_format{ {'W', 'A', 'V', 'E'} };
const std::array<uint8_t, 4> WaveFileHeaderData::_subchunk1ID{ {'f', 'm', 't', ' '} };
const std::array<uint8_t, 4> WaveFileHeaderData::_subchunk2ID{ {'d', 'a', 't', 'a'} };

bool WaveFile::SaveFile(const std::string& filename, const AudioChunkList& chunkList, uint16_t numChannels)
{
  if (numChannels == 0)
  {
    // just no
    DEV_ASSERT(numChannels != 0, "WaveFile.SaveFile Can't save 0 channels.");
    return false;
  }
  
  uint32_t numSamples = 0;
  for (const auto& chunk : chunkList)
  {
    numSamples = Util::numeric_cast<uint32_t>(numSamples + chunk.size());
  }
  numSamples /= numChannels;
  std::vector<uint8_t> dataToWrite = WaveFileHeaderData::GetHeaderData(numSamples, numChannels);
  
  // Put a max here in case we have a *lot* of data we're saving, so that we write it out in pieces rather than use up more memory
  constexpr uint32_t kByteBufferMaxSize = 1024 * 512;
  bool isFirstWrite = true;
  for (const auto& chunk : chunkList)
  {
    auto nextChunkByteSize = chunk.size() * sizeof(AudioSample);
    if (dataToWrite.size() > 0 &&
        dataToWrite.size() + nextChunkByteSize > kByteBufferMaxSize)
    {
      bool shouldAppend = !isFirstWrite;
      if (!Util::FileUtils::WriteFile(filename, dataToWrite, shouldAppend))
      {
        return false;
      }
      
      isFirstWrite = false;
      dataToWrite.clear();
    }
    const auto* castedChunkBegin = reinterpret_cast<const uint8_t *>(&*chunk.begin());
    const auto* castedChunkEnd = reinterpret_cast<const uint8_t *>(&*chunk.end());
    
    dataToWrite.insert(dataToWrite.end(), castedChunkBegin, castedChunkEnd);
  }
  
  bool shouldAppend = !isFirstWrite;
  return Util::FileUtils::WriteFile(filename, dataToWrite, shouldAppend);
}

AudioChunkList WaveFile::ReadFile(const std::string& filename, std::size_t desiredSamplesPerChunk)
{
  auto binaryData = Util::FileUtils::ReadFileAsBinary(filename);
  
  if (binaryData.size() <= WaveFileHeaderData::_headerDataSize)
  {
    PRINT_NAMED_WARNING("WaveFile.ReadFile.HeaderDataMissing", "File only %zu bytes", binaryData.size());
    return AudioChunkList();
  }
  
  uint16_t numChannels = *reinterpret_cast<uint16_t*>(binaryData.data() + WaveFileHeaderData::_numChannelsOffset);
  if (numChannels == 0)
  {
    PRINT_NAMED_WARNING("WaveFile.ReadFile.NoChannels", "Wave file header claims 0 channels. Aborting");
    return AudioChunkList();
  }
  
  AudioChunkList audioData;
  const auto* endDataMarker = binaryData.data() + binaryData.size();
  auto* currentData = binaryData.data() + WaveFileHeaderData::_headerDataSize;
  const auto bytesPerChunk = desiredSamplesPerChunk * numChannels * sizeof(AudioSample);
  while(currentData + bytesPerChunk <= endDataMarker)
  {
    AudioChunk nextChunk;
    nextChunk.resize(desiredSamplesPerChunk * numChannels);
    const auto* dataStart = reinterpret_cast<AudioSample*>(currentData);
    std::copy(dataStart, dataStart + (desiredSamplesPerChunk * numChannels), nextChunk.data());
    audioData.push_back(std::move(nextChunk));
    currentData += bytesPerChunk;
  }
  
  if (currentData < endDataMarker && ((endDataMarker - currentData) % sizeof(AudioSample)) != 0)
  {
    auto extraBytesLen = (endDataMarker - currentData) % sizeof(AudioSample);
    endDataMarker -= extraBytesLen;
    PRINT_NAMED_WARNING("WaveFile.ReadFile.NotEvenlyDivisible", "Clipping %zu bytes", extraBytesLen);
  }
  
  if (currentData < endDataMarker)
  {
    auto extraSamples = (endDataMarker - currentData) / sizeof(AudioSample);
    AudioChunk nextChunk;
    nextChunk.resize(extraSamples);
    const auto* dataStart = reinterpret_cast<AudioSample*>(currentData);
    std::copy(dataStart, dataStart + extraSamples, nextChunk.data());
    audioData.push_back(std::move(nextChunk));
    PRINT_NAMED_WARNING("WaveFile.ReadFile.SmallFinalChunk", "Final chunk only %zu samples", extraSamples);
  }
  
  return audioData;
}

} // namespace AudioUtil
} // namespace Anki
