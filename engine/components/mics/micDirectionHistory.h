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

#include "coretech/common/shared/types.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/components/mics/micDirectionTypes.h"
#include "engine/robotComponents_fwd.h"

#include <array>
#include <cstdint>

namespace Anki {
namespace Cozmo {

class MicDirectionHistory
{
public:
  MicDirectionHistory() {}

  static constexpr uint32_t kMicDirectionHistory_ms = 10000;
  static constexpr uint32_t kTimePerDirectionUpdate_ms = 10;
  static constexpr uint32_t kMicDirectionHistoryLen = kMicDirectionHistory_ms / kTimePerDirectionUpdate_ms;

  void AddDirectionSample(TimeStamp_t timestamp,
                          MicDirectionIndex newIndex, MicDirectionConfidence newConf,
                          MicDirectionIndex selectedDirection);

  // Interface for requesting the "best" direction
  MicDirectionIndex GetRecentDirection(TimeStamp_t timeLength_ms = 0) const;
  MicDirectionIndex GetDirectionAtTime(TimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

  MicDirectionIndex GetSelectedDirection() const { return _mostRecentSelectedDirection; }

  MicDirectionNodeList GetRecentHistory(TimeStamp_t timeLength_ms) const;
  MicDirectionNodeList GetHistoryAtTime(TimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

private:
  std::array<MicDirectionNode, kMicDirectionHistoryLen> _micDirectionBuffer;
  uint32_t _micDirectionBufferIndex = 0;

  // direction that we're currently focusing our mics
  MicDirectionIndex _mostRecentSelectedDirection = kMicDirectionUnknown;

  using DirectionHistoryCount = std::array<uint32_t, (kLastMicDirectionIndex - kFirstMicDirectionIndex + 1)>;

  uint32_t GetNextNodeIdx(uint32_t nodeIndex) const;
  uint32_t GetPrevNodeIdx(uint32_t nodeIndex) const;
  void PrintNodeData(uint32_t index) const;

  DirectionHistoryCount GetDirectionCountAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  MicDirectionNodeList GetHistoryAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  static MicDirectionIndex GetBestDirection(const DirectionHistoryCount& directionCount);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_MicDirectionHistory_H_
