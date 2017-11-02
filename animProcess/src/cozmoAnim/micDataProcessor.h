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

#include "cozmoAnim/micDataTypes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace Anki {
namespace Cozmo {
namespace MicData {
  
class MicDataInfo;

class MicDataProcessor {
public:
  MicDataProcessor(const std::string& writeLocation);
  ~MicDataProcessor();
  MicDataProcessor(const MicDataProcessor& other) = delete;
  MicDataProcessor& operator=(const MicDataProcessor& other) = delete;

  void ProcessNextAudioChunk(const RawAudioChunk& audioChunk);
  void RecordRawAudio(uint32_t duration_ms, const std::string& path, bool runFFT);
  void Update();

private:
  std::string _writeLocationDir = "";

  std::deque<std::shared_ptr<MicDataInfo>> _micProcessingJobs;
  std::mutex _dataRecordJobMutex;

  std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> _inProcessAudioBlock;
  bool _inProcessAudioBlockFirstHalf = true;

  SpeexResamplerState* _speexState = nullptr;

  AudioUtil::AudioChunkList _rawAudioToProcess;

  std::thread _processThread;
  std::mutex _resampleMutex;
  bool _processThreadStop = false;
  
  std::deque<std::vector<uint32_t>> _fftResultList;
  std::mutex _fftResultMutex;

  AudioUtil::AudioChunk ProcessResampledAudio(const AudioUtil::AudioChunk& audioChunk);
  AudioUtil::AudioChunk ResampleAudioChunk(const AudioUtil::AudioChunk& audioChunk);

  void ProcessLoop();
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataProcessor_H_
