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

#include "engine/components/sensors/cliffSensorComponent.h"

#include "engine/ankiEventUtil.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "engine/robot.h"
#include "engine/robotStateHistory.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/helpers/ankiDefines.h"
#include "util/logging/logging.h"

#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#define LOG_CHANNEL "CliffSensor"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kLogDirectory = "cliffSensors";
  
  // IIR filter coefficient used for filtering raw cliff sensor data
  const f32 kCliffFiltCoef = 0.75f;
  
  // The maximum rate at which cliff sensor threshold updates will be sent to the robot.
  const int kCliffThresholdMaxUpdateRate_ms = 1000;
  
  // The default allowed delta above the minimum-ever-seen cliff sensor value before cliff
  // detection is triggered.
  const u16 kCliffDetectAllowedDeltaDefault = 35;
  
  // This value is used whenever it seems like we may be driving on a dark surface, to reduce
  // the chance of false cliff detections.
  const u16 kCliffDetectAllowedDeltaLow = 20;
  
  // If all cliff sensors are reporting below this value, then we reduce the AllowedDelta to prevent
  // cliff detection false positives. Likewise if all cliff sensors are _above_ this value, the AllowedDelta
  // is restored to the default.
  const u16 kCliffValDarkSurface = 250;
  
  // Also reduce the AllowedDelta if the robot could be driving over something (i.e. its pitch angle
  // is sufficiently far from zero)
  const Radians kRobotPitchThresholdPossibleCliff_rad = DEG_TO_RAD(7.f);
}


CliffSensorComponent::CliffSensorComponent() 
: ISensorComponent(kLogDirectory)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::CliffSensor)
, _cliffDetectAllowedDelta(kCliffDetectAllowedDeltaDefault)
{
  _cliffDataRaw.fill(std::numeric_limits<uint16_t>::max());
  _cliffDataRawAtLastStop.fill(std::numeric_limits<uint16_t>::max());
  _cliffMinObserved.fill(std::numeric_limits<uint16_t>::max());
  _cliffDataFilt.fill(0.f);
  _cliffDetectThresholds.fill(CLIFF_SENSOR_THRESHOLD_DEFAULT);
  _cliffDetectionTimes_ms.fill(0);
}

void CliffSensorComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  InitBase(robot);
  if (_robot->HasExternalInterface())
  {
    InitEventHandlers(*(_robot->GetExternalInterface()));
  }
}

void CliffSensorComponent::InitEventHandlers(IExternalInterface& interface)
{
  auto helper = MakeAnkiEventUtil(interface, *this, _eventHandles);
  
  // Subscribe to relevant events, listed in alphabetical order.
  helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged>();
  helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::RobotStopped>();
}
  
void CliffSensorComponent::HandleMessage(const ExternalInterface::RobotStopped& msg)
{
  if (msg.reason == StopReason::CLIFF) {
    _lastStopDueToCliffTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    _cliffDataRawAtLastStop = GetCliffDataRaw();
    LOG_INFO("CliffSensorComponent.RobotStoppedOnCliff", "");
    
    // Reset the recorded data for the last event where the robot was put down on a cliff.
    _lastPutDownOnCliffTime_ms = 0;
  }
}

void CliffSensorComponent::HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged &msg)
{
  if (msg.treadsState == OffTreadsState::OnTreads && IsCliffDetected()) {
    _lastPutDownOnCliffTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    LOG_INFO("CliffSensorComponent.RobotPutDownOnCliff", "");
    
    // Reset the recorded data for the last event where the robot stopped for a cliff.
    _lastStopDueToCliffTime_ms = 0;
    _cliffDataRaw.fill(std::numeric_limits<uint16_t>::max());
  }
}

void CliffSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  // Update raw and filtered cliff sensor data
  for (int i=0 ; i<kNumCliffSensors ; i++) {
    _cliffDataRaw[i] = msg.cliffDataRaw[i];
    _cliffDataFilt[i] = Util::IsNearZero(_cliffDataFilt[i]) ?
                        _cliffDataRaw[i] :
                        (kCliffFiltCoef * _cliffDataFilt[i]) + ((1.f - kCliffFiltCoef) * static_cast<float>(_cliffDataRaw[i]));
  }
  
  _cliffDetectedFlags = msg.cliffDetectedFlags;
  _whiteDetectedFlags = msg.whiteDetectedFlags;
  
  _latestMsgTimestamp = msg.timestamp;
  
  UpdateLatestCliffDetectionDuration();
  
  const bool isPickedUp = (msg.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP) != 0;
  _maxNumCliffsDetectedWhilePickedUp = isPickedUp ? std::max(_maxNumCliffsDetectedWhilePickedUp, _latestNumCliffsDetected) : 0;

  UpdateCliffDetectThresholds();
  
  // Send new thresholds to the robot if needed:
  if (_nextCliffThresholdUpdateToRobot_ms != 0 &&
      msg.timestamp >= _nextCliffThresholdUpdateToRobot_ms) {
    SendCliffDetectThresholdsToRobot();
    _nextCliffThresholdUpdateToRobot_ms = 0;
  }

  // Check if StopOnWhite should be auto disabled due to pickup
  if (_stopOnWhiteEnabled && _robot->IsPickedUp()) {
    LOG_INFO("CliffSensorComponent.NotifyOfRobotStateInternal.AutoDisableStopOnWhiteDueToPickup", "");
    EnableStopOnWhite(false);
  }
}


std::string CliffSensorComponent::GetLogHeader()
{
  return std::string("timestamp_ms, cliffFL, cliffFR, cliffBL, cliffBR");
}


std::string CliffSensorComponent::GetLogRow()
{
  std::string str;
  str += std::to_string(_latestMsgTimestamp);
  str += ", ";
  for (int i=0 ; i < kNumCliffSensors ; i++) {
    str += std::to_string(_cliffDataRaw[i]);
    // comma separate
    if (i < kNumCliffSensors - 1) {
      str += ", ";
    }
  }
  return str;
}

void CliffSensorComponent::UpdateCliffDetectThresholds()
{
  // Estimate whether or not we are on a dark surface and adjust the cliff allowedDelta to reduce
  // false positive cliff detections. Note that this will also reduce the allowedDelta while the robot
  // is in the air, but that is fine since we don't know what kind of surface he'll be on when he's
  // placed down on his treads again. If it's a bright surface, the allowedDelta will automatically
  // increase again.
  const auto minMaxCliffItPair = std::minmax_element(std::begin(_cliffDataRaw), std::end(_cliffDataRaw));
  const auto minCliffVal = *minMaxCliffItPair.first;
  const auto maxCliffVal = *minMaxCliffItPair.second;
  const bool IsRobotSittingFlat = _robot->GetPitchAngle().IsNear(0.f, kRobotPitchThresholdPossibleCliff_rad);
  if (maxCliffVal < kCliffValDarkSurface || !IsRobotSittingFlat) {
    // All cliff values are below the dark threshold (or IMU is saying we might be driving over something),
    // so use a reduced allowedDelta to reduce false cliff detections.
    _cliffDetectAllowedDelta = kCliffDetectAllowedDeltaLow;
  } else if (minCliffVal >= kCliffValDarkSurface) {
    // All cliff values are above the dark threshold and we are sitting flat, so reset the allowedDelta back to the default.
    _cliffDetectAllowedDelta = kCliffDetectAllowedDeltaDefault;
  }
  
  // Update minimum observed cliff sensor values and cliff thresholds (using filtered data):
  for (int i=0 ; i < kNumCliffSensors ; i++) {
    _cliffMinObserved[i] = std::min(_cliffMinObserved[i], static_cast<uint16_t>(_cliffDataFilt[i]));
    uint16_t newThresh = _cliffMinObserved[i] + _cliffDetectAllowedDelta;
    newThresh = Util::Clamp(newThresh, CLIFF_SENSOR_THRESHOLD_MIN, CLIFF_SENSOR_THRESHOLD_MAX);
    if (newThresh != _cliffDetectThresholds[i]) {
      SetCliffDetectThreshold(i, newThresh);
    }
  }
}


