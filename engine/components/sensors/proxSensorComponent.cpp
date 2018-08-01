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

#include "coretech/common/engine/math/convexIntersection.h"
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

  // for checking if the state has changed since the last measurement
  const Point3f kRobotTranslationTolerance_mm{0.1f, 0.1f, 0.1f};
  const float   kMeasurementTolerance_mm = 2.f;
  const Radians kRobotRotationTolerance_rad = 0.01f;
  const u8      kNumMeasurementsAtPose = 32;
} // end anonymous namespace

// enable/disable prox sensor data
CONSOLE_VAR(bool, kProxSensorEnabled, "ProxSensorComponent", true);

// extra padding to add to prox obstacle
CONSOLE_VAR(float, kObsPadding_x_mm, "ProxSensorComponent", 6.f);
CONSOLE_VAR(float, kObsPadding_y_mm, "ProxSensorComponent", 0.f);

// distance range for registering an object detected as an obstacle
CONSOLE_VAR(u16, kMinObsThreshold_mm, "ProxSensorComponent", 30);
CONSOLE_VAR(u16, kMaxObsThreshold_mm, "ProxSensorComponent", 400);

// max forward tilt of the robot to still consider a valid reading
CONSOLE_VAR(float, kMaxForwardTilt_rad, "ProxSensorComponent", -.08);

// Minimum sensor reading strength before trying to use sensor data
CONSOLE_VAR(float, kMinQualityThreshold, "ProxSensorComponent", 0.01f);

// angle for calculating obstacle width, ~tan(22.5)
CONSOLE_VAR(float, kSensorAperture, "ProxSensorComponent", 0.4f);

ProxSensorComponent::ProxSensorComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::ProxSensor)
{
}


void ProxSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  if (kProxSensorEnabled && _enabled)
  {
    _lastMsgTimestamp = msg.timestamp;
    _latestDataRaw = msg.proxData;

    UpdateReadingValidity();

    // Reading is meaningless in calm mode so just skip map update
    const bool isCalmPowerMode = static_cast<bool>(msg.status & (uint16_t)RobotStatusFlag::CALM_POWER_MODE);
    if (!isCalmPowerMode) {
      UpdateNavMap();
    }
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
    // Lift might not be calibrated if bracing while falling
    PRINT_NAMED_INFO("ProxSensorComponent.UpdateReadingValidity.LiftNotCalibrated",
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
    // check if the robot has moved or the sensor reading has changed significantly
    const Pose3d  robotPose = _robot->GetPose(); 
    if (!robotPose.IsSameAs(_previousRobotPose, kRobotTranslationTolerance_mm, kRobotRotationTolerance_rad ) ||
        !NEAR(_latestData.distance_mm, _previousMeasurement, kMeasurementTolerance_mm)) { 
      _measurementsAtPose = 0; 
      _previousRobotPose = robotPose; 
      _previousMeasurement = _latestData.distance_mm;
    }
 
    if (++_measurementsAtPose >= kNumMeasurementsAtPose) { return; }

    // Clear out any obstacles between the robot and ray if we have good signal strength 
    RobotTimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();

    // build line for ray cast by getting the robot pose, casting forward by sensor reading
    const Vec3f offsetx_mm( (noObject) ? kMaxObsThreshold_mm 
                                       : fmin(_latestData.distance_mm, kMaxObsThreshold_mm), 0, 0);   

    // just assume robot center rather the actual sensor pose
    const Pose3d  objectPos = robotPose * Pose3d(0, Z_AXIS_3D(), offsetx_mm);    
    const Rotation3d id = Rotation3d(0.f, Z_AXIS_3D());

    // we would like to clear the whole sensor cone as defined by the aperture, but it creates really large
    // obstacles if it detects something far away. To prevent planning failures, we would like to clamp
    // the max obstacle width to one half robot width, but that can result in stretching the of the clear region
    // cone, which causes some map artifacts. To get around this, define the clear region as the intersection
    // of the sensor cone and a straight beam with obstacle width (which is clamped to robot width).

    //           +-------------+
    //           |             |    ....-----------------------p1--p4
    //           |    robot    |::::                            |xx| (obstacle)
    //           |             |    ''''-----------------------p2--p3
    //           +-------------+

    // sensor points
    const float sensorBeamHalfWidth_mm = offsetx_mm.x() * kSensorAperture *.5f;
    const float obstacleHalfWidth_mm = std::fmin(sensorBeamHalfWidth_mm, ROBOT_BOUNDING_Y *.25);

    // use a slightly larger padding for clear than for obstacle to clean up floating point rounding errors
    const Vec3f sensorOffset1(-kObsPadding_x_mm,  -sensorBeamHalfWidth_mm - kObsPadding_y_mm - .5, 0);   
    const Vec3f sensorOffset2(-kObsPadding_x_mm,   sensorBeamHalfWidth_mm + kObsPadding_y_mm + .5, 0);

    const Vec3f clampOffset1(-offsetx_mm.x(),  -obstacleHalfWidth_mm - kObsPadding_y_mm, 0);   
    const Vec3f clampOffset2(-offsetx_mm.x(),   obstacleHalfWidth_mm + kObsPadding_y_mm, 0);

    const Point2f sensorCorner1 = (objectPos.GetTransform() * Transform3d(id, sensorOffset1)).GetTranslation();
    const Point2f sensorCorner2 = (objectPos.GetTransform() * Transform3d(id, sensorOffset2)).GetTranslation(); 

    const Point2f clampCorner1  = (objectPos.GetTransform() * Transform3d(id, clampOffset1)).GetTranslation();
    const Point2f clampCorner2  = (objectPos.GetTransform() * Transform3d(id, clampOffset2)).GetTranslation(); 

    // obstacle points
    const Vec3f obstacleOffset1(-kObsPadding_x_mm,  -obstacleHalfWidth_mm - kObsPadding_y_mm, 0);   
    const Vec3f obstacleOffset2(-kObsPadding_x_mm,   obstacleHalfWidth_mm + kObsPadding_y_mm, 0);
    const Vec3f obstacleOffset3(kObsPadding_x_mm * 2, 0, 0);

    const Point2f obstacleP1 = (objectPos.GetTransform() * Transform3d(id, obstacleOffset1)).GetTranslation();
    const Point2f obstacleP2 = (objectPos.GetTransform() * Transform3d(id, obstacleOffset2)).GetTranslation(); 
    const Point2f obstacleP3 = (objectPos.GetTransform() * Transform3d(id, obstacleOffset2 + obstacleOffset3)).GetTranslation();
    const Point2f obstacleP4 = (objectPos.GetTransform() * Transform3d(id, obstacleOffset1 + obstacleOffset3)).GetTranslation();

    // clear sensor beam
    ConvexIntersection2f intersection;
    intersection.AddSet( FastPolygon({sensorCorner1, sensorCorner2, robotPose.GetTranslation()}) );   // sensor beam
    intersection.AddSet( FastPolygon({clampCorner1, clampCorner2, obstacleP2, obstacleP1}) );         // clamp

    _robot->GetMapComponent().ClearRegion( intersection,  lastTimestamp);

    // Add proxObstacle if detected and close to robot, and lift is not interfering
    if (_latestData.distance_mm <= kMaxObsThreshold_mm && !noObject && !IsLiftInFOV()) {
      const FastPolygon quad({obstacleP1, obstacleP2, obstacleP3, obstacleP4});
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
