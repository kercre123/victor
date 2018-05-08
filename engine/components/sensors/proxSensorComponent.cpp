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

#include "engine/components/carryingComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"

#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/math/polygon_impl.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirName = "proxSensor";

  // The minimum number of consecutive tics that lift must be out of 
  // FOV in order to actually be considered out of FOV. 
  // This is to account for apparent timing delay between the sensor 
  // and lift position readings.
  // NOTE: A tic in this case is one RobotState msg (i.e. 30ms)
  const u8 kMinNumConsecTicsLiftOutOfFOV = 3;

  // Between these min/max lift heights, the lift interferes with the prox sensor's beam.
  const f32 kLiftHeightOccludingProxSensorMin = 34.f;
  const f32 kLiftHeightOccludingProxSensorMax = 56.f;

  // When carrying a cube, the sensor is occluded most of the time 
  // except when lift is above a certain height
  const f32 kLiftHeightOccludingProxSensorWithCubeMin = 0;
  const f32 kLiftHeightOccludingProxSensorWithCubeMax = LIFT_HEIGHT_HIGHDOCK;

  const f32 kMinPitch = DEG_TO_RAD(-8);
  const f32 kMaxPitch = DEG_TO_RAD(8);

  const Vec3f kProxSensorPositionVec_mm{kProxSensorPosition_mm[0], kProxSensorPosition_mm[1], kProxSensorPosition_mm[2]};
} // end anonymous namespace

// enable/disable prox sensor data
CONSOLE_VAR(bool, kProxSensorEnabled, "ProxSensorComponent", true);

// extra padding to add to prox obstacle
CONSOLE_VAR(float, kObsPadding_x_mm, "ProxSensorComponent", 6.f);
CONSOLE_VAR(float, kObsPadding_y_mm, "ProxSensorComponent", 0.f);
CONSOLE_VAR(float, kClearHalfWidth_mm, "ProxSensorComponent", 1.5f);

// distance range for registering an object detected as an obstacle
CONSOLE_VAR(u16, kMinObsThreshold_mm, "ProxSensorComponent", 30);
CONSOLE_VAR(u16, kMaxObsThreshold_mm, "ProxSensorComponent", 400);

// max forward tilt of the robot to still consider a valid reading
CONSOLE_VAR(float, kMaxForwardTilt_rad, "ProxSensorComponent", -.08);

// Minimum sensor reading strength before trying to use sensor data
CONSOLE_VAR(float, kMinQualityThreshold, "ProxSensorComponent", 0.05f);

// angle for calculating obstacle width, ~tan(22.5)
CONSOLE_VAR(float, kSensorAperature, "ProxSensorComponent", 0.4f);

ProxSensorComponent::ProxSensorComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::ProxSensor)
{
}


void ProxSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  if (kProxSensorEnabled)
  {
    _lastMsgTimestamp = msg.timestamp;
    _latestDataRaw = msg.proxData;

    UpdateReadingValidity();
    UpdateNavMap();
  }
}


std::string ProxSensorComponent::GetLogHeader()
{
  return std::string("timestamp_ms, distance_mm, signalIntensity, ambientIntensity, spadCount, rangeStatus");
}


std::string ProxSensorComponent::GetLogRow()
{
  const auto& d = _latestDataRaw;
  std::stringstream ss;
  ss << _lastMsgTimestamp  << ", ";
  ss << d.distance_mm      << ", ";
  ss << d.signalIntensity  << ", ";
  ss << d.ambientIntensity << ", ";
  ss << d.spadCount        << ", ";
  ss << (uint16_t) d.rangeStatus;

  return ss.str();
}


bool ProxSensorComponent::GetLatestDistance_mm(u16& distance_mm) const
{
  distance_mm = _latestDataRaw.distance_mm;
  return IsLatestReadingValid();
}

