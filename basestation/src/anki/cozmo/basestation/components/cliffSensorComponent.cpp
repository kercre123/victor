/**
 * File: cliffSensorComponent.cpp
 *
 * Author: Matt Michini
 * Created: 6/27/2017
 *
 * Description: Component for managing cliff sensors and cliff sensor-related functions such as carpet detection (and
 *              corresponding cliff threshold adjustment), 'suspicious' cliff detection, and raw data access.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/components/cliffSensorComponent.h"

#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/logging/rollingFileLogger.h"

namespace Anki {
namespace Cozmo {

// Whether or not to lower the cliff detection threshold on the robot
// whenever a suspicious cliff is encountered.
CONSOLE_VAR(bool, kDoProgressiveThresholdAdjustOnSuspiciousCliff, "CliffSensors", true);
  
namespace {

// Minimum value that cliff detection threshold can be dynamically lowered to
const u32 kCliffSensorMinDetectionThresh = 150;

// Amount by which cliff detection level can be lowered when suspiciously cliff-y floor detected
const u32 kCliffSensorDetectionThreshStep = 250;

// Number of suspicious cliffs that must be encountered before detection threshold is lowered
const u32 kCliffSensorSuspiciousCliffCount = 1;

// Size of running window of cliff data for computing running mean/variance in number of RobotState messages (which arrive every 30ms)
const u32 kCliffSensorRunningStatsWindowSize = 100;

// Cliff variance threshold for detecting suspiciously cliffy floor/carpet
const f32 kCliffSensorSuspiciouslyCliffyFloorThresh = 10000;

// The minimum amount by which the cliff data must be above the minimum value observed since stopping
// began in order to be considered a suspicious cliff. (i.e. We might be on crazy carpet)
const u16 kMinRiseDuringStopForSuspiciousCliff = 15;

const std::string kLogDirectory = "sensorData/cliffSensors";
  
} // end anonymous namespace

  
CliffSensorComponent::CliffSensorComponent(Robot& robot)
  : _robot(robot)
  , _cliffDetectThreshold(CLIFF_SENSOR_DROP_LEVEL)
{
  _cliffDataRaw.fill(std::numeric_limits<uint16_t>::max());
}
  
CliffSensorComponent::~CliffSensorComponent() = default;
  
void CliffSensorComponent::UpdateRobotData(const RobotState& msg)
{
  _cliffDataRaw = msg.cliffDataRaw;
  
  _cliffDetectedStatusBitOn = (msg.status & (uint32_t)RobotStatusFlag::CLIFF_DETECTED) != 0;
  
  _lastMsgTimestamp = msg.timestamp;
  
  // Log stuff if appropriate:
  if (_loggingRawData) {
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (now < _logRawDataUntil_s) {
      LogRawData();
    } else {
      _loggingRawData = false;
      _logRawDataUntil_s = 0.f;
    }
  }
}
  
void CliffSensorComponent::SendCliffDetectThresholdToRobot(const u16 thresh)
{
  if (thresh != _cliffDetectThreshold) {
    PRINT_NAMED_INFO("CliffSensorComponent.SendCliffDetectThresholdToRobot.CliffDetectThresholdUpdated", "New cliff detect threshold %d (old threshold %d)", thresh, _cliffDetectThreshold);
    _cliffDetectThreshold = thresh;
  }
  
  _robot.SendRobotMessage<RobotInterface::SetCliffDetectThreshold>(thresh);
}

u16 CliffSensorComponent::GetCliffDataRaw(unsigned int ind) const
{
  DEV_ASSERT(ind < GetNumCliffSensors(), "CliffSensorComponent.GetCliffDataRaw.InvalidIndex");
  
  return _cliffDataRaw[ind];
}

void CliffSensorComponent::UpdateCliffDetectThreshold()
{
  // Check pose history for cliff readings to see if cliff readings went up
  // after stopping indicating a suspicious cliff.
  if (_cliffStartTimestamp > 0) {
    if( !_robot.GetMoveComponent().AreWheelsMoving() ) {
      
      auto poseMap = _robot.GetStateHistory()->GetRawPoses();
      u16 minVal = std::numeric_limits<u16>::max();
      for (auto startIt = poseMap.lower_bound(_cliffStartTimestamp); startIt != poseMap.end(); ++startIt) {
        u16 currVal = startIt->second.GetCliffData();
        PRINT_NAMED_DEBUG("CliffSensorComponent.UpdateCliffDetectThreshold.CliffValueWhileStopping", "%d - %d", startIt->first, currVal);
        
        if (minVal > currVal) {
          minVal = currVal;
        }
        
        // If a cliff reading is ever more than a certain amount above the minimum observed value since
        // it began stopping, the cliff it is reacting to is suspect (probably carpet) so don't bother doing the reaction.
        // Note: This is mostly from testing on the office carpet.
        if (IsFloorSuspiciouslyCliffy() && (currVal > minVal + kMinRiseDuringStopForSuspiciousCliff)) {
          IncrementSuspiciousCliffCount();
        }
      }
      
      _cliffStartTimestamp = 0;
    }
  }
}
  
// Updates mean and variance of cliff readings observed in last kCliffSensorRunningStatsWindowSize RobotState msgs.
// Based on Welford Algorithm. See http://stackoverflow.com/questions/5147378/rolling-variance-algorithm
void CliffSensorComponent::UpdateCliffRunningStats(const RobotState& msg)
{
  // TODO: update this to support multiple cliff sensors
  // Note: This may not be necessary for V2 cliff sensors since they don't exhibit as much variance on carpets
  u16 obs = msg.cliffDataRaw[0];
  if (_robot.GetMoveComponent().AreWheelsMoving() && (_robot.GetOffTreadsState() == OffTreadsState::OnTreads) && obs > (_cliffDetectThreshold)) {
    
    _cliffDataQueue.push_back(obs);
    if (_cliffDataQueue.size() > kCliffSensorRunningStatsWindowSize) {
      // Popping oldest value. Update stats.
      f32 oldestObs = _cliffDataQueue.front();
      _cliffDataQueue.pop_front();
      f32 prevMean = _cliffRunningMean;
      _cliffRunningMean += (obs - oldestObs) / kCliffSensorRunningStatsWindowSize;
      _cliffRunningVar_acc += (obs - prevMean) * (obs - _cliffRunningMean) - (oldestObs - prevMean) * (oldestObs - _cliffRunningMean);
    } else {
      // Queue not full yet. Just update stats
      f32 delta = obs - _cliffRunningMean;
      _cliffRunningMean += delta / _cliffDataQueue.size();
      _cliffRunningVar_acc += delta * (obs - _cliffRunningMean);
    }
    
    // Compute running variance based on number of samples in queue/window
    if (_cliffDataQueue.size() > 1) {
      _cliffRunningVar = _cliffRunningVar_acc / (_cliffDataQueue.size() - 1);
    }
  }
}

void CliffSensorComponent::ClearCliffRunningStats()
{
  // Reset cliff threshold
  _suspiciousCliffCnt = 0;
  if (_cliffDetectThreshold != CLIFF_SENSOR_DROP_LEVEL) {
    _cliffDetectThreshold = CLIFF_SENSOR_DROP_LEVEL;
    LOG_EVENT("CliffSensorComponent.ClearCliffRunningStats.RestoringCliffDetectThreshold", "%d", _cliffDetectThreshold);
    SendCliffDetectThresholdToRobot(_cliffDetectThreshold);
  }
  
  _cliffDataQueue.clear();
  _cliffRunningMean = 0;
  _cliffRunningVar = 0;
  _cliffRunningVar_acc = 0;
}
  
void CliffSensorComponent::EvaluateCliffSuspiciousnessWhenStopped()
{
  if (kDoProgressiveThresholdAdjustOnSuspiciousCliff) {
    _cliffStartTimestamp = _robot.GetLastMsgTimestamp();
  }
}

// The variance of the recent cliff readings is what we use to determine the cliffyness of the floor
bool CliffSensorComponent::IsFloorSuspiciouslyCliffy() const
{
  return (_cliffRunningVar > kCliffSensorSuspiciouslyCliffyFloorThresh);
}
  
void CliffSensorComponent::IncrementSuspiciousCliffCount()
{
  if (_cliffDetectThreshold > kCliffSensorMinDetectionThresh) {
    ++_suspiciousCliffCnt;
    LOG_EVENT("IncrementSuspiciousCliffCount.Count", "%d", _suspiciousCliffCnt);
    if (_suspiciousCliffCnt >= kCliffSensorSuspiciousCliffCount) {
      _cliffDetectThreshold = MAX(_cliffDetectThreshold - kCliffSensorDetectionThreshStep, kCliffSensorMinDetectionThresh);
      LOG_EVENT("IncrementSuspiciousCliffCount.NewThreshold", "%d", _cliffDetectThreshold);
      SendCliffDetectThresholdToRobot(_cliffDetectThreshold);
      _suspiciousCliffCnt = 0;
    }
  }
}
  
void CliffSensorComponent::EnableRawDataLogging(const uint32_t duration_ms)
{
  if (!_loggingRawData) {
    _loggingRawData = true;
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _logRawDataUntil_s = now + static_cast<float>(duration_ms)/1000.f;
    PRINT_NAMED_INFO("CliffSensorComponent.EnableRawDataLogging", "Starting raw cliff data logging, duration %d ms (log will appear in '%s')",
                     duration_ms,
                     _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory).c_str());
  }
}

void CliffSensorComponent::LogRawData()
{
  // Instantiate the logger if it doesn't exist yet
  if (_rawDataLogger == nullptr) {
    _rawDataLogger = std::make_unique<Util::RollingFileLogger>(nullptr, _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory));
  }
  
  std::string str;
  str += std::to_string(GetLastMsgTimestamp()) + ", ";
  const auto nSensors = GetNumCliffSensors();
  for (int i=0 ; i < nSensors ; i++) {
    str += std::to_string(GetCliffDataRaw(i));
    // comma separate
    if (i < nSensors - 1) {
      str += ", ";
    }
  }
  str += "\n";
  _rawDataLogger->Write(str);
}
  
bool CliffSensorComponent::ComputeCliffPose(const CliffEvent& cliffEvent, Pose3d& cliffPose) const
{
  if (cliffEvent.detectedFlags == 0) {
    PRINT_NAMED_WARNING("CliffSensorComponent.ComputeCliffPose.NoCliff",
                        "CliffEvent::detectedFlags == 0! Can't compute a cliff pose if there was no cliff detected.");
    return false;
  }
  
  // Grab historical state at the time the cliff was detected
  HistRobotState histState;
  TimeStamp_t histTimestamp;
  const bool useInterp = true;
  const auto& res = _robot.GetStateHistory()->ComputeStateAt(cliffEvent.timestamp, histTimestamp, histState, useInterp);
  if (res != RESULT_OK) {
    PRINT_NAMED_ERROR("CliffSensorComponent.ComputeCliffPose.NoHistoricalPose",
                      "Could not retrieve historical pose for timestamp %u",
                      cliffEvent.timestamp);
    return false;
  }
  
  const auto& robotPoseAtCliff = histState.GetPose();
  
#ifdef COZMO_V2
  // The cliff pose depends on which cliff sensors were tripped
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
  
  // Estimate the cliff's pose with respect to the robot frame of reference
  Pose3d cliffWrtRobot;
  cliffWrtRobot.SetParent(&robotPoseAtCliff);
  switch (cliffEvent.detectedFlags) {
    case (FL | FR):
      // Hit cliff straight-on
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetFront_mm, 0.f, 0.f});
      cliffWrtRobot.SetRotation(0.f, Z_AXIS_3D());
      break;
    case FL:
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetFront_mm, kCliffSensorYOffset_mm, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(45.f), Z_AXIS_3D());
      break;
    case FR:
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetFront_mm, -kCliffSensorYOffset_mm, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(-45.f), Z_AXIS_3D());
      break;
    case BL:
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetRear_mm, kCliffSensorYOffset_mm, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(135.f), Z_AXIS_3D());
      break;
    case BR:
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetRear_mm, -kCliffSensorYOffset_mm, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(-135.f), Z_AXIS_3D());
      break;
    case (FL | BL):
      cliffWrtRobot.SetTranslation({(kCliffSensorXOffsetFront_mm + kCliffSensorXOffsetRear_mm)/2.f, kCliffSensorYOffset_mm, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(90.f), Z_AXIS_3D());
      break;
    case (FR | BR):
      cliffWrtRobot.SetTranslation({(kCliffSensorXOffsetFront_mm + kCliffSensorXOffsetRear_mm)/2.f, -kCliffSensorYOffset_mm, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(-90.f), Z_AXIS_3D());
      break;
    case (BL | BR):
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetRear_mm, 0.f, 0.f});
      cliffWrtRobot.SetRotation(DEG_TO_RAD(180.f), Z_AXIS_3D());
      break;
    default:
      // Improbable combination of cliff sensors - just assume robot hit cliff straight-on
      cliffWrtRobot.SetTranslation({kCliffSensorXOffsetFront_mm, 0.f, 0.f});
      cliffWrtRobot.SetRotation(0.f, Z_AXIS_3D());
      break;
  }

  // Compute the cliff pose with respect to the robot world origin
  if (!cliffWrtRobot.GetWithRespectTo(*_robot.GetWorldOrigin(), cliffPose)) {
    PRINT_NAMED_ERROR("CliffSensorComponent.ComputeCliffPose.OriginMismatch",
                      "cliffWrtRobot and robot.GetWorldOrigin() do not share the same origin!");
    return false;
  }
  
#else
  // For pre-V2 robots, the cliff pose is just considered to be the robot's pose at the time of the cliff event
  cliffPose = robotPoseAtCliff;
#endif // COZMO_V2
  
  return true;
}

  
} // Cozmo namespace
} // Anki namespace
