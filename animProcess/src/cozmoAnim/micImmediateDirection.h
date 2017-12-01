/**
* File: micImmediateDirection.h
*
* Author: Lee Crippen
* Created: 11/09/2017
*
* Description: Holds onto immediate mic direction data with simple circular buffer
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_MicImmediateDirection_H_
#define __AnimProcess_CozmoAnim_MicImmediateDirection_H_

#include "cozmoAnim/micDataTypes.h"

#include <array>
#include <cstdint>

namespace Anki {
namespace Cozmo {
namespace MicData {

class MicImmediateDirection
{
public:
  MicImmediateDirection();

  using DirectionIndex = uint16_t;
  using DirectionConfidence = int16_t;
  
  static constexpr DirectionIndex kFirstIndex = 0;
  static constexpr DirectionIndex kDirectionUnknown = 12;
  static constexpr DirectionIndex kLastIndex = kDirectionUnknown;

  void AddDirectionSample(DirectionIndex newIndex, DirectionConfidence newConf);
  DirectionIndex GetDominantDirection() const;

private:
  static constexpr uint32_t kMicDirectionBuffer_ms = 700 + kTriggerOverlapSize_ms;
  static constexpr uint32_t kMicDirectionBufferLen = kMicDirectionBuffer_ms / 
                                                     (kTimePerChunk_ms * kChunksPerSEBlock);
  struct MicDirectionData
  {
    DirectionIndex directionIndex;
    DirectionConfidence directionConfidence;
  };
  std::array<MicDirectionData, kMicDirectionBufferLen> _micDirectionBuffer{};
  uint32_t _micDirectionBufferIndex = 0;
  std::array<uint32_t, (kLastIndex - kFirstIndex + 1)> _micDirectionsCount{};
};

} // namespace MicData
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_MicImmediateDirection_H_
