/**
 * File: beatDetectorComponent
 *
 * Author: Matt Michini
 * Created: 05/03/2018
 *
 * Description: Component for managing beat/tempo detection (which is actually done in the anim process)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "util/logging/logging.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Cozmo {

namespace {
  // Minimum beat detector confidence to be considered an actual beat
  const float kConfidenceThreshold = 0.13f;
  
  // Length of time to keep a history of detected beats
  const int kBeatHistoryWindowSize_sec = 10.f;
  
  // Minimum number of beats that we must have accumulated in
  // the history window in order to say a beat is detected
  const int kMinNumBeatsInHistory = 8;
  
  // Detected tempo must be steadily within this window
  // in order to say a beat is detected
  const float kTempoSteadyThreshold_bpm = 7;
}

BeatDetectorComponent::BeatDetectorComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::BeatDetector)
{
}


BeatDetectorComponent::~BeatDetectorComponent()
{
}


void BeatDetectorComponent::GetInitDependencies( RobotCompIDSet& dependencies ) const
{
  dependencies.insert( RobotComponentID::CozmoContextWrapper );
}


void BeatDetectorComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  
  auto* messageHandler = robot->GetRobotMessageHandler();
  _signalHandle = messageHandler->Subscribe(RobotInterface::RobotToEngineTag::beatDetectorState, [this](const AnkiEvent<RobotInterface::RobotToEngine>& event) {
    _recentBeats.push_back(event.GetData().Get_beatDetectorState().latestBeat);
  });
}

void BeatDetectorComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // Cull the recent beats list to its window size
  if (!_recentBeats.empty()) {
    const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
    while (_recentBeats.front().time_sec < now_sec - kBeatHistoryWindowSize_sec) {
      _recentBeats.pop_front();
    }
  }
}

bool BeatDetectorComponent::IsBeatDetected() const
{
  // In order for a beat to be considered detected, the following conditions must be true:
  //   - We have a minimum number of recent beats in the history
  //   - All recent beats have high enough confidence
  //   - Tempo has been steady for a minimum amount of time
  
  const bool haveEnoughBeats = _recentBeats.size() >= kMinNumBeatsInHistory;
  const bool confidenceHighEnough = std::all_of(_recentBeats.begin(),
                                                _recentBeats.end(),
                                                [](const BeatInfo& b) {
                                                  return b.confidence > kConfidenceThreshold;
                                                });
  const auto minMaxTempoItPair = std::minmax_element(_recentBeats.begin(),
                                                     _recentBeats.end(),
                                                     [](const BeatInfo& b1, const BeatInfo& b2) {
                                                       return b1.tempo_bpm < b2.tempo_bpm;
                                                     });
  const bool tempoSteady = Util::IsNear(minMaxTempoItPair.second->tempo_bpm,
                                        minMaxTempoItPair.first->tempo_bpm,
                                        kTempoSteadyThreshold_bpm);
  
  return haveEnoughBeats && confidenceHighEnough && tempoSteady;
}

float BeatDetectorComponent::GetNextBeatTime_sec() const
{
  if (!IsBeatDetected()) {
    DEV_ASSERT(false, "BeatDetectorComponent.GetNextBeatTime.NoBeatDetected");
    return 0.f;
  }
  
  const auto& latestBeat = _recentBeats.back();
  
  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  const float beatPeriod_sec = 60.f / latestBeat.tempo_bpm;
  float nextBeatTime_sec = latestBeat.time_sec + beatPeriod_sec;
  while(nextBeatTime_sec < now_sec) {
    nextBeatTime_sec += beatPeriod_sec;
  }
  return nextBeatTime_sec;
}


float BeatDetectorComponent::GetCurrTempo_bpm() const
{
  if (!IsBeatDetected()) {
    DEV_ASSERT(false, "BeatDetectorComponent.GetCurrTempo_bpm.NoBeatDetected");
    return 0.f;
  }
  
  const auto& latestBeat = _recentBeats.back();
  return latestBeat.tempo_bpm;
}


void BeatDetectorComponent::Reset()
{
  _recentBeats.clear();
  
  _robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::ResetBeatDetector()));
}

} // namespace Cozmo
} // namespace Anki

