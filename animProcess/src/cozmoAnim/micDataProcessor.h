/**
* File: micDataProcessor.h
*
* Author: Lee Crippen
* Created: 9/27/2017
*
* Description: Handles processing the mic samples from the robot process: combining the channels,
*              and extracting direction data.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_MicDataProcessor_H_
#define __AnimProcess_CozmoAnim_MicDataProcessor_H_

#include "audioUtil/audioDataTypes.h"

#include <array>
#include <cstdint>
#include <string>

namespace Anki {
namespace Cozmo {

class MicDataProcessor {
public:
  MicDataProcessor(const std::string& writeLocation);
  ~MicDataProcessor();
  MicDataProcessor(const MicDataProcessor& other) = delete;
  MicDataProcessor& operator=(const MicDataProcessor& other) = delete;

  static constexpr uint32_t kNumInputChannels = 4;
  static constexpr uint32_t kSamplesPerChunk = 80;
  static constexpr uint32_t kChunksPerSEBlock = 2;
  static constexpr uint32_t kSamplesPerBlock = kSamplesPerChunk * kChunksPerSEBlock;
  static constexpr uint32_t kSecondsPerFile = 20;
  static constexpr uint32_t kDefaultAudioSamplesPerFile = AudioUtil::kSampleRate_hz * kSecondsPerFile;
  static constexpr uint32_t kDefaultFilesToCapture = 15;

  void ProcessNextAudioChunk(const AudioUtil::AudioSample* audioChunk);
  
private:
  uint32_t _collectedAudioSamples = 0;
  uint32_t _audioSamplesToCollect = kDefaultAudioSamplesPerFile;
  
  uint32_t _filesToStore = kDefaultFilesToCapture;
  std::string _writeLocationDir = "";

  AudioUtil::AudioChunkList _rawAudioData{};
  AudioUtil::AudioChunkList _processedAudioData{};

  std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> _inProcessAudioBlock;
  bool _inProcessAudioBlockFirstHalf = true;

  void CollectRawAudio(const AudioUtil::AudioSample* audioChunk);
  void ProcessRawAudio(const AudioUtil::AudioSample* audioChunk);
  void ProcessExistingRawFiles(const std::string& micDataDir);
  std::string ChooseAndClearNextFileNameBase(std::string& out_deletedFileName);
  std::string GetProcessedFileNameFromRaw(const std::string& rawFileName);
  std::string GetRawFileNameFromProcessed(const std::string& rawFileName);
};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataProcessor_H_
