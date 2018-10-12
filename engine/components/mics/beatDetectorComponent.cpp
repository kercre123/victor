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

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#include "webServerProcess/src/webVizSender.h"

namespace Anki {
namespace Vector {

namespace {
  #define CONSOLE_GROUP "BeatDetectorComponent"
  
  // Minimum beat detector confidence to be considered an actual beat
  CONSOLE_VAR_RANGED(float, kConfidenceThreshold, CONSOLE_GROUP, 0.18f, 0.01f, 1.f);
  
  // If confidence is above this value, it's very likely that a beat is _actually_ being detected
  CONSOLE_VAR_RANGED(float, kHighConfidenceThreshold, CONSOLE_GROUP, 0.75f, 0.01f, 20.f);
  
  // Length of time to keep a history of detected beats
  CONSOLE_VAR_RANGED(float, kBeatHistoryWindowSize_sec, CONSOLE_GROUP, 10.f, 1.f, 60.f);
  
  // Minimum number of beats that we must have accumulated in
  // the history window in order to say a beat is detected
  CONSOLE_VAR_RANGED(int, kMinNumBeatsInHistory, CONSOLE_GROUP, 6, 2, 50);
  
  // Detected tempo must be steadily within this window
  // in order to say a beat is detected
  CONSOLE_VAR_RANGED(float, kTempoSteadyThreshold_bpm, CONSOLE_GROUP, 5, 1, 25);
  
  // For detected 'potential' beats, a shorter window is used.
  CONSOLE_VAR_RANGED(float, kPossibleBeatWindow_sec, CONSOLE_GROUP, 9.f, 1.f, 10.f);
  
  // Used to fake a detected beat. If greater than 0, then a series of confident beats is created at the given bpm and
  // confidence.
  CONSOLE_VAR_RANGED(float, kFakeBeatConfidence, CONSOLE_GROUP, 0.50f,  0.f, 100.f);
  CONSOLE_VAR_RANGED(float, kFakeBeat_bpm,       CONSOLE_GROUP, -1.f, -1.f, 200.f);
  
  const float kSendWebVizDataPeriod_sec = 0.5f;
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


void BeatDetectorComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
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
      beat.confidence = kFakeBeatConfidence;
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
  
