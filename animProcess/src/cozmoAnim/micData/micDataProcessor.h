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

#include "svad.h"

#include "micDataTypes.h"
#include "coretech/common/shared/types.h"
#include "audioUtil/audioDataTypes.h"
#include "util/container/fixedCircularBuffer.h"
#include "util/global/globalDefinitions.h"

#include <array>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

// Declarations
namespace Anki {
  namespace Cozmo {
    class BeatDetector;
    namespace MicData {
      class MicDataSystem;
      class MicImmediateDirection;
    }
    namespace RobotInterface {
      struct MicData;
    }
    class SpeechRecognizerTHF;
  }
}

namespace Anki {
namespace Cozmo {
namespace MicData {

class MicDataProcessor {
public:
  MicDataProcessor(MicDataSystem* micDataSystem, const std::string& writeLocation, const std::string& triggerWordDataDir);
  ~MicDataProcessor();
  MicDataProcessor(const MicDataProcessor& other) = delete;
  MicDataProcessor& operator=(const MicDataProcessor& other) = delete;

  void ProcessMicDataPayload(const RobotInterface::MicData& payload);
  void RecordRawAudio(uint32_t duration_ms, const std::string& path, bool runFFT);

  void ResetMicListenDirection();
  float GetIncomingMicDataPercentUsed();

  BeatDetector& GetBeatDetector() { assert(nullptr != _beatDetector); return *_beatDetector.get(); }
  
private:
  MicDataSystem* _micDataSystem = nullptr;
  std::string _writeLocationDir = "";
  // Members for caching off lookup indices for mic processing results
  int _bestSearchBeamIndex = 0;
  int _bestSearchBeamConfidence = 0;
  int _selectedSearchBeamIndex = 0;
  int _selectedSearchBeamConfidence = 0;
  int _searchConfidenceState = 0;

  // Members for general purpose processing and state
  std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> _inProcessAudioBlock;
  bool _inProcessAudioBlockFirstHalf = true;
  std::unique_ptr<SpeechRecognizerTHF> _recognizer;
  std::unique_ptr<SVadConfig_t> _sVadConfig;
  std::unique_ptr<SVadObject_t> _sVadObject;
  uint32_t _vadCountdown = 0;
  std::unique_ptr<MicImmediateDirection> _micImmediateDirection;

  static constexpr uint32_t kRawAudioBufferSize = kRawAudioPerBuffer_ms / kTimePerChunk_ms;
  float _rawAudioBufferFullness[2] = { 0.f, 0.f };
  // We have 2 fixed buffers for incoming raw audio that we alternate between, so that the processing thread can work
  // on one set of data while the main thread can copy new data into the other set.
  Util::FixedCircularBuffer<RobotInterface::MicData, kRawAudioBufferSize> _rawAudioBuffers[2];
  // Index of the buffer that is currently being used by the processing thread
  uint32_t _rawAudioProcessingIndex = 0;
  std::thread _processThread;
  std::thread _processTriggerThread;
  std::mutex _rawMicDataMutex;
  bool _processThreadStop = false;
  bool _robotWasMoving = false;

  // Internal buffer used to add to the streaming audio once a trigger is detected
  static constexpr uint32_t kImmediateBufferSize = kTriggerOverlapSize_ms / kTimePerSEBlock_ms;
  struct TimedMicData {
    std::array<AudioUtil::AudioSample, kSamplesPerBlock> audioBlock;
    TimeStamp_t timestamp;
  };
  Util::FixedCircularBuffer<TimedMicData, kImmediateBufferSize> _immediateAudioBuffer;

  using RawAudioChunk = std::array<AudioUtil::AudioSample, kRawAudioChunkSize>;
  static constexpr uint32_t kImmediateBufferRawSize = kPreTriggerOverlapSize_ms / kTimePerChunk_ms;
  Util::FixedCircularBuffer<RawAudioChunk, kImmediateBufferRawSize> _immediateAudioBufferRaw;
  
  std::mutex _procAudioXferMutex;
  std::condition_variable _dataReadyCondition;
  std::condition_variable _xferAvailableCondition;
  size_t _procAudioRawComplete = 0;
  size_t _procAudioXferCount = 0;

  // Mutex for different accessing signal essence software
  std::mutex _seInteractMutex;

  // Aubio beat detector
  std::unique_ptr<BeatDetector> _beatDetector;
  
  void InitVAD();
  void TriggerWordDetectCallback(const char* resultFound, float score);
  void ProcessRawAudio(TimeStamp_t timestamp,
                       const AudioUtil::AudioSample* audioChunk,
                       uint32_t robotStatus,
                       float robotAngle);

  MicDirectionData ProcessMicrophonesSE(const AudioUtil::AudioSample* audioChunk,
                                        AudioUtil::AudioSample* bufferOut,
                                        uint32_t robotStatus,
                                        float robotAngle);

  void ProcessRawLoop();
  void ProcessTriggerLoop();
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataProcessor_H_
