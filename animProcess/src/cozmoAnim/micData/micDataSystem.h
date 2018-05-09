/**
* File: micDataSystem.h
*
* Author: Lee Crippen
* Created: 03/26/2018
*
* Description: Handles Updates to mic data processing, streaming collection jobs, and generally acts
*              as messaging/access hub.
*
* Copyright: Anki, Inc. 2018
*
*/

#ifndef __AnimProcess_CozmoAnim_MicDataSystem_H_
#define __AnimProcess_CozmoAnim_MicDataSystem_H_

#include "micDataTypes.h"
#include "coretech/common/shared/types.h"
#include "util/global/globalDefinitions.h"

#include "clad/robotInterface/messageRobotToEngine.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Declarations
namespace Anki {
  namespace Cozmo {
    namespace CloudMic {
      class Message;
    }
    namespace MicData {
      class MicDataInfo;
      class MicDataProcessor;
    }
    namespace RobotInterface {
      struct MicData;
      struct RobotToEngine;
    }
  }
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
}
class LocalUdpServer;

namespace Anki {
namespace Cozmo {
namespace MicData {

class MicDataSystem {
public:
  MicDataSystem(Util::Data::DataPlatform* dataPlatform);
  ~MicDataSystem();
  MicDataSystem(const MicDataSystem& other) = delete;
  MicDataSystem& operator=(const MicDataSystem& other) = delete;

  void ProcessMicDataPayload(const RobotInterface::MicData& payload);
  void RecordRawAudio(uint32_t duration_ms, const std::string& path, bool runFFT);
  void RecordProcessedAudio(uint32_t duration_ms, const std::string& path);
  void StartWakeWordlessStreaming();
  void Update(BaseStationTime_t currTime_nanosec);

#if ANKI_DEV_CHEATS
  void SetForceRecordClip(bool newValue) { _forceRecordClip = newValue; }
#endif

  void ResetMicListenDirection();

  void SendMessageToEngine(std::unique_ptr<RobotInterface::RobotToEngine> msgPtr);

  void AddMicDataJob(std::shared_ptr<MicDataInfo> newJob, bool isStreamingJob = false);
  bool HasStreamingJob() const;
  std::deque<std::shared_ptr<MicDataInfo>> GetMicDataJobs() const;
  void UpdateMicJobs();
  void AudioSaveCallback(const std::string& dest);

private:
  void RecordAudioInternal(uint32_t duration_ms, const std::string& path, MicDataType type, bool runFFT);

  std::string _writeLocationDir = "";
  // Members for the the mic jobs
  std::deque<std::shared_ptr<MicDataInfo>> _micProcessingJobs;
  std::shared_ptr<MicDataInfo> _currentStreamingJob;
  mutable std::recursive_mutex _dataRecordJobMutex;
  bool _currentlyStreaming = false;
#if ANKI_DEV_CHEATS
  bool _fakeStreamingState = false;
#endif
  size_t _streamingAudioIndex = 0;

  std::unique_ptr<MicDataProcessor> _micDataProcessor;
  std::unique_ptr<LocalUdpServer> _udpServer;

#if ANKI_DEV_CHEATS
  bool _forceRecordClip = false;
#endif
  
  // Members for managing the results of async FFT processing
  struct FFTResultData {
    std::deque<std::vector<uint32_t>> _fftResultList;
    std::mutex _fftResultMutex;
  };
  std::shared_ptr<FFTResultData> _fftResultData;

  // Members for holding outgoing messages
  std::vector<std::unique_ptr<RobotInterface::RobotToEngine>> _msgsToEngine;
  std::mutex _msgsMutex;

  void ClearCurrentStreamingJob();
  float GetIncomingMicDataPercentUsed();
  void SendUdpMessage(const CloudMic::Message& msg);
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicData_MicDataSystem_H_
