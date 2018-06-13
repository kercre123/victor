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

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Cozmo {

namespace {
  #define CONSOLE_GROUP "BeatDetectorComponent"
  
  // Minimum beat detector confidence to be considered an actual beat
  CONSOLE_VAR_RANGED(float, kConfidenceThreshold, CONSOLE_GROUP, 0.25f, 0.01f, 50.f);
  
  // Length of time to keep a history of detected beats
  CONSOLE_VAR_RANGED(float, kBeatHistoryWindowSize_sec, CONSOLE_GROUP, 10.f, 1.f, 60.f);
  
  // Minimum number of beats that we must have accumulated in
  // the history window in order to say a beat is detected
  CONSOLE_VAR_RANGED(int, kMinNumBeatsInHistory, CONSOLE_GROUP, 8, 2, 50);
  
  // Detected tempo must be steadily within this window
  // in order to say a beat is detected
  CONSOLE_VAR_RANGED(float, kTempoSteadyThreshold_bpm, CONSOLE_GROUP, 5, 1, 25);
  
  // Used to fake a detected beat. If greater than 0, then a series of confident
  // beats is created at the given bpm.
  CONSOLE_VAR_RANGED(float, kFakeBeat_bpm, CONSOLE_GROUP, -1.f, -1.f, 200.f);
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
    // Only call callback if we are not currently faking a beat
    if (kFakeBeat_bpm < 0.f) {
      OnBeat(event.GetData().Get_beatDetectorState().latestBeat);
    }
  });
}

void BeatDetectorComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  // Are we currently faking a beat?
  static float prevFakeBeat_bpm = kFakeBeat_bpm;
  if (kFakeBeat_bpm > 0.f) {
    if (kFakeBeat_bpm != prevFakeBeat_bpm) {
      // Just started faking a beat - clear the queue
      _recentBeats.clear();
    }
    const float fakeBeatPeriod_sec = 60.f / kFakeBeat_bpm;
    const float lastFakeBeatTime_sec = _recentBeats.empty() ? 0.f : _recentBeats.back().time_sec;
    // Do we need to add any new beats?
    if (now_sec > lastFakeBeatTime_sec + fakeBeatPeriod_sec) {
      BeatInfo beat;
      beat.tempo_bpm = kFakeBeat_bpm;
      beat.confidence = 100.f;
      // If this is the first beat, pretend it happened right now
      beat.time_sec = _recentBeats.empty() ? now_sec : lastFakeBeatTime_sec + fakeBeatPeriod_sec;
      OnBeat(beat);
    }
  }
  prevFakeBeat_bpm = kFakeBeat_bpm;

  // Cull the recent beats list to its window size
  while (!_recentBeats.empty() &&
         _recentBeats.front().time_sec < now_sec - kBeatHistoryWindowSize_sec) {
    _recentBeats.pop_front();
  }
}


void BeatDetectorComponent::OnBeat(const BeatInfo& beat)
{
  _recentBeats.push_back(beat);
  
  for (const auto& callbackMapEntry : _onBeatCallbacks) {
    const auto& callback = callbackMapEntry.second;
    callback();
  }
}


bool BeatDetectorComponent::IsBeatDetected() const
{
  // In order for a beat to be considered detected, the following conditions must be true:
  //   - We have a minimum number of recent beats in the history
  //   - All recent beats have high enough confidence
  //   - Tempo has been steady for a minimum amount of time
  
  const bool haveEnoughBeats = _recentBeats.size() >= kMinNumBeatsInHistory;
  if (!haveEnoughBeats) {
    return false;
  }
  
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
  
  return confidenceHighEnough && tempoSteady;
}

float BeatDetectorComponent::GetNextBeatTime_sec() const
{
  if (!IsBeatDetected() || _recentBeats.empty()) {
    PRINT_NAMED_WARNING("BeatDetectorComponent.GetNextBeatTime.NoBeatDetected",
                        "No current beat is detected");
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
  if (!IsBeatDetected() || _recentBeats.empty()) {
    PRINT_NAMED_WARNING("BeatDetectorComponent.GetCurrTempo_bpm.NoBeatDetected",
                        "No current beat is detected");
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

  
int BeatDetectorComponent::RegisterOnBeatCallback(const OnBeatCallback& callback)
{
  static int callbackId = 1;
  _onBeatCallbacks[callbackId] = callback;
  return callbackId++;
}

  
bool BeatDetectorComponent::UnregisterOnBeatCallback(const int callbackId)
{
  const auto numErased = _onBeatCallbacks.erase(callbackId);
  const bool success = ANKI_VERIFY(numErased == 1,
                                  "BeatDetectorComponent.UnregisterOnBeatCallback.FailedToUnregisterCallback",
                                  "Failed to erase callback with ID %d (num erased %zu)", callbackId, numErased);
  return success;
}

} // namespace Cozmo
} // namespace Anki