  // Send to web viz if it's time
  if (now_sec > _nextSendWebVizDataTime_sec) {
    SendToWebViz();
    _nextSendWebVizDataTime_sec = now_sec + kSendWebVizDataPeriod_sec;
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


bool BeatDetectorComponent::IsPossibleBeatDetected() const
{
  // Should always return true if a definite beat is detected
  if (IsBeatDetected()) {
    return true;
  }
  
  // In order for a possible beat to be considered detected, the following conditions must be true. For the past
  // kPossibleBeatWindow_sec seconds:
  //   - We have a minimum number of beats
  //   - Tempo has been steady for kPossibleBeatWindow_sec
  //   - The most recent beat in the history has a confidence above a minimum threshold
  //   - All beats in the past kPossibleBeatWindow_sec have a confidence of at least half the threshold

  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  const float earliestAllowedTime_sec = now_sec - kPossibleBeatWindow_sec;
  
  // Only consider beats in the past kPossibleBeatWindow_sec seconds
  auto earliestBeatIt = std::lower_bound(_recentBeats.begin(),
                                         _recentBeats.end(),
                                         BeatInfo{0.f, 0.f, earliestAllowedTime_sec},
                                         [](const BeatInfo& b1, const BeatInfo& b2){
                                           return b1.time_sec < b2.time_sec;
                                         });
  
  const auto numBeats = std::distance(earliestBeatIt, _recentBeats.end());
  const bool haveEnoughBeats = numBeats >= kMinNumBeatsInHistory;
  if (!haveEnoughBeats) {
    return false;
  }
  
  // Do all the beats in the past kPossibleBeatWindow_sec have a confidence of at least half the threshold?
  const bool allConfidenceHighEnough = std::all_of(earliestBeatIt,
                                                   _recentBeats.end(),
                                                   [](const BeatInfo& b) {
                                                     return b.confidence > 0.5f * kConfidenceThreshold;
                                                   });
  
  const bool latestConfidenceHighEnough = _recentBeats.back().confidence > kConfidenceThreshold;
  
  const auto minMaxTempoItPair = std::minmax_element(earliestBeatIt,
                                                     _recentBeats.end(),
                                                     [](const BeatInfo& b1, const BeatInfo& b2) {
                                                       return b1.tempo_bpm < b2.tempo_bpm;
                                                     });
  const bool tempoSteady = Util::IsNear(minMaxTempoItPair.second->tempo_bpm,
                                        minMaxTempoItPair.first->tempo_bpm,
                                        kTempoSteadyThreshold_bpm);
  
  return allConfidenceHighEnough && latestConfidenceHighEnough && tempoSteady;
}
  
  
bool BeatDetectorComponent::IsBeatDetected() const
{
  // In order for a _definite_ beat to be considered detected, the following conditions must be true, for the entire
  // beat history:
  //   - We have a minimum number of beats
  //   - Tempo has been steady the entire time
  //   - All beats in the history have a confidence above a minimum threshold OR the latest beat has a very high
  //     confidence.
  
  const auto numBeats = _recentBeats.size();
  const bool haveEnoughBeats = numBeats >= kMinNumBeatsInHistory;
  if (!haveEnoughBeats) {
    return false;
  }
  
  const bool confidenceHighEnough = (_recentBeats.back().confidence > kHighConfidenceThreshold) ||
                                    std::all_of(_recentBeats.begin(),
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
  if (!IsPossibleBeatDetected() || _recentBeats.empty()) {
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
  if (!IsPossibleBeatDetected() || _recentBeats.empty()) {
    PRINT_NAMED_WARNING("BeatDetectorComponent.GetCurrTempo_bpm.NoBeatDetected",
                        "No current beat is detected");
    return 0.f;
  }
  
  const auto& latestBeat = _recentBeats.back();
  return latestBeat.tempo_bpm;
}


float BeatDetectorComponent::GetCurrConfidence() const
{
  if (!IsPossibleBeatDetected() || _recentBeats.empty()) {
    PRINT_NAMED_WARNING("BeatDetectorComponent.GetCurrConfidence.NoBeatDetected",
                        "No current beat is detected");
    return 0.f;
  }
  
  const auto& latestBeat = _recentBeats.back();
  return latestBeat.confidence;
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


void BeatDetectorComponent::SendToWebViz()
{
  if (auto webSender = WebService::WebVizSender::CreateWebVizSender("beatdetector",
                                                                    _robot->GetContext()->GetWebService()) ) {
    const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
    auto& jsonData = webSender->Data();
    Json::Value detectorJson;
    detectorJson["possibleBeatDetected"] = IsPossibleBeatDetected() ? "yes" : "no";
    detectorJson["beatDetected"] = IsBeatDetected() ? "yes" : "no";
    detectorJson["latestTempo_bpm"] = _recentBeats.empty() ? -1.f : _recentBeats.back().tempo_bpm;
    detectorJson["latestConf"] = _recentBeats.empty() ? -1.f : _recentBeats.back().confidence;
    jsonData["detectorInfo"] = detectorJson;
    for (const auto& beat : _recentBeats) {
      Json::Value beatJson;
      beatJson["timeSinceBeat"] = now_sec - beat.time_sec;
      beatJson["tempo_bpm"] = beat.tempo_bpm;
      beatJson["conf"] = beat.confidence;
      beatJson["aboveThresh"] = beat.confidence > kConfidenceThreshold;
      jsonData["beatInfo"].append(beatJson);
    }
  }
}


BeatInfo BeatDetectorComponent::TEST_fakeLowConfidenceBeat(const float tempo_bpm, const float time_sec)
{
  // Create a beat with a confidence just under the threshold
  BeatInfo beat;
  beat.tempo_bpm  = tempo_bpm;
  beat.time_sec   = time_sec;
  beat.confidence = kConfidenceThreshold - 0.01f;
  return beat;
}


BeatInfo BeatDetectorComponent::TEST_fakeHighConfidenceBeat(const float tempo_bpm, const float time_sec)
{
  // Create a beat with a confidence just above the threshold
  BeatInfo beat;
  beat.tempo_bpm  = tempo_bpm;
  beat.time_sec   = time_sec;
  beat.confidence = kConfidenceThreshold + 0.01f;
  return beat;
}


BeatInfo BeatDetectorComponent::TEST_fakeVeryHighConfidenceBeat(const float tempo_bpm, const float time_sec)
{
  // Create a beat with a confidence just above the 'high' threshold
  BeatInfo beat;
  beat.tempo_bpm  = tempo_bpm;
  beat.time_sec   = time_sec;
  beat.confidence = kHighConfidenceThreshold + 0.01f;
  return beat;
}
  
} // namespace Vector
} // namespace Anki

