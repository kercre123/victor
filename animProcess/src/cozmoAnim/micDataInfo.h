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

#include "cozmoAnim/micDataTypes.h"
#include "util/bitFlags/bitFlags.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Anki {
namespace Cozmo {
namespace MicData {

class MicDataInfo
{
public:
  Util::BitFlags8<MicDataType>  _typesToRecord;
  uint32_t                      _timeRecorded_ms  = 0;
  uint32_t                      _timeToRecord_ms  = 0;
  bool                          _doFFTProcess     = false;
  bool                          _repeating        = false;
  std::string                   _writeLocationDir;
  std::string                   _writeNameBase;
  
  AudioUtil::AudioChunkList _rawAudioData{};
  AudioUtil::AudioChunkList _resampledAudioData{};
  AudioUtil::AudioChunkList _processedAudioData{};
  
  // Note this will be called from a separate processing thread
  std::function<void(std::vector<uint32_t>&&)> _rawAudioFFTCallback;
  
  void CollectRawAudio(const AudioUtil::AudioChunk& audioChunk);
  void CollectResampledAudio(const AudioUtil::AudioChunk& audioChunk);
  void CollectProcessedAudio(const AudioUtil::AudioChunk& audioChunk);
  bool CheckDone();
  
private:
  void SaveCollectedAudio(const std::string& dataDirectory, const std::string& nameToUse, const std::string& nameToRemove);
  std::string ChooseNextFileNameBase(std::string& out_dirToDelete);
  
  static std::vector<uint32_t> GetFFTResultFromRaw(const AudioUtil::AudioChunkList& data, float length_s);
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataInfo_H_