void CliffSensorComponent::QueueCliffThresholdUpdate()
{
  if (_nextCliffThresholdUpdateToRobot_ms == 0) {
    _nextCliffThresholdUpdateToRobot_ms = _latestMsgTimestamp + kCliffThresholdMaxUpdateRate_ms;
  }
}


void CliffSensorComponent::SendCliffDetectThresholdsToRobot()
{
  if(_isPaused)
  {
    return;
  }
  
  LOG_DEBUG("CliffSensorComponent.SendCliffDetectThresholdsToRobot.SendThresholds",
            "New cliff thresholds being sent to robot: %d %d %d %d",
            _cliffDetectThresholds[0], _cliffDetectThresholds[1], _cliffDetectThresholds[2], _cliffDetectThresholds[3]);
  
  _robot->SendRobotMessage<SetCliffDetectThresholds>(_cliffDetectThresholds);
  
#if defined(SIMULATOR)
  // Also send to game (for webots tests)
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(SetCliffDetectThresholds(_cliffDetectThresholds)));
#endif
}


u16 CliffSensorComponent::GetCliffDetectThreshold(unsigned int ind) const
{
  DEV_ASSERT(ind < kNumCliffSensors, "CliffSensorComponent.GetCliffDetectThreshold.InvalidIndex");
  return _cliffDetectThresholds[ind];
}

void CliffSensorComponent::SetCliffDetectThreshold(unsigned int ind, uint16_t newThresh)
{
  DEV_ASSERT(ind < kNumCliffSensors, "CliffSensorComponent.SetCliffDetectThreshold.InvalidIndex");

  DEV_ASSERT(newThresh >= CLIFF_SENSOR_THRESHOLD_MIN || newThresh <= CLIFF_SENSOR_THRESHOLD_MAX, "CliffSensorComponent.SetCliffDetectThreshold.OutOfRange");
  
  newThresh = Util::Clamp(newThresh, CLIFF_SENSOR_THRESHOLD_MIN, CLIFF_SENSOR_THRESHOLD_MAX);
  
  auto& curThresh = _cliffDetectThresholds[ind];
  if (curThresh != newThresh) {
    LOG_DEBUG("CliffSensorComponent.SetCliffDetectThreshold.NewThreshold",
              "New cliff threshold for %s (old: %d, new %d). Message to robot queued.",
              EnumToString(static_cast<CliffSensor>(ind)),
              curThresh,
              newThresh);
    curThresh = newThresh;
    QueueCliffThresholdUpdate();
  }
}
  
void CliffSensorComponent::UpdateLatestCliffDetectionDuration()
{
  // Cache the latest number of cliffs detected to compare later whether it changes after the recount
  const int prevNumCliffsDetected = _latestNumCliffsDetected;

  // Count the number of flags/bits set
  _latestNumCliffsDetected = __builtin_popcount(_cliffDetectedFlags.GetFlags());

  const TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  _cliffLastDetectedTimes_ms[_latestNumCliffsDetected] = currTime;
  
  if ( _latestNumCliffsDetected != prevNumCliffsDetected ) {  
    
    // When the number of cliffs detected increases, update all start times for entries
    // for cliff detections greater than the previous number seen, up to the current level.
    // Otherwise, this function call does nothing when the number of cliffs decreases.
    std::fill_n(_cliffDetectionTimes_ms.begin() + prevNumCliffsDetected + 1,
                _latestNumCliffsDetected - prevNumCliffsDetected, currTime);
    
    // When we stop detecting any cliffs, the above call won't work to populate the
    // zeroth entry in the array of start times, so we must update it manually.
    if ( _latestNumCliffsDetected == 0 ) {
      _cliffDetectionTimes_ms[_latestNumCliffsDetected] = currTime;
    }
    
    // When the number of cliffs detected decreases, reset all start times for entries
    // that are tracking cliff detections greater than the current level.
    // Otherwise, this function call does nothing when the number of cliffs increases.
    std::fill_n(_cliffDetectionTimes_ms.begin() + _latestNumCliffsDetected + 1,
                prevNumCliffsDetected - _latestNumCliffsDetected, 0);
    
    // When we start detecting any cliffs at all, we don't want the start time of the zero-cliff-detection
    // slot to remain at zero, since it's not true that zero cliffs are still being detected.
    if ( prevNumCliffsDetected == 0 ) {
      _cliffDetectionTimes_ms[0] = 0;
    }
  }
}
  
