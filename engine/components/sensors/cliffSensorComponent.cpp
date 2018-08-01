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

#include "engine/components/movementComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "engine/markerlessObject.h"
#include "engine/robot.h"
#include "engine/robotStateHistory.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include "coretech/common/engine/math/polygon_impl.h"

namespace Anki {
namespace Cozmo {
  
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
  const Radians kRobotPitchThresholdPossibleCliff_rad = DEG_TO_RAD(5.f);
  
}


CliffSensorComponent::CliffSensorComponent() 
: ISensorComponent(kLogDirectory)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::CliffSensor)
, _cliffDetectAllowedDelta(kCliffDetectAllowedDeltaDefault)
{
  _cliffDataRaw.fill(std::numeric_limits<uint16_t>::max());
  _cliffMinObserved.fill(std::numeric_limits<uint16_t>::max());
  _cliffDataFilt.fill(0.f);
  _cliffDetectThresholds.fill(CLIFF_SENSOR_THRESHOLD_DEFAULT);
}


void CliffSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  // Update raw and filtered cliff sensor data
  for (int i=0 ; i<kNumCliffSensors ; i++) {
    _cliffDataRaw[i] = msg.cliffDataRaw[i];
    if (_cliffDataRaw[i] == 0) {
      PRINT_NAMED_WARNING("CliffSensorComponent.UpdateInternal.RawCliffZero", "Raw cliff data for sensor %d is zero! Discarding.", i);
    } else {
      // Update filtered values (and initialize it if needed)
      _cliffDataFilt[i] = Util::IsNearZero(_cliffDataFilt[i]) ?
                          _cliffDataRaw[i] :
                          (kCliffFiltCoef * _cliffDataFilt[i]) + ((1.f - kCliffFiltCoef) * static_cast<float>(_cliffDataRaw[i]));
    }
  }
  _cliffDetectedFlags = msg.cliffDetectedFlags;
  _whiteDetectedFlags = msg.whiteDetectedFlags;
  
  _lastMsgTimestamp = msg.timestamp;
  
  UpdateCliffDetectThresholds();
  
  // Send new thresholds to the robot if needed:
  if (_nextCliffThresholdUpdateToRobot_ms != 0 &&
      msg.timestamp >= _nextCliffThresholdUpdateToRobot_ms) {
    SendCliffDetectThresholdsToRobot();
    _nextCliffThresholdUpdateToRobot_ms = 0;
  }

  // Check if StopOnWhite should be auto disabled due to pickup
  if (_stopOnWhiteEnabled && _robot->IsPickedUp()) {
    PRINT_NAMED_INFO("CliffSensorComponent.NotifyOfRobotStateInternal.AutoDisableStopOnWhiteDueToPickup", "");
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
  str += std::to_string(_lastMsgTimestamp);
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
    _nextCliffThresholdUpdateToRobot_ms = _lastMsgTimestamp + kCliffThresholdMaxUpdateRate_ms;
  }
}


void CliffSensorComponent::SendCliffDetectThresholdsToRobot()
{
  if(_isPaused)
  {
    return;
  }
  
  PRINT_NAMED_INFO("CliffSensorComponent.SendCliffDetectThresholdsToRobot.SendThresholds",
                   "New cliff thresholds being sent to robot: %d %d %d %d",
                   _cliffDetectThresholds[0], _cliffDetectThresholds[1], _cliffDetectThresholds[2], _cliffDetectThresholds[3]);
  
  _robot->SendRobotMessage<SetCliffDetectThresholds>(_cliffDetectThresholds);
  
  // Also send to game (for webots tests)
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(SetCliffDetectThresholds(_cliffDetectThresholds)));
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
    PRINT_NAMED_INFO("CliffSensorComponent.SetCliffDetectThreshold.NewThreshold",
                     "New cliff threshold for %s (old: %d, new %d). Message to robot queued.",
                     EnumToString(static_cast<CliffSensor>(ind)),
                     curThresh,
                     newThresh);
    curThresh = newThresh;
    QueueCliffThresholdUpdate();
  }
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
  RobotTimeStamp_t histTimestamp;
  const bool useInterp = true;
  const auto& res = _robot->GetStateHistory()->ComputeStateAt(cliffEvent.timestamp, histTimestamp, histState, useInterp);
  if (res != RESULT_OK) {
    PRINT_NAMED_ERROR("CliffSensorComponent.ComputeCliffPose.NoHistoricalPose",
                      "Could not retrieve historical pose for timestamp %u",
                      cliffEvent.timestamp);
    return false;
  }
  
  const auto& robotPoseAtCliff = histState.GetPose();
  
  // The cliff pose depends on which cliff sensors were tripped
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
  
  // Estimate the cliff's pose with respect to the robot frame of reference
  Pose3d cliffWrtRobot;
  cliffWrtRobot.SetParent(robotPoseAtCliff);
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
  if (!cliffWrtRobot.GetWithRespectTo(_robot->GetWorldOrigin(), cliffPose)) {
    PRINT_NAMED_ERROR("CliffSensorComponent.ComputeCliffPose.OriginMismatch",
                      "cliffWrtRobot and robot.GetWorldOrigin() do not share the same origin!");
    return false;
  }
  
  // update navmap
  Point2f cliffSize = MarkerlessObject(ObjectType::CliffDetection).GetSize();
  Quad2f cliffquad {
    { +cliffSize.x(), +cliffSize.y() * .5f },  // up L
    { 0.f,            +cliffSize.y() * .5f },  // lo L
    { +cliffSize.x(), -cliffSize.y() * .5f },  // up R
    { 0.f,            -cliffSize.y() * .5f }}; // lo R
  ((Pose2d) cliffPose).ApplyTo(cliffquad, cliffquad);

  _robot->GetMapComponent().InsertData(Poly2f(cliffquad), MemoryMapData_Cliff(cliffPose, cliffEvent.timestamp));

  return true;
}

void CliffSensorComponent::EnableStopOnWhite(bool stopOnWhite)
{
  if (_stopOnWhiteEnabled != stopOnWhite) {
    if (stopOnWhite && _robot->IsPickedUp()) {
      PRINT_NAMED_WARNING("CliffSensorComponent.EnableStopOnWhite.IgnoredDueToPickup", "");
    } else {
      PRINT_NAMED_INFO("CliffSensorComponent.EnableStopOnWhite", "%d", stopOnWhite);
      _stopOnWhiteEnabled = stopOnWhite;
      _robot->SendRobotMessage<RobotInterface::EnableStopOnWhite>(stopOnWhite);
    }
  }
}
  
} // Cozmo namespace
} // Anki namespace
