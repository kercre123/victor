/**
* File: micDirectionHistory.cpp
*
* Author: Lee Crippen
* Created: 11/14/2017
*
* Description: Holds onto microphone direction history
*
* Copyright: Anki, Inc. 2017
*
*/

#include "engine/micDirectionHistory.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
namespace Cozmo {

static_assert(std::is_same<decltype(RobotInterface::MicDirection::timestamp), TimeStamp_t>::value,
              "Expecting type of MicDirection::timestamp to match TimeStamp_t");
static_assert(std::is_same<decltype(RobotInterface::MicDirection::direction),
                           MicDirectionHistory::DirectionIndex>::value,
              "Expecting type of MicDirection::direction to match MicDirectionHistory::DirectionIndex");
static_assert(std::is_same<decltype(RobotInterface::MicDirection::confidence),
                           MicDirectionHistory::DirectionConfidence>::value,
              "Expecting type of MicDirection::confidence to match MicDirectionHistory::DirectionConfidence");

void MicDirectionHistory::PrintNodeData(uint32_t index) const
{
  const MicDirectionHistory::DirectionNode& node = _micDirectionBuffer[GetPrevNodeIdx(GetNextNodeIdx(index))];
  PRINT_NAMED_INFO("MicDirectionHistory::PrintNodeData", 
                   "idx: %d ts: %d dir: %d confAvg: %d count: %d",
                   index, node.timestampEnd, node.directionIndex, node.confidenceAvg, node.count);
}

void MicDirectionHistory::AddDirectionSample(TimeStamp_t timestamp, 
                                             MicDirectionHistory::DirectionIndex newDirection,
                                             DirectionConfidence newConf)
{
  if (_micDirectionBuffer[_micDirectionBufferIndex].directionIndex == newDirection)
  {
    auto& currentEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    currentEntry.timestampEnd = timestamp;

    if (newConf > currentEntry.confidenceMax)
    {
      currentEntry.confidenceMax = newConf;
      currentEntry.timestampAtMax = timestamp;
    }

    const auto curAvgConf = static_cast<uint32_t>(currentEntry.confidenceAvg);
    const auto weightedConf = Util::numeric_cast<uint32_t>((curAvgConf * currentEntry.count) + newConf);
    const auto newAvgConf = Util::numeric_cast<DirectionConfidence>(weightedConf / (currentEntry.count + 1));
    currentEntry.confidenceAvg = newAvgConf;

    ++currentEntry.count;
  }
  else
  {
    _micDirectionBufferIndex = GetNextNodeIdx(_micDirectionBufferIndex);
    auto& newEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    newEntry.timestampEnd = timestamp;
    newEntry.directionIndex = newDirection;
    newEntry.confidenceAvg = newConf;
    newEntry.confidenceMax = newConf;
    newEntry.timestampAtMax = timestamp;
    newEntry.count = 1;
  }
}

uint32_t MicDirectionHistory::GetNextNodeIdx(uint32_t nodeIndex) const
{
  return (nodeIndex + 1) % kMicDirectionHistoryLen;
}

uint32_t MicDirectionHistory::GetPrevNodeIdx(uint32_t nodeIndex) const
{
  return (nodeIndex == 0) ? (kMicDirectionHistoryLen - 1) : (nodeIndex - 1);
}

MicDirectionHistory::DirectionIndex MicDirectionHistory::GetDirectionAtTime(TimeStamp_t timestampEnd, 
                                                                            TimeStamp_t timeLength_ms) const
{
  // Special case where we're asked for time after or at the end of history
  {
    auto& currentEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    if (timestampEnd >= currentEntry.timestampEnd)
    {
      const auto timeDiff = timestampEnd - currentEntry.timestampEnd;
      // Update our timelength to remove time not yet recorded in direction history
      timeLength_ms = timeDiff > timeLength_ms ? 0 : timeLength_ms - timeDiff;
      return GetRecentDirection(timeLength_ms);
    }
  }

  // Find the node containing the timestamp asked about
  auto endingIndex = _micDirectionBufferIndex;
  auto prevToEndingIndex = GetPrevNodeIdx(endingIndex);
  while (prevToEndingIndex != _micDirectionBufferIndex && _micDirectionBuffer[prevToEndingIndex].timestampEnd > timestampEnd)
  {
    endingIndex = prevToEndingIndex;
    prevToEndingIndex = GetPrevNodeIdx(endingIndex);
  }

  const auto& endingNode = _micDirectionBuffer[endingIndex];

  // Did we hit the end of history?
  if (prevToEndingIndex == _micDirectionBufferIndex)
  {
    return endingNode.directionIndex;
  }

  // Was the full time length requested within this node?
  const auto timeSpentInEndNode = timestampEnd - _micDirectionBuffer[prevToEndingIndex].timestampEnd;
  if (timeSpentInEndNode >= timeLength_ms)
  {
    return endingNode.directionIndex;
  }

  // Start with the count from before the ending node
  const auto timePrevToNode = timeLength_ms - timeSpentInEndNode;
  DirectionHistoryCount directionCount = GetDirectionCountAtIndex(prevToEndingIndex, timePrevToNode);

  // Add in the partial count based on leftover time
  const auto fullNodeLength = endingNode.timestampEnd - _micDirectionBuffer[prevToEndingIndex].timestampEnd;
  const auto proportionCount = (timeSpentInEndNode * endingNode.count) / fullNodeLength;
  directionCount[endingNode.directionIndex] += proportionCount;

  return GetBestDirection(directionCount);
}

MicDirectionHistory::DirectionIndex MicDirectionHistory::GetRecentDirection(TimeStamp_t timeLength_ms) const
{
  const auto startIndex = _micDirectionBufferIndex;

  // Special case where we just want the first piece of info we can get.
  if (timeLength_ms == 0)
  {
    return _micDirectionBuffer[startIndex].directionIndex;
  }

  DirectionHistoryCount directionCount = GetDirectionCountAtIndex(startIndex, timeLength_ms);
  return GetBestDirection(directionCount);
}

MicDirectionHistory::DirectionHistoryCount MicDirectionHistory::GetDirectionCountAtIndex(uint32_t startIndex, 
                                                                                         TimeStamp_t timeLength_ms) const
{
  DirectionHistoryCount directionCount{};
  uint32_t curIndex = startIndex;
  while (timeLength_ms > 0)
  {
    const auto& currentNode = _micDirectionBuffer[curIndex];
    const auto prevNodeIdx = GetPrevNodeIdx(curIndex);
    // If we've wrapped all the way around, we're done
    if (prevNodeIdx == _micDirectionBufferIndex)
    {
      break;
    }
    const auto& prevNode = _micDirectionBuffer[prevNodeIdx];
    const auto nodeTimeDiff = currentNode.timestampEnd - prevNode.timestampEnd;

    // If we're using up the last of the time we're counting over, do a proportional count and break
    if (nodeTimeDiff >= timeLength_ms)
    {
      const auto divisor = nodeTimeDiff > 0 ? nodeTimeDiff : 1;
      const auto proportionCount = (timeLength_ms * currentNode.count) / divisor;
      directionCount[currentNode.directionIndex] += proportionCount;
      timeLength_ms = 0;
      break;
    }

    // otherwise count everything from this node and move on
    timeLength_ms -= nodeTimeDiff;
    directionCount[currentNode.directionIndex] += currentNode.count;
    curIndex = prevNodeIdx;
  }

  return directionCount;
}

MicDirectionHistory::DirectionIndex MicDirectionHistory::GetBestDirection(const DirectionHistoryCount& directionCount)
{
  auto bestIndex = kDirectionUnknown;
  uint32_t bestCount = 0;
  for (auto i = kFirstIndex; i <= kLastIndex; ++i)
  {
    if (directionCount[i] > bestCount)
    {
      bestCount = directionCount[i];
      bestIndex = i;
    }
  }
  return bestIndex;
}

MicDirectionHistory::NodeList MicDirectionHistory::GetRecentHistory(TimeStamp_t timeLength_ms) const
{
  const auto startIndex = _micDirectionBufferIndex;

  // Special case for requesting just the most recent bit of history
  if (timeLength_ms == 0)
  {
    NodeList results;
    const auto& node = _micDirectionBuffer[startIndex];
    if (node.IsValid())
    {
      results.push_front(node);
    }
    return results;
  }

  return GetHistoryAtIndex(startIndex, timeLength_ms);
}

MicDirectionHistory::NodeList MicDirectionHistory::GetHistoryAtTime(TimeStamp_t timestampEnd, 
                                                                    TimeStamp_t timeLength_ms) const
{
  // Special case where we're asked for time after or at the end of history
  {
    auto& currentEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    if (timestampEnd >= currentEntry.timestampEnd)
    {
      const auto timeDiff = timestampEnd - currentEntry.timestampEnd;
      // Update our timelength to remove time not yet recorded in direction history
      timeLength_ms = timeDiff > timeLength_ms ? 0 : timeLength_ms - timeDiff;
      return GetRecentHistory(timeLength_ms);
    }
  }

  // Find the node containing the timestamp asked about
  auto endingIndex = _micDirectionBufferIndex;
  auto prevToEndingIndex = GetPrevNodeIdx(endingIndex);
  while (prevToEndingIndex != _micDirectionBufferIndex && _micDirectionBuffer[prevToEndingIndex].timestampEnd >= timestampEnd)
  {
    endingIndex = prevToEndingIndex;
    prevToEndingIndex = GetPrevNodeIdx(endingIndex);
  }

  const auto& endingNode = _micDirectionBuffer[endingIndex];

  // Did we hit the end of history?
  if (prevToEndingIndex == _micDirectionBufferIndex)
  {
    NodeList results;
    if (endingNode.IsValid())
    {
      results.push_front(endingNode);
    }
    return results;
  }

  // Was the full time length requested within this node?
  const auto timeSpentInEndNode = timestampEnd - _micDirectionBuffer[prevToEndingIndex].timestampEnd;
  if (timeSpentInEndNode >= timeLength_ms)
  {
    NodeList results;
    if (endingNode.IsValid())
    {
      results.push_front(endingNode);
    }
    return results;
  }

  // Start with the count from before the ending node
  const auto timePrevToNode = timeLength_ms - timeSpentInEndNode;
  NodeList results = GetHistoryAtIndex(prevToEndingIndex, timePrevToNode);
  if (endingNode.IsValid())
  {
    results.push_back(endingNode);
  }

  return results;
}

MicDirectionHistory::NodeList MicDirectionHistory::GetHistoryAtIndex(uint32_t startIndex, 
                                                                     TimeStamp_t timeLength_ms) const
{
  NodeList results;
  uint32_t curIndex = startIndex;
  while (timeLength_ms > 0)
  {
    const auto& currentNode = _micDirectionBuffer[curIndex];
    const auto prevNodeIdx = GetPrevNodeIdx(curIndex);
    // If we've wrapped all the way around, we're done
    if (prevNodeIdx == _micDirectionBufferIndex)
    {
      break;
    }
    const auto& prevNode = _micDirectionBuffer[prevNodeIdx];
    const auto nodeTimeDiff = currentNode.timestampEnd - prevNode.timestampEnd;

    // otherwise include this node and move on
    if (nodeTimeDiff >= timeLength_ms)
    {
      timeLength_ms = 0;
    }
    else
    {
      timeLength_ms -= nodeTimeDiff;
    }

    if (currentNode.IsValid())
    {
      results.push_front(currentNode);
    }
    curIndex = prevNodeIdx;
  }

  return results;
}

} // namespace Cozmo
} // namespace Anki