u32 CliffSensorComponent::GetDurationForAtLeastNCliffDetections_ms(const int minNumCliffs) const
{
  DEV_ASSERT(minNumCliffs >= 0 && minNumCliffs <= kNumCliffSensors,
             "CliffSensorComponent.GetDurationForAtLeastNCliffDetections.InvalidNumCliffs");
  
  const u32 cliffDetectionStartTime = _cliffDetectionTimes_ms[minNumCliffs];
  const TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  return (cliffDetectionStartTime > 0) && (cliffDetectionStartTime < currTime) ?
         (currTime - cliffDetectionStartTime) : 0u;
}

u32 CliffSensorComponent::GetDurationForAtMostNCliffDetections_ms(const int numCliffs) const
{
  DEV_ASSERT(numCliffs >= 0 && numCliffs <= kNumCliffSensors,
             "CliffSensorComponent.GetDurationForAtMostNCliffDetections_ms.InvalidNumCliffs");

  const TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  u32 maxDuration_ms = std::numeric_limits<u32>::max();
  if (numCliffs == kNumCliffSensors) {
    return maxDuration_ms;
  }
  for (int i=numCliffs+1; i<=kNumCliffSensors; ++i) {
    const u32 duration_ms = currTime - _cliffLastDetectedTimes_ms[i];
    if (maxDuration_ms > duration_ms) {
      maxDuration_ms = duration_ms;
    }
  }
  return maxDuration_ms;
}

