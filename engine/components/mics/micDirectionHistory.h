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
#include "clad/robotInterface/messageRobotToEngine.h"

#include <array>
#include <cstdint>
#include <functional>

namespace Anki {
namespace Cozmo {

namespace WebService {
  class WebService;
}

class MicDirectionHistory
{
public:
  using MicPowerLevelType = double;

  MicDirectionHistory() : _webService(nullptr) {}

  void Initialize( WebService::WebService* webServer ) { _webService = webServer; }

  static constexpr uint32_t kMicDirectionHistory_ms = 10000;
  static constexpr uint32_t kTimePerDirectionUpdate_ms = 10;
  static constexpr uint32_t kMicDirectionHistoryLen = kMicDirectionHistory_ms / kTimePerDirectionUpdate_ms;

  void AddMicSample( const RobotInterface::MicDirection& message );
  void AddDirectionSample(TimeStamp_t timestamp,
                          MicDirectionIndex newIndex, MicDirectionConfidence newConf,
                          MicDirectionIndex selectedDirection);
  void AddMicPowerSample(TimeStamp_t timestamp, float powerLevel, float noiseFloor);

  // Interface for requesting the "best" direction
  static constexpr uint32_t kDefaultDirectionRecentTime_ms = 1000;
  MicDirectionIndex GetRecentDirection(TimeStamp_t timeLength_ms = kDefaultDirectionRecentTime_ms) const;
  MicDirectionIndex GetDirectionAtTime(TimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

  MicDirectionIndex GetSelectedDirection() const { return _mostRecentSelectedDirection; }

  MicDirectionNodeList GetRecentHistory(TimeStamp_t timeLength_ms) const;
  MicDirectionNodeList GetHistoryAtTime(TimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

  // currently we only allow the listerner to supply an OnSound "reaction", but could easily have them supply
  // the "testing" function to determine themselves what is considered a valid sound reaction
  SoundReactorId RegisterSoundReactor(OnMicPowerSampledCallback callback);
  void UnRegisterSoundReactor(SoundReactorId id);

  void SendWebvizDebugString( const std::string& debugString );

private:
  WebService::WebService* _webService;

  std::array<MicDirectionNode, kMicDirectionHistoryLen> _micDirectionBuffer;
  uint32_t _micDirectionBufferIndex = 0;

  // direction that we're currently focusing our mics
  MicDirectionIndex _mostRecentSelectedDirection = kMicDirectionUnknown;

  struct SoundTrackingData
  {
    MicDirectionIndex         latestMicDirection = kMicDirectionUnknown;
    MicDirectionConfidence    latestMicConfidence = 0;

    MicPowerLevelType         latestPowerLevel = 0.0;
    MicPowerLevelType         latestNoiseFloor = 0.0;
    bool                      increasing = false;

    MicPowerLevelType         latestPeakLevel = 0.0;
    MicPowerLevelType         averagePeakLevel = 0.0;
  };

  struct SoundReactionListener
  {
    SoundReactorId              id;
    OnMicPowerSampledCallback   callback;
  };

  // data we want to send to the webserver for debugging, but don't otherwise need to store
  struct WebServerData
  {
    bool                    forceUpdate = false;

    double                  timeOfReaction = 0.0;
    double                  powerLevel = 0.0;
    MicDirectionConfidence  confidence = 0;
    MicDirectionIndex       direction = kMicDirectionUnknown;

    double                  timeOfDebugString = 0.0;
    std::string             debugString;
  };

  SoundTrackingData         _soundTrackingData;
  WebServerData             _webServerData;
  std::vector<SoundReactionListener> _soundReactors;

  using DirectionHistoryCount = std::array<uint32_t, (kLastMicDirectionIndex - kFirstMicDirectionIndex + 1)>;

  uint32_t GetNextNodeIdx(uint32_t nodeIndex) const;
  uint32_t GetPrevNodeIdx(uint32_t nodeIndex) const;
  void PrintNodeData(uint32_t index) const;

  DirectionHistoryCount GetDirectionCountAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  MicDirectionNodeList GetHistoryAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  static MicDirectionIndex GetBestDirection(const DirectionHistoryCount& directionCount);

  MicPowerLevelType AccumulatePeakPowerLevel( MicPowerLevelType latestPeakPowerLevel );
  void ProcessSoundReactors();
  void SendMicDataToWebserver();
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_MicDirectionHistory_H_
