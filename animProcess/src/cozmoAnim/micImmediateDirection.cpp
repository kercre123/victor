/**
* File: micImmediateDirection.cpp
*
* Author: Lee Crippen
* Created: 11/09/2017
*
* Description: Holds onto immediate mic direction data with simple circular buffer
*
* Copyright: Anki, Inc. 2017
*
*/
  
#include "cozmoAnim/micImmediateDirection.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace MicData {

MicImmediateDirection::MicImmediateDirection()
{
  // Fill the historical array with the unknown direction to start
  const auto initialDirection = kDirectionUnknown;
  MicDirectionData initialData = 
  {
    .directionIndex = initialDirection,
    .directionConfidence = 0
  };
  _micDirectionBuffer.fill(initialData);

  // The full count will decrease and go away as other directions come in that are not "unknown"
  _micDirectionsCount[initialDirection] = kMicDirectionBufferLen;
}

void MicImmediateDirection::AddDirectionSample(MicImmediateDirection::DirectionIndex newIndex, DirectionConfidence newConf)
{
  // Decrement the count for the direction of the existing oldest sample we'll be replacing
  auto& directionDataEntry = _micDirectionBuffer[_micDirectionBufferIndex];
  auto& countRef = _micDirectionsCount[directionDataEntry.directionIndex];
  if (ANKI_VERIFY(
    countRef > 0,
    "MicImmediateDirection.AddDirectionSample",
    "Trying to replace a direction sample in index %d but count is 0",
    directionDataEntry.directionIndex))
  {
    --countRef;
  }

  // Replace the data in the oldest sample with our new sample and update the count on that direction
  directionDataEntry.directionIndex = newIndex;
  directionDataEntry.directionConfidence = newConf;
  ++_micDirectionsCount[newIndex];

  // Update our index to the next oldest sample
  _micDirectionBufferIndex = (_micDirectionBufferIndex + 1) % kMicDirectionBufferLen;
}

MicImmediateDirection::DirectionIndex MicImmediateDirection::GetDominantDirection() const
{
  // Loop through our stored direction counts and pick the direction with the higest count
  // Does not currently consider confidence level
  auto bestIndex = kDirectionUnknown;
  uint32_t bestCount = 0;
  for (auto i = kFirstIndex; i <= kLastIndex; ++i)
  {
    if (_micDirectionsCount[i] > bestCount)
    {
      bestCount = _micDirectionsCount[i];
      bestIndex = i;
    }
  }

  return bestIndex;
}

} // namespace MicData
} // namespace Cozmo
} // namespace Anki
