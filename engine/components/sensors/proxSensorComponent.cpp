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

#include "engine/components/sensors/proxSensorComponent.h"

#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirName = "proxSensor";

  const Vec3f kProxSensorPositionVec_mm{kProxSensorPosition_mm[0], kProxSensorPosition_mm[1], kProxSensorPosition_mm[2]};
} // end anonymous namespace

// enable/disable prox sensor data
CONSOLE_VAR(bool, kProxSensorEnabled, "ProxSensorComponent", true);

// extra padding to add to prox obstacle
CONSOLE_VAR(float, kObsPadding_mm, "ProxSensorComponent", 0.05f);

// distance range for registering an object detected as an obstacle
CONSOLE_VAR(u16, kMinObsThreshold_mm, "ProxSensorComponent", 30);
CONSOLE_VAR(u16, kMaxObsThreshold_mm, "ProxSensorComponent", 400);

// max forward tilt of the robot to still consider a valid reading
CONSOLE_VAR(float, kMaxForwardTilt_rad, "ProxSensorComponent", -.08);

// Minimum sensor reading strength before trying to use sensor data
CONSOLE_VAR(float, kMinQualityThreshold, "ProxSensorComponent", 0.05f);

  
ProxSensorComponent::ProxSensorComponent() 
: ISensorComponent(kLogDirName, RobotComponentID::ProxSensor)
{
}
  
  
void ProxSensorComponent::UpdateInternal(const RobotState& msg)
{
  if (kProxSensorEnabled)
  {
    _lastMsgTimestamp = msg.timestamp;
    _latestData = msg.proxData;
    
    UpdateNavMap();
  }
}


std::string ProxSensorComponent::GetLogHeader()
{
  return std::string("timestamp_ms, distance_mm, signalIntensity, ambientIntensity, spadCount, rangeStatus");
}


std::string ProxSensorComponent::GetLogRow()
{
  const auto& d = _latestData;
  std::stringstream ss;
  ss << _lastMsgTimestamp  << ", ";
  ss << d.distance_mm      << ", ";
  ss << d.signalIntensity  << ", ";
  ss << d.ambientIntensity << ", ";
  ss << d.spadCount        << ", ";
  ss << (uint16_t) d.rangeStatus;

  return ss.str();
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
  

Result ProxSensorComponent::IsLiftInFOV(bool& isInFOV) const
{
  // TODO: This may require a tolerance due to motor backlash, etc.
  // Verify with physical robots that this is accurate.
  
  isInFOV = false;
  if (!_robot->IsLiftCalibrated()) {
    PRINT_NAMED_WARNING("ProxSensorComponent.IsLiftInFOV.LiftNotCalibrated",
                        "Lift is not calibrated! Considering it not in FOV.");
    return Result::RESULT_FAIL;
  }

  const float liftHeight = _robot->GetLiftHeight();
  
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


bool ProxSensorComponent::IsSensorReadingValid()
{
  const u16 proxDist_mm = GetLatestDistance_mm();
  bool liftBlocking;
  IsLiftInFOV(liftBlocking);
  const bool sensorInRangeAndValid = (proxDist_mm > kMinObsThreshold_mm) &&
                                      (proxDist_mm < kMaxObsThreshold_mm) &&
                                      !liftBlocking;
  return sensorInRangeAndValid;
}


bool ProxSensorComponent::CalculateSensedObjectPose(Pose3d& sensedObjectPose)
{
  const bool sensorIsValid = IsSensorReadingValid();
  if(sensorIsValid){
    const Pose3d proxPose = GetPose();
    const u16 proxDist_mm = GetLatestDistance_mm();
    Transform3d transformToSensed(Rotation3d(0.f, Z_AXIS_3D()), {0.f, proxDist_mm, 0.f});
    // Since prox pose is destroyed when it falls out of scope, don't want parent invalidated
    sensedObjectPose = Pose3d(transformToSensed, proxPose).GetWithRespectToRoot();
  }
  return sensorIsValid;
}


void ProxSensorComponent::UpdateNavMap()
{
  if (_latestData.spadCount == 0)
  {
    PRINT_NAMED_WARNING("ProxSensorComponent.UpdateNavMap", "Invalid sensor reading, SpadCount == 0");
    return;
  }

  const float quality = _latestData.signalIntensity / _latestData.spadCount;

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
    
    // clear out known free space
    INavMap* currentNavMemoryMap = _robot->GetMapComponent().GetCurrentMemoryMap();
    if ( currentNavMemoryMap ) {
      Vec3f rayOffset1(-kObsPadding_mm,  -kObsPadding_mm, 0);   
      Vec3f rayOffset2(-kObsPadding_mm,   kObsPadding_mm, 0);
      Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());

      const Point2f t1 = (objectPos.GetTransform() * Transform3d(rot, rayOffset1)).GetTranslation();
      const Point2f t2 = (objectPos.GetTransform() * Transform3d(rot, rayOffset2)).GetTranslation(); 

      Triangle2f tri(t1, t2, _robot->GetPose().GetTranslation());
      MemoryMapData clearRegion(INavMap::EContentType::ClearOfObstacle, lastTimestamp);

      // currentNavMemoryMap->AddLine(_robot->GetPose().GetTranslation(), objectPos.GetTranslation(), clearRegion);
      currentNavMemoryMap->AddTriangle(tri, clearRegion);

      // Add proxObstacle if detected and close to robot 
      if (_latestData.distance_mm <= kMaxObsThreshold_mm && !noObject) { 
        const float obstacleHalfWidth_mm = ROBOT_BOUNDING_Y * .5 + kObsPadding_mm;
        Vec3f offset1(-kObsPadding_mm,  -obstacleHalfWidth_mm, 0);   
        Vec3f offset2(-kObsPadding_mm,   obstacleHalfWidth_mm, 0);
        Vec3f offset3(kObsPadding_mm * 2, 0, 0);

        const Point2f p1 = (objectPos.GetTransform() * Transform3d(rot, offset1)).GetTranslation();
        const Point2f p2 = (objectPos.GetTransform() * Transform3d(rot, offset2)).GetTranslation(); 
        const Point2f p3 = (objectPos.GetTransform() * Transform3d(rot, offset1 + offset3)).GetTranslation();
        const Point2f p4 = (objectPos.GetTransform() * Transform3d(rot, offset2 + offset3)).GetTranslation();

        const Quad2f quad(p1, p2, p3, p4);
        const Vec3f rotatedFwdVector = _robot->GetPose().GetWithRespectToRoot().GetRotation() * X_AXIS_3D();
        
        MemoryMapData_ProxObstacle proxData(Vec2f{rotatedFwdVector.x(), rotatedFwdVector.y()}, lastTimestamp);      
        currentNavMemoryMap->AddQuad(quad, proxData);
      }
    }
  }
}


} // Cozmo namespace
} // Anki namespace