bool CliffSensorComponent::GetCliffPoseRelativeToRobot(const uint8_t cliffDetectedFlags, Pose3d& relativePose) const
{
  // The cliff pose depends on which cliff sensors were tripped
  // Bit flags for each of the cliff sensors:
  static const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  static const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  static const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  static const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
  
  // Estimate the cliff's pose with respect to the robot frame of reference
  switch (cliffDetectedFlags) {
    case (FL | FR):
      // Hit cliff straight-on
      relativePose.SetTranslation({kCliffSensorXOffsetFront_mm, 0.f, 0.f});
      relativePose.SetRotation(0.f, Z_AXIS_3D());
      break;
    case FL:
      relativePose.SetTranslation({kCliffSensorXOffsetFront_mm, kCliffSensorYOffset_mm, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(45.f), Z_AXIS_3D());
      break;
    case FR:
      relativePose.SetTranslation({kCliffSensorXOffsetFront_mm, -kCliffSensorYOffset_mm, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(-45.f), Z_AXIS_3D());
      break;
    case BL:
      relativePose.SetTranslation({kCliffSensorXOffsetRear_mm, kCliffSensorYOffset_mm, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(135.f), Z_AXIS_3D());
      break;
    case BR:
      relativePose.SetTranslation({kCliffSensorXOffsetRear_mm, -kCliffSensorYOffset_mm, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(-135.f), Z_AXIS_3D());
      break;
    case (FL | BL):
      relativePose.SetTranslation({(kCliffSensorXOffsetFront_mm + kCliffSensorXOffsetRear_mm)/2.f, kCliffSensorYOffset_mm, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(90.f), Z_AXIS_3D());
      break;
    case (FR | BR):
      relativePose.SetTranslation({(kCliffSensorXOffsetFront_mm + kCliffSensorXOffsetRear_mm)/2.f, -kCliffSensorYOffset_mm, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(-90.f), Z_AXIS_3D());
      break;
    case (BL | BR):
      relativePose.SetTranslation({kCliffSensorXOffsetRear_mm, 0.f, 0.f});
      relativePose.SetRotation(DEG_TO_RAD(180.f), Z_AXIS_3D());
      break;
    default:
      // Improbable combination of cliff sensors - just assume robot hit cliff straight-on
      relativePose.SetTranslation({kCliffSensorXOffsetFront_mm, 0.f, 0.f});
      relativePose.SetRotation(0.f, Z_AXIS_3D());
      return false;
  }

  return true;
}

bool CliffSensorComponent::ComputeCliffPose(uint32_t timestampOfCliff, uint8_t cliffDetectedFlags, Pose3d& cliffPose) const
{
  if (cliffDetectedFlags == 0) {
    LOG_WARNING("CliffSensorComponent.ComputeCliffPose.NoCliff",
                "CliffEvent::detectedFlags == 0! Can't compute a cliff pose if there was no cliff detected.");
    return false;
  }
  
  // Grab historical state at the time the cliff was detected
  HistRobotState histState;
  RobotTimeStamp_t histTimestamp;
  const bool kUseInterp = true;
  const auto& res = _robot->GetStateHistory()->ComputeStateAt(timestampOfCliff, histTimestamp, histState, kUseInterp);
  if (res != RESULT_OK) {
    LOG_ERROR("CliffSensorComponent.ComputeCliffPose.NoHistoricalPose",
              "Could not retrieve historical pose for timestamp %u",
              timestampOfCliff);
    return false;
  }
  
  const auto& robotPoseAtCliff = histState.GetPose();
  
  Pose3d cliffWrtRobot;
  const bool isValidPose = GetCliffPoseRelativeToRobot(cliffDetectedFlags, cliffWrtRobot);
  if(!isValidPose) {
    LOG_INFO("CliffSensorComponent.ComputeCliffPose.NoPoseForCliffFlags", "flags=%hhu", cliffDetectedFlags);
    return false;
  }

  // Compute the cliff pose with respect to the robot world origin
  cliffWrtRobot.SetParent(robotPoseAtCliff);
  if (!cliffWrtRobot.GetWithRespectTo(_robot->GetWorldOrigin(), cliffPose)) {
    LOG_ERROR("CliffSensorComponent.ComputeCliffPose.OriginMismatch",
              "cliffWrtRobot and robot.GetWorldOrigin() do not share the same origin!");
    return false;
  }

  return true;
}

void CliffSensorComponent::UpdateNavMapWithCliffAt(const Pose3d& pose, const uint32_t timestamp) const
{
  Point2f cliffSize{10.f, ROBOT_BOUNDING_Y};
  Quad2f cliffquad {
    { +cliffSize.x(), +cliffSize.y() * .5f },  // up L
    { 0.f,            +cliffSize.y() * .5f },  // lo L
    { +cliffSize.x(), -cliffSize.y() * .5f },  // up R
    { 0.f,            -cliffSize.y() * .5f }}; // lo R
  ((Pose2d) pose).ApplyTo(cliffquad, cliffquad);

  MemoryMapData_Cliff cliffFromSensor(pose, timestamp);
  cliffFromSensor.isFromCliffSensor = true;
  _robot->GetMapComponent().InsertData(Poly2f(cliffquad), cliffFromSensor);
}

void CliffSensorComponent::EnableStopOnWhite(bool stopOnWhite)
{
  if (_stopOnWhiteEnabled != stopOnWhite) {
    if (stopOnWhite && _robot->IsPickedUp()) {
      LOG_WARNING("CliffSensorComponent.EnableStopOnWhite.IgnoredDueToPickup", "");
    } else {
      LOG_INFO("CliffSensorComponent.EnableStopOnWhite", "%d", stopOnWhite);
      _stopOnWhiteEnabled = stopOnWhite;
      _robot->SendRobotMessage<RobotInterface::EnableStopOnWhite>(stopOnWhite);
    }
  }
}
  
} // Vector namespace
} // Anki namespace
