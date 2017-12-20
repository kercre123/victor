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

#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"

#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirName = "proxSensor";

  const Vec3f kProxSensorPositionVec_mm{kProxSensorPosition_mm[0], kProxSensorPosition_mm[1], kProxSensorPosition_mm[2]};

  const f32 kObsPadding_mm       = 1.0; // extra padding to add to prox obstacle
  const u16 kMinObsThreshold_mm  = 30;  // Minimum distance for registering an object detected as an obstacle
  const u16 kMaxObsThreshold_mm  = 400; // Maximum distance for registering an object detected as an obstacle  
  const f32 kMinQualityThreshold = .15; // Minimum sensor reading strength before trying to use sensor data
} // end anonymous namespace

  
ProxSensorComponent::ProxSensorComponent(Robot& robot) : ISensorComponent(robot, kLogDirName)
{
}
  
  
void ProxSensorComponent::UpdateInternal(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;
  _latestData = msg.proxData;
 
  if (!_thereminActive &&
      _robot.GetHeadAngle() < 0.f) {
    ActivateTheremin(true);
  } else if (_thereminActive &&
             _robot.GetHeadAngle() > 0.f) {
    ActivateTheremin(false);
  }
  
  UpdateTheremin();
  
  UpdateNavMap();
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
  
  if (objectDetected || noObject)
  {  
    // Clear out any obstacles between the robot and ray if we have good signal strength 
    TimeStamp_t lastTimestamp = _robot.GetLastMsgTimestamp();

    // build line for ray cast by getting the robot pose, casting forward by sensor reading
    const Vec3f offsetx_mm( (noObject) ? kMaxObsThreshold_mm 
                                       : fmin(_latestData.distance_mm, kMaxObsThreshold_mm), 0, 0);   

    const Pose3d  objectPos = _robot.GetPose() * Pose3d(0, Z_AXIS_3D(), offsetx_mm);    
    
    // clear out known free space
    INavMap* currentNavMemoryMap = _robot.GetMapComponent().GetCurrentMemoryMap();
    if ( currentNavMemoryMap ) {
      Vec3f rayOffset1(-kObsPadding_mm,  -kObsPadding_mm, 0);   
      Vec3f rayOffset2(-kObsPadding_mm,   kObsPadding_mm, 0);
      Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());

      const Point2f t1 = (objectPos.GetTransform() * Transform3d(rot, rayOffset1)).GetTranslation();
      const Point2f t2 = (objectPos.GetTransform() * Transform3d(rot, rayOffset2)).GetTranslation(); 

      Triangle2f tri(t1, t2, _robot.GetPose().GetTranslation());
      MemoryMapData clearRegion(INavMap::EContentType::ClearOfObstacle, lastTimestamp);

      // currentNavMemoryMap->AddLine(_robot.GetPose().GetTranslation(), objectPos.GetTranslation(), clearRegion);
      currentNavMemoryMap->AddTriangle(tri, clearRegion);

      // Add proxObstacle if detected and close to robot 
      if (_latestData.distance_mm <= kMaxObsThreshold_mm) { 
        const float obstacleHalfWidth_mm = ROBOT_BOUNDING_Y * .5 + kObsPadding_mm;
        Vec3f offset1(-kObsPadding_mm,  -obstacleHalfWidth_mm, 0);   
        Vec3f offset2(-kObsPadding_mm,   obstacleHalfWidth_mm, 0);
        Vec3f offset3(kObsPadding_mm * 2, 0, 0);

        const Point2f p1 = (objectPos.GetTransform() * Transform3d(rot, offset1)).GetTranslation();
        const Point2f p2 = (objectPos.GetTransform() * Transform3d(rot, offset2)).GetTranslation(); 
        const Point2f p3 = (objectPos.GetTransform() * Transform3d(rot, offset1 + offset3)).GetTranslation();
        const Point2f p4 = (objectPos.GetTransform() * Transform3d(rot, offset2 + offset3)).GetTranslation();

        const Quad2f quad(p1, p2, p3, p4);
        const Vec3f rotatedFwdVector = _robot.GetPose().GetWithRespectToRoot().GetRotation() * X_AXIS_3D();
        
        MemoryMapData_ProxObstacle proxData(Vec2f{rotatedFwdVector.x(), rotatedFwdVector.y()}, lastTimestamp);      
        currentNavMemoryMap->AddQuad(quad, proxData);
      }
    }
  }
}


void ProxSensorComponent::ActivateTheremin(const bool enable)
{
  PRINT_NAMED_WARNING("ProxSensorComponent.Theremin", "%s theremin",
                      enable ? "activating" : "de-activating");
  
  _thereminActive = enable;
  
  if (enable) {
    _robot.GetAudioClient()->PostEvent(AudioMetaData::GameEvent::GenericEvent::Play__Robot_Sfx__Theremin_Loop_Play,
                                       AudioMetaData::GameObjectType::Cozmo_OnDevice);

    // turn volume on:
    _robot.GetAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Robot_Volume,
                                           0.5f);
  } else {
    _robot.GetAudioClient()->PostEvent(AudioMetaData::GameEvent::GenericEvent::Stop__Robot_Sfx__Theremin_Loop_Stop,
                                       AudioMetaData::GameObjectType::Cozmo_OnDevice);
  }
}


void ProxSensorComponent::UpdateTheremin()
{
  if (!_thereminActive) {
    return;
  }
  
  const float distance = static_cast<float>(_latestData.distance_mm);
  const float signalStrength = static_cast<float>(_latestData.signalIntensity) / static_cast<float>(_latestData.spadCount);
  
  float pitchVal = 0.f;
  float volumeVal = 1.f;
  
  // Create a pitch value from 0 to 1 based on distance sensor readings. Pitch should
  // be highest when distance readings are smallest.
  const float minDist = 60;
  const float maxDist = 300;
  
  pitchVal = (maxDist - distance) / (maxDist - minDist);
  pitchVal = Util::Clamp(pitchVal, 0.f, 1.f);
  
  // Only allow pitch to change if the signal strength is sufficiently strong:
  const float kThereminSigStrThreshold = 0.01f;
  if (signalStrength < kThereminSigStrThreshold) {
    pitchVal = 0.f;
  }
  
  // Post pitch parameter
  _robot.GetAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Theremin_Pitch,
                                         pitchVal,
                                         AudioMetaData::GameObjectType::Cozmo_OnDevice);
  
  // Post volume parameter
  _robot.GetAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Theremin_Volume,
                                         volumeVal,
                                         AudioMetaData::GameObjectType::Cozmo_OnDevice);
  
  static int cnt = 0;
  if (++cnt%10 == 0) {
    PRINT_NAMED_INFO("ThereminUpdate",
                     "dist: %d, sigstr %.4f, touch gesture %s",
                     _latestData.distance_mm,
                     signalStrength,
                     EnumToString(_robot.GetTouchSensorComponent().GetLatestTouchGesture()));
  }
}

  
} // Cozmo namespace
} // Anki namespace
