/**
* File: micDataTypes.h
*
* Author: Lee Crippen
* Created: 10/25/2017
*
* Description: Holds types associated with mic data processing.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_MicDataTypes_H_
#define __AnimProcess_CozmoAnim_MicDataTypes_H_

#include "audioUtil/audioDataTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include <cstdint>

namespace Anki {
namespace Cozmo {
namespace MicData {

  using RawAudioChunk = decltype(RobotInterface::MicData::data);

  enum class MicDataType {
    Raw,
    Resampled,
    Processed
  };

  static constexpr uint32_t kNumInputChannels         = 4;
  static constexpr uint32_t kSamplesPerChunkIncoming  = 120;
  static constexpr uint32_t kSampleRateIncoming_hz    = 24000;
  static constexpr uint32_t kSamplesPerChunkForSE     = 80;
  static constexpr uint32_t kTimePerChunk_ms          = 5;
  static constexpr uint32_t kChunksPerSEBlock         = 2;
  static constexpr uint32_t kSamplesPerBlock          = kSamplesPerChunkForSE * kChunksPerSEBlock;
  static constexpr uint32_t kSecondsPerFile           = 20;
  static constexpr uint32_t kDefaultFilesToCapture    = 15;
  static constexpr uint32_t kRawAudioChunkSize        = kSamplesPerChunkIncoming * kNumInputChannels;
  static constexpr uint32_t kResampledAudioChunkSize  = kSamplesPerChunkForSE * kNumInputChannels;
  static constexpr uint32_t kTriggerOverlapSize_ms    = 140;
  static constexpr uint32_t kMinAudioSizeToSave_ms    = kTriggerOverlapSize_ms + 100;
  
} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicDataTypes_H_
