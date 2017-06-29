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
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

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

} // end anonymous namespace

  
CliffSensorComponent::CliffSensorComponent(Robot& robot)
  : _robot(robot)
  , _cliffDetectThreshold(CLIFF_SENSOR_DROP_LEVEL)
{
  _cliffDataRaw.fill(std::numeric_limits<uint16_t>::max());
}
  
void CliffSensorComponent::UpdateRobotData(const RobotState& msg)
{
  _cliffDataRaw = msg.cliffDataRaw;
  
  _cliffDetectedStatusBitOn = (msg.status & (uint32_t)RobotStatusFlag::CLIFF_DETECTED) != 0;
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
#ifdef COZMO_V2
  DEV_ASSERT(ind < Util::EnumToUnderlying(CliffSensor::CLIFF_COUNT), "CliffSensorComponent.GetCliffDataRaw.InvalidIndex");
#else
  // For pre-V2 robots, there is only one cliff sensor, so index should be 0.
  DEV_ASSERT(ind == 0, "CliffSensorComponent.GetCliffDataRaw.InvalidIndex");
#endif // COZMO_V2
  
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

  
} // Cozmo namespace
} // Anki namespace
