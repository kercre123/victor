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

#include "anki/common/types.h"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
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
  bool _forceRecordClip = false;

  // Members for managing the incoming raw audio jobs
  struct TimedMicData {
    TimeStamp_t timestamp;
    AudioUtil::AudioChunk audioChunk;
  };
  std::deque<TimedMicData> _rawAudioToProcess;
  std::thread _processThread;
  std::mutex _resampleMutex;
  bool _processThreadStop = false;
  
  // Members for managing the results of async FFT processing
  std::deque<std::vector<uint32_t>> _fftResultList;
  std::mutex _fftResultMutex;

  // Internal buffer used to add to the streaming audio once a trigger is detected
  std::deque<TimedMicData> _triggerOverlapBuffer;

  // Members for holding outgoing messages
  std::vector<std::unique_ptr<RobotInterface::RobotToEngine>> _msgsToEngine;
  std::mutex _msgsMutex;

  AudioUtil::AudioChunk ProcessResampledAudio(TimeStamp_t timestamp, const AudioUtil::AudioChunk& audioChunk);
  AudioUtil::AudioChunk ResampleAudioChunk(const AudioUtil::AudioChunk& audioChunk);

  void ProcessLoop();
  void ClearCurrentStreamingJob();
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataProcessor_H_
