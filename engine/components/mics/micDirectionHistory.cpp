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
#include "engine/cozmoContext.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"
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
namespace Vector {
namespace {
  const EngineTimeStamp_t kVadSessionLength_ms = (30 * 60 * 1000);  // 30 mins == 1,800,000 ms
}

CONSOLE_VAR_RANGED( uint32_t,       kRTS_PowerAvgNumSamples,   "SoundReaction", 100, 1, 250 );
CONSOLE_VAR_RANGED( double,         kRTS_WebVizUpdateInterval, "SoundReaction", 0.20, 0.0, 1.0 );

// NOTE: these are only for display purposes now (actual values live in instance jsons)
CONSOLE_VAR( double,                kRTS_AbsolutePowerThreshold_display,           "SoundReaction", 2.90 );
CONSOLE_VAR( double,                kRTS_MinPowerThreshold_display,                "SoundReaction", 1.50 );

static_assert(std::is_same<decltype(RobotInterface::MicDirection::timestamp), TimeStamp_t>::value,
              "Expecting type of MicDirection::timestamp to match TimeStamp_t");
static_assert(std::is_same<decltype(RobotInterface::MicDirection::direction),
                           MicDirectionIndex>::value,
              "Expecting type of MicDirection::direction to match MicDirectionIndex");
static_assert(std::is_same<decltype(RobotInterface::MicDirection::confidence),
                           MicDirectionConfidence>::value,
              "Expecting type of MicDirection::confidence to match MicDirectionConfidence");

void MicDirectionHistory::Initialize( const CozmoContext* context )
{
  _webService = context->GetWebService();
}

void MicDirectionHistory::PrintNodeData(uint32_t index) const
{
  const MicDirectionNode& node = _micDirectionBuffer[GetPrevNodeIdx(GetNextNodeIdx(index))];
  PRINT_NAMED_INFO("MicDirectionHistory::PrintNodeData", 
                   "idx: %d ts: %d dir: %d confAvg: %d count: %d",
                   index, (TimeStamp_t)node.timestampEnd, node.directionIndex, node.confidenceAvg, node.count);
}

void MicDirectionHistory::AddMicSample(const RobotInterface::MicDirection& message)
{
  const EngineTimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  AddDirectionSample(currentTime_ms, (message.activeState == 1),
                     message.direction, message.confidence, message.selectedDirection);
  AddMicPowerSample(currentTime_ms, message.latestPowerValue, message.latestNoiseFloor);
  AddVadLogSample(currentTime_ms, message.latestNoiseFloor, message.activeState);

  ProcessSoundReactors();
  SendMicDataToWebserver();
}

uint32_t MicDirectionHistory::GetNextNodeIdx(uint32_t nodeIndex) const
{
  return (nodeIndex + 1) % kMicDirectionHistoryLen;
}

uint32_t MicDirectionHistory::GetPrevNodeIdx(uint32_t nodeIndex) const
{
  return (nodeIndex == 0) ? (kMicDirectionHistoryLen - 1) : (nodeIndex - 1);
}

MicDirectionIndex MicDirectionHistory::GetDirectionAtTime(EngineTimeStamp_t timestampEnd,
                                                          TimeStamp_t timeLength_ms) const
{
  // Special case where we're asked for time after or at the end of history
  {
    auto& currentEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    if (timestampEnd >= currentEntry.timestampEnd)
    {
      const auto timeDiff = TimeStamp_t(timestampEnd - currentEntry.timestampEnd);
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
  const auto timeSpentInEndNode = TimeStamp_t(timestampEnd - _micDirectionBuffer[prevToEndingIndex].timestampEnd);
  if (timeSpentInEndNode >= timeLength_ms)
  {
    return endingNode.directionIndex;
  }

  // Start with the count from before the ending node
  const auto timePrevToNode = timeLength_ms - timeSpentInEndNode;
  DirectionHistoryCount directionCount = GetDirectionCountAtIndex(prevToEndingIndex, timePrevToNode);

  // Add in the partial count based on leftover time
  const auto fullNodeLength = endingNode.timestampEnd - _micDirectionBuffer[prevToEndingIndex].timestampEnd;
  const auto proportionCount = (timeSpentInEndNode * endingNode.count) / (TimeStamp_t)fullNodeLength;
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
    const auto nodeTimeDiff = TimeStamp_t(currentNode.timestampEnd - prevNode.timestampEnd);

    // If we're using up the last of the time we're counting over, do a proportional count and break
    if (nodeTimeDiff >= timeLength_ms)
    {
      const TimeStamp_t divisor = nodeTimeDiff > 0 ? nodeTimeDiff : 1;
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

MicDirectionNodeList MicDirectionHistory::GetHistoryAtTime(EngineTimeStamp_t timestampEnd,
                                                           TimeStamp_t timeLength_ms) const
{
  // Special case where we're asked for time after or at the end of history
  {
    auto& currentEntry = _micDirectionBuffer[_micDirectionBufferIndex];
    if (timestampEnd >= currentEntry.timestampEnd)
    {
      const auto timeDiff = timestampEnd - currentEntry.timestampEnd;
      // Update our timelength to remove time not yet recorded in direction history
      timeLength_ms = timeDiff > timeLength_ms ? 0 : TimeStamp_t(timeLength_ms - timeDiff);
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
  const auto timePrevToNode = TimeStamp_t(timeLength_ms - timeSpentInEndNode);
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
    const auto nodeTimeDiff = (TimeStamp_t)(currentNode.timestampEnd - prevNode.timestampEnd);

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

SoundReactorId MicDirectionHistory::RegisterSoundReactor(OnMicPowerSampledCallback callback)
{
  static SoundReactorId sNextId = kInvalidSoundReactorId;

   // in case we wrap around
  if ( kInvalidSoundReactorId == ++sNextId ) {
    ++sNextId;
  }

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

      return;
    }
  }

  PRINT_NAMED_WARNING("MicDirectionHistory.UnRegisterSoundReactor", "No SoundReactor found with id %d", (int)id);
}

void MicDirectionHistory::AddDirectionSample(EngineTimeStamp_t timestamp,
                                             bool isVadActive,
                                             MicDirectionIndex newDirection,
                                             MicDirectionConfidence newConf,
                                             MicDirectionIndex selectedDirection)
{
  // use this for Sound Reactors
  _soundTrackingData.isVadActive = isVadActive;
  _soundTrackingData.latestMicDirection = newDirection;
  _soundTrackingData.selectedMicDirection = selectedDirection;
  _soundTrackingData.latestMicConfidence = newConf;


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
    newEntry.timestampBegin = timestamp;
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

void MicDirectionHistory::AddMicPowerSample(EngineTimeStamp_t timestamp, float powerLevel, float noiseFloor)
{
  const double previousPowerLevel = _soundTrackingData.latestPowerLevel;
  const double previousNoiseFloor = _soundTrackingData.latestNoiseFloor;

  _soundTrackingData.latestPowerLevel = log10(static_cast<double>(powerLevel));
  _soundTrackingData.latestNoiseFloor = log10(static_cast<double>(noiseFloor));

   // previousPowerLevel will be zero our first time through
  if ( previousPowerLevel != 0.0 ) {
    // we want to only track the "average peak magnitude" of the mic power level, as it fluctuates wildly each sample.
    // we are trying to determine when the power level spikes high outside of the "average peak magnitude"
    // to find the "peak magnitude", we need to essentially find the local maximum of the power samples
    const double powerLevelDelta = ( _soundTrackingData.latestPowerLevel - previousPowerLevel );
    if ( !Util::IsNearZero( powerLevelDelta ) ) {
       // if we were currently increasing in value, and now we're decreasing, consider this our local maximum
      const bool isIncreasing = ( powerLevelDelta > 0.0f );
      if ( !isIncreasing && _soundTrackingData.increasing ) { // we've gone from increasing -> decreasing ... PEAK
         // we know we're at a peak right now, but we only care about peaks that are above the noise floor
        if ( previousPowerLevel > previousNoiseFloor ) {
          AccumulatePeakPowerLevel( previousPowerLevel - previousNoiseFloor );
        }
      }

      _soundTrackingData.increasing = isIncreasing;
    }
  }
}

void MicDirectionHistory::AddVadLogSample(EngineTimeStamp_t timeStamp, float noiseFloorLevel, int activeState)
{
  // NOT Active -> Active
  if ( (activeState == 1) && !_vadTrackingData.isActive ) {
    _vadTrackingData.isActive = true;
    ++_vadTrackingData.totalActiveCount;
    _vadTrackingData.timeActive_ms = timeStamp;
  }
  // Active -> NOT Active
  else if ( (activeState == 0) && _vadTrackingData.isActive ) {
    _vadTrackingData.isActive = false;
    _vadTrackingData.totalTimeActive_ms += (timeStamp - _vadTrackingData.timeActive_ms);
  }
  // Always track noise floor
  _vadTrackingData.AddNoiseFloorAverage( noiseFloorLevel );
  
  // Check if Session time expired
  if ( (timeStamp - _vadTrackingData.sessionStartTime_ms) > kVadSessionLength_ms ) {
    // Time to create event, wait until the VAD is NOT Active
    if ( !_vadTrackingData.isActive ) {
      // Create Event
      const EngineTimeStamp_t totalSessionLength_ms = timeStamp - _vadTrackingData.sessionStartTime_ms;
      
      DASMSG(wakeword_vad, "wakeword.vad",
             "Track the number of times the Voice Audio Detection (VAD) is activated, how long it's activated, the \
             average noise floor during tracking and the length of the session (normally 30 minutes)");
      DASMSG_SET(i1, _vadTrackingData.totalActiveCount,
                 "Number of times VAD was actived");
      DASMSG_SET(i2, static_cast<int>((TimeStamp_t)_vadTrackingData.totalTimeActive_ms),
                 "Total time actived in milliseconds");
      DASMSG_SET(i3, static_cast<int>(_vadTrackingData.noiseFloorAverage),
                 "Average noise floor level, this is the microphone’s sound pressure value");
      DASMSG_SET(i4, static_cast<int>((TimeStamp_t)totalSessionLength_ms),
                 "Total tracking in milliseconds")
      DASMSG_SEND();
      
      // Reset Tracker
      _vadTrackingData.ResetData();
      _vadTrackingData.noiseFloorAverage = noiseFloorLevel; // Start the next session with the current level
      _vadTrackingData.sessionStartTime_ms = timeStamp;     // Start next session
    }
  }
}

double MicDirectionHistory::AccumulatePeakPowerLevel( double latestPeakPowerLevel )
{
  const double alpha = 1.0 / (double)kRTS_PowerAvgNumSamples;

   // simple exponential moving average over kRTS_PowerAvgNumSamples samples
  _soundTrackingData.latestPeakLevel = latestPeakPowerLevel;
  _soundTrackingData.averagePeakLevel = ( alpha * latestPeakPowerLevel ) + ( 1.0f - alpha ) * _soundTrackingData.averagePeakLevel;

   #if SEND_MICENGINE_DATA
  {
    // we want to visualize the spikes in mic power (even if we're a frame late at this point)
    _webServerData.forceUpdate = true;
  }
  #endif

  return _soundTrackingData.averagePeakLevel;
}

void MicDirectionHistory::ProcessSoundReactors()
{
  // ignore values until we have an average to compare against
  if ( 0.0 != _soundTrackingData.averagePeakLevel ) {
    bool isTriggered = false;

    const double powerDelta = ( _soundTrackingData.latestPowerLevel - _soundTrackingData.latestNoiseFloor );
    const double powerScore = ( powerDelta - _soundTrackingData.averagePeakLevel );

    if ( Util::IsFltGTZero( powerScore ) ) {
      for (auto reactor : _soundReactors) {
        isTriggered |= reactor.callback( powerScore, _soundTrackingData.latestMicConfidence, _soundTrackingData.latestMicDirection );
      }
    }

    #if SEND_MICENGINE_DATA
    {
      const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
      if ( isTriggered ) {
        _webServerData.forceUpdate = true;
        _webServerData.timeOfReaction = currentTime;
        _webServerData.powerLevel = powerDelta;
        _webServerData.confidence = _soundTrackingData.latestMicConfidence;
        _webServerData.direction = _soundTrackingData.latestMicDirection;
      }
    }
    #endif
  }
}


void MicDirectionHistory::SendMicDataToWebserver()
{
  #if SEND_MICENGINE_DATA
  {
    if ( nullptr != _webService )
    {
      static const std::string kWebVizModuleName = "soundreactions";
      if (_webService->IsWebVizClientSubscribed(kWebVizModuleName))
      {
        static const double kWebserverTriggerDisplayTime = 1.5;
        
        static double sNextUpdateTime = 0.0;
        const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
        
        if ( ( currentTime >= sNextUpdateTime ) || _webServerData.forceUpdate )
        {
          Json::Value webData;
          webData["time"] = currentTime;
          webData["confidence"] = _soundTrackingData.latestMicConfidence;
          webData["activeState"] = _soundTrackingData.isVadActive;
          webData["direction"] = _soundTrackingData.latestMicDirection;
          webData["selectedDirection"] = _soundTrackingData.selectedMicDirection;
          webData["latestPowerValue"] = _soundTrackingData.latestPowerLevel;
          webData["latestNoiseFloor"] = _soundTrackingData.latestNoiseFloor;
          webData["powerScore"] = _soundTrackingData.latestPeakLevel;
          webData["powerScoreAvg"] = _soundTrackingData.averagePeakLevel;
          webData["powerScoreThreshold"] = kRTS_AbsolutePowerThreshold_display;
          webData["powerScoreMinThreshold"] = kRTS_MinPowerThreshold_display;
          
          webData["isTriggered"] = ( currentTime <= ( _webServerData.timeOfReaction + kWebserverTriggerDisplayTime ) );
          webData["triggerScore"] = _webServerData.powerLevel;
          webData["triggerConfidence"] = _webServerData.confidence;
          webData["triggerDirection"] = _webServerData.direction;
          
          _webService->SendToWebViz( kWebVizModuleName, webData );
          
          sNextUpdateTime = currentTime + kRTS_WebVizUpdateInterval; // update every Xms
          _webServerData.forceUpdate = false;
        }
      }
    }
  }
  #endif
}
  
// VadTrackingData struct
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicDirectionHistory::VadTrackingData::ResetData() {
  isActive            = false;
  timeActive_ms       = 0;
  totalActiveCount    = 0;
  totalTimeActive_ms  = 0;
  noiseFloorAverage   = 0.f;
  sessionStartTime_ms = 0;
}

void MicDirectionHistory::VadTrackingData::AddNoiseFloorAverage(float noiseFloorLevel) {
  noiseFloorAverage = (noiseFloorAverage + noiseFloorLevel) / 2.0f;
}

} // namespace Vector
} // namespace Anki
