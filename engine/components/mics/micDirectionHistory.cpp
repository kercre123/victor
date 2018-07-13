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

#include "engine/components/mics/micDirectionHistory.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "webServerProcess/src/webService.h"

#include "clad/robotInterface/messageRobotToEngine.h"


#if defined(ANKI_NO_WEBSERVER_ENABLED)
  #define SEND_MICENGINE_DATA !ANKI_NO_WEBSERVER_ENABLED
#else
  #define SEND_MICENGINE_DATA 1
#endif

namespace Anki {
namespace Cozmo {

CONSOLE_VAR( MicDirectionHistory::MicPowerLevelType,  kReactionPowerThreshold,       "MicData", 2.90 );
CONSOLE_VAR( MicDirectionConfidence,                  kReactionConfidenceThreshold,  "MicData", 0 );


static_assert(std::is_same<decltype(RobotInterface::MicDirection::timestamp), TimeStamp_t>::value,
              "Expecting type of MicDirection::timestamp to match TimeStamp_t");
static_assert(std::is_same<decltype(RobotInterface::MicDirection::direction),
                           MicDirectionIndex>::value,
              "Expecting type of MicDirection::direction to match MicDirectionIndex");
static_assert(std::is_same<decltype(RobotInterface::MicDirection::confidence),
                           MicDirectionConfidence>::value,
              "Expecting type of MicDirection::confidence to match MicDirectionConfidence");

void MicDirectionHistory::PrintNodeData(uint32_t index) const
{
  const MicDirectionNode& node = _micDirectionBuffer[GetPrevNodeIdx(GetNextNodeIdx(index))];
  PRINT_NAMED_INFO("MicDirectionHistory::PrintNodeData", 
                   "idx: %d ts: %d dir: %d confAvg: %d count: %d",
                   index, node.timestampEnd, node.directionIndex, node.confidenceAvg, node.count);
}

void MicDirectionHistory::AddMicSample(const RobotInterface::MicDirection& message)
{
  // we use the previous "Direction Sample" in order to compute sound reactions within the "Mic Power Sample",
  // so we need to call AddMicPowerSample prior to calling AddDirectionSample
  AddMicPowerSample(message.timestamp, message.latestPowerValue, message.latestNoiseFloor);
  AddDirectionSample(message.timestamp, message.direction, message.confidence, message.selectedDirection);

  SendMicDataToWebserver( message );
}

void MicDirectionHistory::AddDirectionSample(TimeStamp_t timestamp, 
                                             MicDirectionIndex newDirection,
                                             MicDirectionConfidence newConf,
                                             MicDirectionIndex selectedDirection)
{
  if (_micDirectionBuffer[_micDirectionBufferIndex].directionIndex == newDirection)
  {
    auto& currentEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    currentEntry.timestampEnd = timestamp;
    currentEntry.latestConfidence = newConf;

    if (newConf > currentEntry.confidenceMax)
    {
      currentEntry.confidenceMax = newConf;
      currentEntry.timestampAtMax = timestamp;
    }

    const auto curAvgConf = static_cast<uint32_t>(currentEntry.confidenceAvg);
    const auto weightedConf = Util::numeric_cast<uint32_t>((curAvgConf * currentEntry.count) + newConf);
    const auto newAvgConf = Util::numeric_cast<MicDirectionConfidence>(weightedConf / (currentEntry.count + 1));
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
    newEntry.latestConfidence = newConf;
    newEntry.timestampAtMax = timestamp;
    newEntry.count = 1;
  }

  // only store the selected direction when it is known
  // note: when the robot is moving, selected direction is set to unknown
  //       when no focus direction is set, this will be the same as newDirection
  if (selectedDirection != kMicDirectionUnknown)
  {
    _mostRecentSelectedDirection = selectedDirection;
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

MicDirectionIndex MicDirectionHistory::GetDirectionAtTime(TimeStamp_t timestampEnd,
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

MicDirectionIndex MicDirectionHistory::GetRecentDirection(TimeStamp_t timeLength_ms) const
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

MicDirectionIndex MicDirectionHistory::GetBestDirection(const DirectionHistoryCount& directionCount)
{
  MicDirectionIndex bestIndex = kMicDirectionUnknown;
  uint32_t bestCount = 0;
  for (MicDirectionIndex i = kFirstMicDirectionIndex; i < kNumMicDirections; ++i) // Don't consider unknown direction
  {
    if (directionCount[i] > bestCount)
    {
      bestCount = directionCount[i];
      bestIndex = i;
    }
  }
  return bestIndex;
}

MicDirectionNodeList MicDirectionHistory::GetRecentHistory(TimeStamp_t timeLength_ms) const
{
  const auto startIndex = _micDirectionBufferIndex;

  // Special case for requesting just the most recent bit of history
  if (timeLength_ms == 0)
  {
    MicDirectionNodeList results;
    const auto& node = _micDirectionBuffer[startIndex];
    if (node.IsValid())
    {
      results.push_front(node);
    }
    return results;
  }

  return GetHistoryAtIndex(startIndex, timeLength_ms);
}

MicDirectionNodeList MicDirectionHistory::GetHistoryAtTime(TimeStamp_t timestampEnd,
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
    MicDirectionNodeList results;
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
    MicDirectionNodeList results;
    if (endingNode.IsValid())
    {
      results.push_front(endingNode);
    }
    return results;
  }

  // Start with the count from before the ending node
  const auto timePrevToNode = timeLength_ms - timeSpentInEndNode;
  MicDirectionNodeList results = GetHistoryAtIndex(prevToEndingIndex, timePrevToNode);
  if (endingNode.IsValid())
  {
    results.push_back(endingNode);
  }

  return results;
}

MicDirectionNodeList MicDirectionHistory::GetHistoryAtIndex(uint32_t startIndex,
                                                                     TimeStamp_t timeLength_ms) const
{
  MicDirectionNodeList results;
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

SoundReactorId MicDirectionHistory::RegisterSoundReactor(OnSoundReactionCallback callback)
{
  static SoundReactorId sNextId = kInvalidSoundReactorId;

  ++sNextId;
  _soundReactors.push_back( { sNextId, callback } );

  return sNextId;
}

void MicDirectionHistory::UnRegisterSoundReactor(SoundReactorId id)
{
  for (size_t index = _soundReactors.size(); index > 0;)
  {
    --index;
    if (id == _soundReactors[index].id) {
      _soundReactors[index] = _soundReactors.back();
      _soundReactors.pop_back();
    }
  }
}

void MicDirectionHistory::AddMicPowerSample(TimeStamp_t timestamp, float powerLevel, float noiseFloor)
{
  const MicPowerLevelType previousPowerLevel = _soundTrackingData.latestPowerLevel;
  const MicPowerLevelType previousNoiseFloor = _soundTrackingData.latestNoiseFloor;

  // since we react a "frame" after our peak levels, let's record the previous values so we can debug our
  // reactions via the webserver
  #if SEND_MICENGINE_DATA
  {
    _webServerData.powerLevel = previousPowerLevel;
    _webServerData.noiseFloor = previousNoiseFloor;
  }
  #endif
  
  _soundTrackingData.latestPowerLevel = log10(static_cast<MicPowerLevelType>(powerLevel));
  _soundTrackingData.latestNoiseFloor = log10(static_cast<MicPowerLevelType>(noiseFloor));

  // we want to only track the "average peak magnitude" of the mic power level, as it fluctuates wildly each sample.
  // we are trying to determine when the power level spikes high outside of the "average peak magnitude"
  // to find the "peak magnitude", we need to essentially find the local maximum of the power samples
  const MicPowerLevelType powerLevelDelta = ( _soundTrackingData.latestPowerLevel - previousPowerLevel );
  if ( !Util::IsNearZero( powerLevelDelta ) )
  {
    // if we were currently increasing in value, and now we're decreasing, consider this our local maximum
    const bool isIncreasing = ( powerLevelDelta > 0.0f );
    if ( !isIncreasing && _soundTrackingData.increasing )
    {
      // we know we're at a peak right now, but we only care about peaks that are above the noise floor
      if ( previousPowerLevel > previousNoiseFloor )
      {
        AccumulatePeakPowerLevel( previousPowerLevel - previousNoiseFloor );

        SoundReactionData data =
        {
          .currentPowerLevel  = _soundTrackingData.latestPeakLevel,
          .averagePowerLevel  = _soundTrackingData.averagePeakLevel,
          .micDirectionData   = _micDirectionBuffer[_micDirectionBufferIndex],
        };

        if ( ShouldReactToSound( data ) ) {
          // let all of our listeners know that we've heard a valid reaction sound
          for (auto reactor : _soundReactors) {
            reactor.callback(data.micDirectionData);
          }

          // only used for debugging purposes
          #if SEND_MICENGINE_DATA
          {
            _webServerData.forceUpdate = true;

            _webServerData.timeOfReaction = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
            _webServerData.reactionData = data;
          }
          #endif
        }
      }
    }

    _soundTrackingData.increasing = isIncreasing;
  }
}

MicDirectionHistory::MicPowerLevelType MicDirectionHistory::AccumulatePeakPowerLevel( MicPowerLevelType latestPeakPowerLevel )
{
  constexpr double alpha = 1.0 / 50.0;

  // simple exponential moving average over 50 samples
  _soundTrackingData.latestPeakLevel = latestPeakPowerLevel;
  _soundTrackingData.averagePeakLevel = ( alpha * latestPeakPowerLevel ) + ( 1.0f - alpha ) * _soundTrackingData.averagePeakLevel;

  return _soundTrackingData.averagePeakLevel;
}

bool MicDirectionHistory::ShouldReactToSound( const SoundReactionData& data )
{
  const MicPowerLevelType powerDelta = ( data.currentPowerLevel - data.averagePowerLevel );

  const bool exceedsPowerThreshold = ( powerDelta >= kReactionPowerThreshold );
  const bool exceedsConfidenceThreshold = ( data.micDirectionData.latestConfidence >= kReactionConfidenceThreshold );

  return ( exceedsPowerThreshold && exceedsConfidenceThreshold );
}

void MicDirectionHistory::SendMicDataToWebserver( const RobotInterface::MicDirection& micData )
{
  #if SEND_MICENGINE_DATA
  {
    if ( nullptr != _webService )
    {
      static double sNextUpdateTime = 0.0;
      const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if ( ( currentTime >= sNextUpdateTime ) || _webServerData.forceUpdate )
      {
        sNextUpdateTime += 0.20; // update every Xms
        _webServerData.forceUpdate = false;

        Json::Value webData;
        webData["time"] = currentTime;
        webData["confidence"] = micData.confidence;
        // 'selectedDirection' is what's being used (locked-in), whereas 'dominant' is just the strongest direction
        webData["dominant"] = micData.direction;
        webData["selectedDirection"] = micData.selectedDirection;
        webData["maxConfidence"] = 20000;

        webData["latestPowerValue"] = _webServerData.powerLevel;
        webData["latestNoiseFloor"] = _webServerData.noiseFloor;

        webData["powerScore"] = _soundTrackingData.latestPeakLevel;
        webData["powerScoreAvg"] = _soundTrackingData.averagePeakLevel;

        webData["displayPowerScore"] = _webServerData.noiseFloor + _soundTrackingData.latestPeakLevel;
        webData["displayPowerAvg"] = _webServerData.noiseFloor + _soundTrackingData.averagePeakLevel;
        webData["displayPowerThreshold"] = _webServerData.noiseFloor + _soundTrackingData.averagePeakLevel + kReactionPowerThreshold;

        webData["isTriggered"] = ( currentTime <= ( _webServerData.timeOfReaction + 1.0f ) );
        webData["triggerScore"] = _webServerData.reactionData.currentPowerLevel - _webServerData.reactionData.averagePowerLevel;
        webData["triggerConfidence"] = _webServerData.reactionData.micDirectionData.latestConfidence;

        Json::Value& directionValues = webData["directions"];
        for ( float confidence : micData.confidenceList )
        {
          directionValues.append(confidence);
        }

        static const std::string moduleName = "soundreactions";
        _webService->SendToWebViz( moduleName, webData );
      }
    }
  }
  #endif
}

} // namespace Cozmo
} // namespace Anki
