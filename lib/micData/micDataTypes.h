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

#ifndef __MicData_MicDataTypes_H_
#define __MicData_MicDataTypes_H_

#include <array>
#include <cstdint>

namespace Anki {
namespace Cozmo {
namespace MicData {

  enum class MicDataType {
    Raw,
    Processed
  };

  static constexpr uint32_t kNumInputChannels         = 4;
  static constexpr uint32_t kSamplesPerChunkIncoming  = 80;
  static constexpr uint32_t kSampleRateIncoming_hz    = 16000;
  static constexpr uint32_t kTimePerChunk_ms          = 5;
  static constexpr uint32_t kChunksPerSEBlock         = 2;
  static constexpr uint32_t kSamplesPerBlock          = kSamplesPerChunkIncoming * kChunksPerSEBlock;
  static constexpr uint32_t kTimePerSEBlock_ms        = kTimePerChunk_ms * kChunksPerSEBlock;
  static constexpr uint32_t kRawAudioChunkSize        = kSamplesPerChunkIncoming * kNumInputChannels;
  static constexpr uint32_t kTriggerOverlapSize_ms    = 140;
  static constexpr uint32_t kPreTriggerOverlapSize_ms = 1500;
  static constexpr uint32_t kStreamingTimeout_ms      = 6000 + kTriggerOverlapSize_ms;
  static constexpr uint32_t kRawAudioPerBuffer_ms     = 1000;

  using DirectionIndex = uint16_t;
  using DirectionConfidence = int16_t;
  
  static constexpr DirectionIndex kFirstIndex = 0;
  static constexpr DirectionIndex kLastValidIndex = 11;
  static constexpr DirectionIndex kDirectionUnknown = 12;
  static constexpr DirectionIndex kLastIndex = kDirectionUnknown;
  static constexpr size_t kNumDirections = kLastIndex - kFirstIndex + 1;
  using DirectionConfidences = std::array<float, kNumDirections>;

  struct MicDirectionData
  {
    DirectionIndex        winningDirection = 0;
    DirectionConfidence   winningConfidence = 0;
    DirectionIndex        selectedDirection = 0;
    DirectionConfidence   selectedConfidence = 0;
    DirectionConfidences  confidenceList{};
    int                   activeState = 0;
    float                 latestPowerValue = 0.f;
    float                 latestNoiseFloor = 0.f;
  };
} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __MicData_MicDataTypes_H_
