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
#include "clad/robotInterface/messageRobotToEngine.h"

#include <array>
#include <cstdint>
#include <string>

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace Anki {
namespace Cozmo {

class MicDataProcessor {
public:
  MicDataProcessor(const std::string& writeLocation);
  ~MicDataProcessor();
  MicDataProcessor(const MicDataProcessor& other) = delete;
  MicDataProcessor& operator=(const MicDataProcessor& other) = delete;

  static constexpr uint32_t kNumInputChannels = 4;
  static constexpr uint32_t kSamplesPerChunkIncoming = 120;
  static constexpr uint32_t kSampleRateIncoming_hz = 24000;
  static constexpr uint32_t kSamplesPerChunkForSE = 80;
  static constexpr uint32_t kChunksPerSEBlock = 2;
  static constexpr uint32_t kSamplesPerBlock = kSamplesPerChunkForSE * kChunksPerSEBlock;
  static constexpr uint32_t kSecondsPerFile = 20;
  static constexpr uint32_t kDefaultAudioSamplesPerFile = AudioUtil::kSampleRate_hz * kSecondsPerFile;
  static constexpr uint32_t kDefaultFilesToCapture = 15;

  using RawAudioChunk = decltype(RobotInterface::AudioInput::data);
  void ProcessNextAudioChunk(const RawAudioChunk& audioChunk);
  
private:
  uint32_t _collectedAudioSamples = 0;
  uint32_t _audioSamplesToCollect = kDefaultAudioSamplesPerFile;
  
  // uint32_t _filesToStore = kDefaultFilesToCapture;
  std::string _writeLocationDir = "";

  AudioUtil::AudioChunkList _rawAudioData{};
  AudioUtil::AudioChunkList _resampledAudioData{};
  AudioUtil::AudioChunkList _processedAudioData{};

  std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> _inProcessAudioBlock;
  bool _inProcessAudioBlockFirstHalf = true;

  SpeexResamplerState* _speexState = nullptr;

  using ResampledAudioChunk = std::array<AudioUtil::AudioSample, kSamplesPerChunkForSE * kNumInputChannels>;
  void CollectRawAudio(const RawAudioChunk& audioChunk);
  void CollectResampledAudio(const ResampledAudioChunk& audioChunk);
  void ProcessRawAudio(const ResampledAudioChunk& audioChunk);
  void ResampleAudioChunk(const RawAudioChunk&, ResampledAudioChunk& out_resmpledAudio);
  void ProcessExistingRawFiles(const std::string& micDataDir);
  std::string ChooseAndClearNextFileNameBase(std::string& out_deletedFileName);
  std::string GetProcessedFileNameFromRaw(const std::string& rawFileName);
  std::string GetRawFileNameFromProcessed(const std::string& rawFileName);
  std::string GetResampleFileNameFromProcessed(const std::string& processedName);
};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataProcessor_H_