Pose3d ProxSensorComponent::GetPose() const
{
  Pose3d sensorPose;
  sensorPose.SetTranslation(kProxSensorPositionVec_mm);
  sensorPose.SetRotation(-kProxSensorTiltAngle_rad, Y_AXIS_3D()); // negative since tilt angle is upward
  sensorPose.SetParent(_robot->GetPose());
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
  
void ProxSensorComponent::UpdateReadingValidity() 
{
  bool isInFOV = false;
  if (_robot->IsLiftCalibrated()) {
    // Check if lift or carried object may be occluding the sensor
    const bool isCarryingObject  = _robot->GetCarryingComponent().IsCarryingObject();
    isInFOV = Util::InRange(_robot->GetLiftHeight(),
                            isCarryingObject ? kLiftHeightOccludingProxSensorWithCubeMin : kLiftHeightOccludingProxSensorMin,
                            isCarryingObject ? kLiftHeightOccludingProxSensorWithCubeMax : kLiftHeightOccludingProxSensorMax);
    
    // Make sure lift is out of field of view for the minimum number of 
    // tics to actually be considered out of field of view
    if (!isInFOV) {
      ++_numTicsLiftOutOfFOV;
      if (_numTicsLiftOutOfFOV < kMinNumConsecTicsLiftOutOfFOV) {
        isInFOV = true;
      } else {
        _numTicsLiftOutOfFOV = kMinNumConsecTicsLiftOutOfFOV;
      }
    } else {
      _numTicsLiftOutOfFOV = 0;
    }

  } else {
    PRINT_NAMED_WARNING("ProxSensorComponent.UpdateReadingValidity.LiftNotCalibrated",
                        "Lift is not calibrated! Considering it not in FOV.");
  }
  _latestData.isLiftInFOV = isInFOV;

  // Check if robot is too pitched for reading to be valid
  const f32 pitchAngle = _robot->GetPitchAngle().ToFloat();
  _latestData.isTooPitched = pitchAngle < kMinPitch || pitchAngle > kMaxPitch;

  // Check if the reading is within the valid range  
  _latestData.distance_mm = _latestDataRaw.distance_mm;
  _latestData.isInValidRange = Util::InRange(_latestData.distance_mm,
                                             kMinObsThreshold_mm,
                                             kMaxObsThreshold_mm);
  
  // Check that the signal strength is high enough
  _latestData.signalQuality = GetSignalQuality(_latestDataRaw);
  _latestData.isValidSignalQuality = _latestData.signalQuality > kMinQualityThreshold;
}


bool ProxSensorComponent::CalculateSensedObjectPose(Pose3d& sensedObjectPose) const
{
  u16 proxDist_mm = 0;
  const bool sensorIsValid = GetLatestDistance_mm(proxDist_mm);
  if(sensorIsValid){
    const Pose3d proxPose = GetPose();
    Transform3d transformToSensed(Rotation3d(0.f, Z_AXIS_3D()), {0.f, proxDist_mm, 0.f});
    // Since prox pose is destroyed when it falls out of scope, don't want parent invalidated
    sensedObjectPose = Pose3d(transformToSensed, proxPose).GetWithRespectToRoot();
  }
  return sensorIsValid;
}


void ProxSensorComponent::UpdateNavMap()
{
  if (_latestDataRaw.spadCount == 0)
  {
    PRINT_NAMED_WARNING("ProxSensorComponent.UpdateNavMap", "Invalid sensor reading, SpadCount == 0");
    return;
  }

  const float quality       = _latestData.signalQuality;
  const bool noObject       = quality <= kMinQualityThreshold;                      // sensor is pointed at free space
  const bool objectDetected = (quality >= kMinQualityThreshold &&                   // sensor is getting some reading
                               _latestData.distance_mm >= kMinObsThreshold_mm);     // the sensor is not seeing the lift
  const bool tiltedForward  = _robot->GetPitchAngle() < kMaxForwardTilt_rad;        // if the robot is titled too far forward (- rad) 

  if ((objectDetected || noObject) && !tiltedForward)
  {  
    // Clear out any obstacles between the robot and ray if we have good signal strength 
    TimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();

    // build line for ray cast by getting the robot pose, casting forward by sensor reading
    const Vec3f offsetx_mm( (noObject) ? kMaxObsThreshold_mm 
                                       : fmin(_latestData.distance_mm, kMaxObsThreshold_mm), 0, 0);   

    const Pose3d  objectPos = _robot->GetPose() * Pose3d(0, Z_AXIS_3D(), offsetx_mm);    
    Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());

    const float obstacleHalfWidth_mm = std::fmin(offsetx_mm.x() * kSensorAperature, ROBOT_BOUNDING_Y) * .5 + kObsPadding_y_mm;
    Vec3f offset1(-kObsPadding_x_mm,  -obstacleHalfWidth_mm, 0);   
    Vec3f offset2(-kObsPadding_x_mm,   obstacleHalfWidth_mm, 0);
    Vec3f offset3(kObsPadding_x_mm * 2, 0, 0);

    const Point2f p1 = (objectPos.GetTransform() * Transform3d(rot, offset1)).GetTranslation();
    const Point2f p2 = (objectPos.GetTransform() * Transform3d(rot, offset2)).GetTranslation(); 
    const Point2f p3 = (objectPos.GetTransform() * Transform3d(rot, offset2 + offset3)).GetTranslation();
    const Point2f p4 = (objectPos.GetTransform() * Transform3d(rot, offset1 + offset3)).GetTranslation();

    // note: the size that gets cleared might not be as large as you think, since we only replace
    // a prox obstacle with clear space if the clear space leaf quad has its center within
    // the poly from the robot footprint to line segment (t1,t2)
    _robot->GetMapComponent().ClearRobotToEdge(p1, p2, lastTimestamp);

    // Add proxObstacle if detected and close to robot, and lift is not interfering
    if (_latestData.distance_mm <= kMaxObsThreshold_mm && !noObject && !IsLiftInFOV()) {
      const Poly2f quad({p1, p2, p3, p4});
      Radians angle = _robot->GetPose().GetRotationAngle<'Z'>();
      
      MemoryMapData_ProxObstacle proxData(MemoryMapData_ProxObstacle::NOT_EXPLORED,
                                          Pose2d{angle, objectPos.GetWithRespectToRoot().GetTranslation().x(), objectPos.GetWithRespectToRoot().GetTranslation().y()},
                                          lastTimestamp);
      _robot->GetMapComponent().AddProxData(quad, proxData);
    }
  }
}


} // Cozmo namespace
} // Anki namespace
