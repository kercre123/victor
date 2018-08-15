/**
* File: micDataInfo.h
*
* Author: Lee Crippen
* Created: 10/25/2017
*
* Description: Holds onto info related to recording and processing mic data.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_MicDataInfo_H_
#define __AnimProcess_CozmoAnim_MicDataInfo_H_

#include "micDataTypes.h"
#include "audioUtil/audioDataTypes.h"
#include "clad/cloud/mic.h"
#include "util/bitFlags/bitFlags.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace Anki {
namespace Vector {
namespace MicData {

class MicDataInfo
{
public:
  static constexpr uint32_t kDefaultFilesToCapture = 15;
  static constexpr uint32_t kMinAudioSizeToSave_ms = kTriggerOverlapSize_ms + 100;

  bool                          _doFFTProcess     = false;
  bool                          _repeating        = false;
  uint32_t                      _numMaxFiles      = kDefaultFilesToCapture;
  CloudMic::StreamType          _type             = CloudMic::StreamType::Normal;
  std::string                   _writeLocationDir;
  std::string                   _writeNameBase;

  static constexpr uint32_t kMaxRecordTime_ms = std::numeric_limits<uint32_t>::max();
  
  // Note this will be called from a separate processing thread
  std::function<void(std::vector<uint32_t>&&)> _rawAudioFFTCallback;
  
  // Callback for when audio is written to disk
  std::function<void(const std::string&)> _audioSaveCallback;

  void SetTimeToRecord(uint32_t timeToRecord);
  void CollectRawAudio(const AudioUtil::AudioSample* audioChunk, size_t size);
  void CollectProcessedAudio(const AudioUtil::AudioSample* audioChunk, size_t size);

  AudioUtil::AudioChunkList GetProcessedAudio(size_t beginIndex);
  void UpdateForNextChunk();
  bool CheckDone() const;
  uint32_t GetTimeToRecord_ms() const;
  uint32_t GetTimeRecorded_ms() const;

  void EnableDataCollect(MicDataType type, bool saveToFile);
  void DisableDataCollect(MicDataType type);
  
private:
  Util::BitFlags8<MicDataType>  _typesToCollect;
  Util::BitFlags8<MicDataType>  _typesToSave;
  // These members are accessed via multiple threads when the job is running, so they use a mutex
  uint32_t _timeRecorded_ms  = 0;
  uint32_t _timeToRecord_ms  = 0;
  AudioUtil::AudioChunkList _rawAudioData{};
  AudioUtil::AudioChunkList _processedAudioData{};
  mutable std::mutex _dataMutex;

  void SaveCollectedAudio(const std::string& dataDirectory, const std::string& nameToUse, const std::string& nameToRemove);
  std::string ChooseNextFileNameBase(std::string& out_dirToDelete);
  
  static std::vector<uint32_t> GetFFTResultFromRaw(const AudioUtil::AudioChunkList& data, float length_s);
};

} // namespace MicData
} // namespace Vector
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataInfo_H_
