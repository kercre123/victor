/**
 * File: proxSensorComponent.cpp
 *
 * Author: Matt Michini
 * Created: 8/30/2017
 *
 * Description: Component for managing forward distance sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/proxSensorComponent.h"

#include "engine/robot.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/logging/rollingFileLogger.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const Vec3f kProxSensorPositionVec_mm{kProxSensorPosition_mm[0], kProxSensorPosition_mm[1], kProxSensorPosition_mm[2]};
  
  const std::string kLogDirectory = "sensorData/proxSensor";
} // end anonymous namespace

  
ProxSensorComponent::ProxSensorComponent(Robot& robot)
  : _robot(robot)
  , _rawDataLogger(nullptr)
{
}
  
  
ProxSensorComponent::~ProxSensorComponent() = default;
  
  
void ProxSensorComponent::Update(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;

  _latestData = msg.proxData;
  
  Log();
}


void ProxSensorComponent::StartLogging(const uint32_t duration_ms)
{
  if (!_loggingRawData) {
    _loggingRawData = true;
    if (duration_ms != 0) {
      const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _logRawDataUntil_s = now + static_cast<float>(duration_ms)/1000.f;
    } else {
      _logRawDataUntil_s = 0.f;
    }
    PRINT_NAMED_INFO("ProxSensorComponent.StartLogging.Start",
                     "Starting raw cliff data logging, duration %d ms%s. Log will appear in '%s'",
                     duration_ms,
                     _logRawDataUntil_s == 0.f ? " (indefinitely)" : "",
                     _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory).c_str());
  } else {
    PRINT_NAMED_WARNING("ProxSensorComponent.StartLogging.AlreadyLogging", "Already logging raw data!");
  }
}
  
  
void ProxSensorComponent::StopLogging()
{
  if (_loggingRawData) {
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _logRawDataUntil_s = now;
  } else {
    PRINT_NAMED_WARNING("ProxSensorComponent.StopLogging.NotLogging", "Not logging raw data!");
  }
}
  
  
Pose3d ProxSensorComponent::GetPose() const
{
  Pose3d sensorPose;
  sensorPose.SetTranslation(kProxSensorPositionVec_mm);
  sensorPose.SetRotation(-kProxSensorTiltAngle_rad, Y_AXIS_3D()); // negative since tilt angle is upward
  sensorPose.SetParent(_robot.GetPose());
  return sensorPose;
}


Result ProxSensorComponent::IsInFOV(const Pose3d& inPose, bool& isInFOV) const
{
  isInFOV = false;

  const auto& sensorPose = GetPose();
  Pose3d inPoseWrtSensor("inPoseWrtSensor");
  if (!inPose.GetWithRespectTo(sensorPose, inPoseWrtSensor)) {
    return Result::RESULT_FAIL;
  }
  
  // Sensor beam goes along its x axis, so the distance away is simply the pose's x value
  const auto dist = inPoseWrtSensor.GetTranslation().x();
  if (dist < 0) {
    // Not in field of view if behind the sensor
    isInFOV = false;
    return Result::RESULT_OK;
  }
  
  // Compute r (which is the radial distance from the beam's center to the pose in question)
  const float r = std::hypot(inPoseWrtSensor.GetTranslation().y(),
                             inPoseWrtSensor.GetTranslation().z());

  // The sensor beam is a cone, and the cone's radius at a given distance is
  // given by distance * tan(fullFOV / 2)
  const float beamRadiusAtDist = dist * std::tan(kProxSensorFullFOV_rad / 2.f);
  
  isInFOV = (r <= beamRadiusAtDist);
  
  return Result::RESULT_OK;
}
  

Result ProxSensorComponent::IsLiftInFOV(bool& isInFOV) const
{
  // TODO: This may require a tolerance due to motor backlash, etc.
  // Verify with physical robots that this is accurate.
  
  isInFOV = false;
  if (!_robot.IsLiftCalibrated()) {
    PRINT_NAMED_WARNING("ProxSensorComponent.IsLiftInFOV.LiftNotCalibrated",
                        "Lift is not calibrated! Considering it not in FOV.");
    return Result::RESULT_FAIL;
  }

  const float liftHeight = _robot.GetLiftHeight();
  
  // Note: this is the approximate x distance from the sensor to the lift crossbar
  // when the lift is in front of the sensor. All distances here in mm.
  const float sensorToLiftDist = (LIFT_BASE_POSITION[0] + LIFT_ARM_LENGTH) - kProxSensorPosition_mm[0];
  const float beamRadiusAtLift = sensorToLiftDist * std::tan(kProxSensorFullFOV_rad / 2.f);
  
  // If the current lift height is within this tolerance of LIFT_HEIGHT_OCCLUDING_PROX_SENSOR,
  // then at least part of the lift is within view of the sensor.
  const float withinViewTol = beamRadiusAtLift + (LIFT_XBAR_HEIGHT / 2.f);
  
  isInFOV = Util::IsNear(liftHeight, LIFT_HEIGHT_OCCLUDING_PROX_SENSOR, withinViewTol);
  
  return Result::RESULT_OK;
}
  
  
void ProxSensorComponent::Log()
{
  if (!_loggingRawData) {
    return;
  }
  
  // Check to see if we should stop logging:
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (_logRawDataUntil_s != 0.f &&
      now >= _logRawDataUntil_s &&
      _rawDataLogger != nullptr) {
    _rawDataLogger->Flush();
    _rawDataLogger.reset();
    _loggingRawData = false;
    return;
  }
  
  // Create a logger if it doesn't exist already
  if (_rawDataLogger == nullptr) {
    _rawDataLogger = std::make_unique<Util::RollingFileLogger>(nullptr, _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory));
    _rawDataLogger->Write("timestamp_ms, distance_mm, signalIntensity, ambientIntensity, spadCount, rangeStatus \n");
  }
  
  const auto& d = _latestData;
  std::string str;
  str += std::to_string(_lastMsgTimestamp)  + ", ";
  str += std::to_string(d.distance_mm)      + ", ";
  str += std::to_string(d.signalIntensity)  + ", ";
  str += std::to_string(d.ambientIntensity) + ", ";
  str += std::to_string(d.spadCount)        + ", ";
  str += std::to_string(d.rangeStatus)      + "\n";
  
  _rawDataLogger->Write(str);
}

  
} // Cozmo namespace
} // Anki namespace
