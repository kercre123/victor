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
#include "cozmoAnim/beatDetector.h"

#include "util/container/fixedCircularBuffer.h"

#include "coretech/common/shared/types.h"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

class UdpServer;

namespace Anki {
namespace Cozmo {

namespace RobotInterface {
  struct MicData;
  struct RobotToEngine;
}

class SpeechRecognizerTHF;

namespace MicData {
  
class MicDataInfo;
class MicImmediateDirection;

class MicDataProcessor {
public:
  MicDataProcessor(const std::string& writeLocation, const std::string& triggerWordDataDir);
  ~MicDataProcessor();
  MicDataProcessor(const MicDataProcessor& other) = delete;
  MicDataProcessor& operator=(const MicDataProcessor& other) = delete;

  void ProcessMicDataPayload(const RobotInterface::MicData& payload);
  void RecordRawAudio(uint32_t duration_ms, const std::string& path, bool runFFT);
  void Update();

  void SetForceRecordClip(bool newValue) { _forceRecordClip = newValue; }

private:
  std::string _writeLocationDir = "";
  // Members for caching off lookup indices for mic processing results
  int _bestSearchBeamIndex = 0;
  int _bestSearchBeamConfidence = 0;
  int _searchConfidenceState = 0;

  // Members for the the mic processing/recording/streaming jobs
  std::deque<std::shared_ptr<MicDataInfo>> _micProcessingJobs;
  std::shared_ptr<MicDataInfo> _currentStreamingJob;
  std::recursive_mutex _dataRecordJobMutex;
  bool _currentlyStreaming = false;

  // Members for general purpose processing and state
  std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> _inProcessAudioBlock;
  bool _inProcessAudioBlockFirstHalf = true;
  SpeexResamplerState* _speexState = nullptr;
  std::unique_ptr<SpeechRecognizerTHF> _recognizer;
  std::unique_ptr<UdpServer> _udpServer;
  std::unique_ptr<MicImmediateDirection> _micImmediateDirection;
  bool _forceRecordClip = true;

  // Members for managing the incoming raw audio jobs
  struct TimedRawMicData {
    std::array<AudioUtil::AudioSample, kRawAudioChunkSize> audioChunk;
    TimeStamp_t timestamp;
  };
  static constexpr uint32_t kRawAudioPerBuffer_ms = 2000;
  static constexpr uint32_t kRawAudioBufferSize = kRawAudioPerBuffer_ms / (kChunksPerSEBlock * kTimePerChunk_ms);
  // We have 2 fixed buffers for incoming raw audio that we alternate between, so that the processing thread can work
  // on one set of data while the main thread can copy new data into the other set.
  Util::FixedCircularBuffer<TimedRawMicData, kRawAudioBufferSize> _rawAudioBuffers[2];
  // Index of the buffer that is currently being used by the processing thread
  uint32_t _rawAudioProcessingIndex = 0;
  std::thread _processThread;
  std::mutex _resampleMutex;
  bool _processThreadStop = false;
  
  // Members for managing the results of async FFT processing
  std::deque<std::vector<uint32_t>> _fftResultList;
  std::mutex _fftResultMutex;

  // Internal buffer used to add to the streaming audio once a trigger is detected
  static constexpr uint32_t kImmediateBufferSize = kTriggerOverlapSize_ms / (kChunksPerSEBlock * kTimePerChunk_ms);
  struct TimedMicData {
    std::array<AudioUtil::AudioSample, kSamplesPerBlock> audioBlock;
    TimeStamp_t timestamp;
  };
  Util::FixedCircularBuffer<TimedMicData, kImmediateBufferSize> _immediateAudioBuffer;

  // Members for holding outgoing messages
  std::vector<std::unique_ptr<RobotInterface::RobotToEngine>> _msgsToEngine;
  std::mutex _msgsMutex;

  void TriggerWordDetectCallback(const char* resultFound, float score);
  bool ProcessResampledAudio(TimeStamp_t timestamp, const AudioUtil::AudioSample* audioChunk);

  MicDirectionData ProcessMicrophonesSE(const AudioUtil::AudioSample* audioChunk,
                                     AudioUtil::AudioSample* bufferOut) const;

  void ResampleAudioChunk(const AudioUtil::AudioSample* audioChunk, AudioUtil::AudioSample* bufferOut);

  void ProcessLoop();
  void ClearCurrentStreamingJob();
  
  // Beat/tempo detection:
  BeatDetector _beatDetector;
  
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataProcessor_H_
