/**
* File: micDirectionHistory.h
*
* Author: Lee Crippen
* Created: 11/14/2017
*
* Description: Holds onto microphone direction history
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __Engine_MicDirectionHistory_H_
#define __Engine_MicDirectionHistory_H_

#include "anki/common/types.h"

#include <array>
#include <cstdint>
#include <deque>

namespace Anki {
namespace Cozmo {

class MicDirectionHistory
{
public:
  using DirectionIndex = uint16_t;
  using DirectionConfidence = int16_t;

  static constexpr uint32_t kMicDirectionHistory_ms = 10000;
  static constexpr uint32_t kTimePerDirectionUpdate_ms = 10;
  static constexpr uint32_t kMicDirectionHistoryLen = kMicDirectionHistory_ms / kTimePerDirectionUpdate_ms;
  
  static constexpr DirectionIndex kFirstIndex = 0;
  static constexpr DirectionIndex kDirectionUnknown = 12;
  static constexpr DirectionIndex kLastIndex = kDirectionUnknown;

  void AddDirectionSample(TimeStamp_t timestamp, DirectionIndex newIndex, DirectionConfidence newConf);

  // Interface for requesting the "best" direction
  DirectionIndex GetRecentDirection(TimeStamp_t timeLength_ms = 0) const;
  DirectionIndex GetDirectionAtTime(TimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

  // Interface for requesting a copy of direction history
  struct DirectionNode
  {
    TimeStamp_t         timestampEnd    = 0;
    DirectionIndex      directionIndex  = kDirectionUnknown;
    DirectionConfidence confidenceAvg   = 0;
    DirectionConfidence confidenceMax   = 0;
    TimeStamp_t         timestampAtMax  = 0;
    uint32_t            count           = 0;

    bool IsValid() const { return count > 0; }
  };
  using NodeList = std::deque<DirectionNode>;

  NodeList GetRecentHistory(TimeStamp_t timeLength_ms) const;
  NodeList GetHistoryAtTime(TimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

private:
  std::array<DirectionNode, kMicDirectionHistoryLen> _micDirectionBuffer;
  uint32_t _micDirectionBufferIndex = 0;

  using DirectionHistoryCount = std::array<uint32_t, (kLastIndex - kFirstIndex + 1)>;

  uint32_t GetNextNodeIdx(uint32_t nodeIndex) const;
  uint32_t GetPrevNodeIdx(uint32_t nodeIndex) const;
  void PrintNodeData(uint32_t index) const;

  DirectionHistoryCount GetDirectionCountAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  NodeList GetHistoryAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  static DirectionIndex GetBestDirection(const DirectionHistoryCount& directionCount);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_MicDirectionHistory_H_
