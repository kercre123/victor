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
#include "engine/components/mics/micDirectionTypes.h"
#include "engine/engineTimeStamp.h"
#include "engine/robotComponents_fwd.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include <array>
#include <cstdint>
#include <functional>

namespace Anki {
namespace Vector {

class CozmoContext;

namespace WebService {
  class WebService;
}

class MicDirectionHistory
{
public:
  MicDirectionHistory() : _webService(nullptr) {}

  void Initialize( const CozmoContext* context );

  static constexpr uint32_t kMicDirectionHistory_ms = 10000;
  static constexpr uint32_t kTimePerDirectionUpdate_ms = 10;
  static constexpr uint32_t kMicDirectionHistoryLen = kMicDirectionHistory_ms / kTimePerDirectionUpdate_ms;

  void AddMicSample( const RobotInterface::MicDirection& message );
  void AddDirectionSample(EngineTimeStamp_t timestamp, bool isVadActive, MicDirectionIndex newIndex, MicDirectionConfidence newConf,
                          MicDirectionIndex selectedDirection);
  void AddMicPowerSample(EngineTimeStamp_t timestamp, float powerLevel, float noiseFloor);
  void AddVadLogSample(EngineTimeStamp_t timeStamp, float noiseFloorLevel, int activeState);

  // Interface for requesting the "best" direction
  static constexpr uint32_t kDefaultDirectionRecentTime_ms = 1000;
  MicDirectionIndex GetRecentDirection(TimeStamp_t timeLength_ms = kDefaultDirectionRecentTime_ms) const;
  MicDirectionIndex GetDirectionAtTime(EngineTimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;

  // Selected direction is only valid when microphone beamforming is enabled, the direction will either be the what the
  // mic processing has determined same as "RecentDirection" or if the beamforming direction has been specifically set.
  MicDirectionIndex GetSelectedDirection() const { return _mostRecentSelectedDirection; }

  MicDirectionNodeList GetRecentHistory(TimeStamp_t timeLength_ms) const;
  MicDirectionNodeList GetHistoryAtTime(EngineTimeStamp_t timestampEnd, TimeStamp_t timeLength_ms) const;


  // currently we only allow the listerner to supply an OnSound "reaction", but could easily have them supply
  // the "testing" function to determine themselves what is considered a valid sound reaction
  SoundReactorId RegisterSoundReactor(OnMicPowerSampledCallback callback);
  void UnRegisterSoundReactor(SoundReactorId id);


private:
  WebService::WebService* _webService;

  std::array<MicDirectionNode, kMicDirectionHistoryLen> _micDirectionBuffer;
  uint32_t _micDirectionBufferIndex = 0;

  // direction that we're currently focusing our mics
  MicDirectionIndex _mostRecentSelectedDirection = kMicDirectionUnknown;

  struct SoundTrackingData
  {
    MicDirectionIndex         latestMicDirection = kMicDirectionUnknown;
    MicDirectionIndex         selectedMicDirection = kMicDirectionUnknown;
    bool                      isVadActive = false;
    MicDirectionConfidence    latestMicConfidence = 0;
    double                    latestPowerLevel = 0.0;
    double                    latestNoiseFloor = 0.0;
    bool                      increasing = false;
    double                    latestPeakLevel = 0.0;
    double                    averagePeakLevel = 0.0;
  };
  struct SoundReactionListener
  {
    SoundReactorId              id;
    OnMicPowerSampledCallback   callback;
  };
  
  struct VadTrackingData
  {
    bool                    isActive            = false;
    EngineTimeStamp_t       timeActive_ms       = 0;
    uint32_t                totalActiveCount    = 0;
    EngineTimeStamp_t       totalTimeActive_ms  = 0;
    float                   noiseFloorAverage   = 0.f;
    EngineTimeStamp_t       sessionStartTime_ms = 0;
    
    void ResetData();
    void AddNoiseFloorAverage( float noiseFloorLevel );
  };
  
  // data we want to send to the webserver for debugging, but don't otherwise need to store
  struct WebServerData
  {
    bool                    forceUpdate = false;
    double                  timeOfReaction = 0.0;
    double                  powerLevel = 0.0;
    MicDirectionConfidence  confidence = 0;
    MicDirectionIndex       direction = kMicDirectionUnknown;
  };
  
  SoundTrackingData         _soundTrackingData;
  VadTrackingData           _vadTrackingData;
  WebServerData             _webServerData;
  std::vector<SoundReactionListener> _soundReactors;

  using DirectionHistoryCount = std::array<uint32_t, (kLastMicDirectionIndex - kFirstMicDirectionIndex + 1)>;

  uint32_t GetNextNodeIdx(uint32_t nodeIndex) const;
  uint32_t GetPrevNodeIdx(uint32_t nodeIndex) const;
  void PrintNodeData(uint32_t index) const;

  DirectionHistoryCount GetDirectionCountAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  MicDirectionNodeList GetHistoryAtIndex(uint32_t startIndex, TimeStamp_t timeLength_ms) const;
  static MicDirectionIndex GetBestDirection(const DirectionHistoryCount& directionCount);

  double AccumulatePeakPowerLevel( double latestPeakPowerLevel );
  void ProcessSoundReactors();
  void SendMicDataToWebserver();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_MicDirectionHistory_H_
