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
#include "engine/robotStateHistory.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/math/convexIntersection.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"

#include <iomanip>

namespace Anki {
namespace Vector {
  
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

  // for checking if the state has changed since the last measurement
  const Point3f kRobotTranslationTolerance_mm{0.1f, 0.1f, 0.1f};
  const float   kMeasurementTolerance = .05f;  // percentage based tolerance
  const Radians kRobotRotationTolerance_rad = 0.01f;
  const u8      kNumMeasurementsAtPose = 32;

  // Returns a unitless metric of "signal quality", which is computed as the
  // signal intensity (which is the total signal intensity of the reading)
  // divided by the number of active SPADs (which are the actual imaging sensors)
  inline float GetSignalQuality(const ProxSensorDataRaw& proxData) {
    return proxData.signalIntensity / proxData.spadCount;
  }
  
} // end anonymous namespace

// extra padding to add to prox obstacle
CONSOLE_VAR(float, kObsPadding_x_mm, "ProxSensorComponent", 6.f);
CONSOLE_VAR(float, kObsPadding_y_mm, "ProxSensorComponent", 0.f);

// distance range for registering an object detected as an obstacle
CONSOLE_VAR(u16, kMinObsThreshold_mm, "ProxSensorComponent", 30);
CONSOLE_VAR(u16, kMaxObsThreshold_mm, "ProxSensorComponent", 400);

// Minimum sensor reading strength before trying to use sensor data
CONSOLE_VAR(float, kMinQualityThreshold, "ProxSensorComponent", 0.01f);

// angle for calculating obstacle width, ~tan(22.5)
CONSOLE_VAR(float, kSensorAperture, "ProxSensorComponent", 0.4f);

// maximum width for a prox obstacle
CONSOLE_VAR(float, kMaxObstacleWidth_mm, "ProxSensorComponent", 18.f);

ProxSensorComponent::ProxSensorComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::ProxSensor)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProxSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;
  _lastMsgPoseFrameID = msg.pose_frame_id;
  _latestDataRaw = msg.proxData;

  ProcessRawSensorData();

  // Reading is meaningless in calm mode so just skip map update
  const bool isCalmPowerMode = static_cast<bool>(msg.status & (uint32_t)RobotStatusFlag::CALM_POWER_MODE);
  if (_enabled && !isCalmPowerMode) {
    UpdateNavMap();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ProxSensorComponent::GetLogHeader()
{
  return std::string("timestamp_ms, distance_mm, signalIntensity, ambientIntensity, spadCount, rangeStatus");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ProxSensorComponent::GetDebugString(const std::string& delimeter)
{
  const auto& d = _latestDataRaw;
  std::stringstream ss;
  ss << _lastMsgTimestamp  << delimeter;
  ss << d.distance_mm      << delimeter;
  ss << std::fixed << std::setprecision(3) << d.signalIntensity  << delimeter;
  ss << std::fixed << std::setprecision(3) << d.ambientIntensity << delimeter;
  ss << std::fixed << std::setprecision(3) << d.spadCount        << delimeter;
  ss << RangeStatusToString(d.rangeStatus);

  return ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void ProxSensorComponent::ProcessRawSensorData() 
{
  // check if the measurement was delayed, otherwise use the current robot pose.
  if (_latestDataRaw.timestamp_ms < _lastMsgTimestamp) {
    HistRobotState histState;
    RobotTimeStamp_t histTimestamp;
    const bool kUseInterp = true;
    const auto& res = _robot->GetStateHistory()->ComputeStateAt(_latestDataRaw.timestamp_ms, histTimestamp, histState, kUseInterp);
    if (res != RESULT_OK) {
      // If robot is localized and there are at least 3 RobotState messages in history
      // which span 60ms (i.e. the slowest rate expected of the prox sensor) then
      // report warning since a historical state should've been found.
      const u32 numRawStatesInSameFrame = _robot->GetStateHistory()->GetNumRawStatesWithFrameID(_lastMsgPoseFrameID);
      if (numRawStatesInSameFrame >= 3 && _robot->IsLocalized()) {
        LOG_WARNING("ProxSensorComponent.ProcessRawSensorData.NoHistoricalPose",
                    "Could not retrieve historical pose for timestamp %u (msg time %u)",
                    _latestDataRaw.timestamp_ms, _lastMsgTimestamp);
      }
      return;
    }
    if (histState.GetFrameId() != _robot->GetPoseFrameID()) {
      // Don't update nav map since this could've been from before
      // we cleared the map due to delocalization. 
      // It could also be from a localization but it's not a big deal
      // to drop a few readings when this happens.
      return;
    }
    _currentRobotPose = histState.GetPose();
  } else {
    _currentRobotPose = _robot->GetPose();
  }

  // TODO: use historical pose for lift checks
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
    PRINT_NAMED_INFO("ProxSensorComponent.ProcessRawSensorData.LiftNotCalibrated",
                     "Lift is not calibrated! Considering it not in FOV.");
  }
  _latestData.isLiftInFOV = isInFOV;

  // Check if robot is too pitched for reading to be valid
  const f32 pitchAngle = _robot->GetPitchAngle().ToFloat();
  const bool isTooPitched = pitchAngle < kMinPitch || pitchAngle > kMaxPitch;

  // Check if the reading is within the valid range  
  _latestData.distance_mm = Util::Clamp(_latestDataRaw.distance_mm, kMinObsThreshold_mm, kMaxObsThreshold_mm);
  const bool isInValidRange = _latestDataRaw.distance_mm < kMaxObsThreshold_mm;
  
  // Check that the signal strength is high enough
  const bool validSpadCount = (_latestDataRaw.spadCount > 0);
  if (!validSpadCount) {
    PRINT_NAMED_WARNING("ProxSensorComponent.ProcessRawSensorData.BadSpadCount", "Invalid sensor reading, SpadCount <= 0");
  }

  _latestData.signalQuality = validSpadCount ? GetSignalQuality(_latestDataRaw) : 0.f;
  const bool isValidSignalQuality = _latestData.signalQuality > kMinQualityThreshold;
  
  // Check that the RangeStatus is valid
  const bool hasValidRangeStatus = _latestDataRaw.rangeStatus == RangeStatus::RANGE_VALID;

  // check if either we found an object, or we know the sensor is unobstructed
  _latestData.unobstructed = (_latestData.signalQuality <= kMinQualityThreshold) || !isInValidRange;
  _latestData.foundObject  = isInValidRange && 
                             isValidSignalQuality && 
                            !_latestData.isLiftInFOV && 
                            !isTooPitched && 
                             hasValidRangeStatus;

  // check if the robot has moved or the sensor reading has changed significantly
  const float changePct = fabs(_latestData.distance_mm - _previousMeasurement) / _previousMeasurement;
  const bool noObject   = (_latestData.signalQuality <= kMinQualityThreshold);  // sensor is pointed at free space
  if ( !_currentRobotPose.IsSameAs(_previousRobotPose, kRobotTranslationTolerance_mm, kRobotRotationTolerance_rad ) ||
      (!noObject && FLT_GT(changePct, kMeasurementTolerance)) ) { 
    _measurementsAtPose  = 0; 
    _previousRobotPose   = _currentRobotPose; 
    _previousMeasurement = _latestData.distance_mm;
  }
  ++_measurementsAtPose;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Map Update Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  // we would like to clear the whole sensor cone as defined by the aperture, but it creates really large
  // obstacles if it detects something far away. To prevent planning failures, we would like to clamp
  // the max obstacle width to one half robot width, but that can result in stretching of the clear region
  // cone, which causes some map artifacts. To get around this, define the clear region as the intersection
  // of the sensor cone and a straight beam with obstacle width (which is clamped to robot width).

  //           +-------------+
  //           |             |    ....-----------------------p1--p4
  //           |    robot    |::::                            |xx| (obstacle)
  //           |             |    ''''-----------------------p2--p3
  //           +-------------+

  inline float SensorBeamWidth(float measurement) 
  {
    return measurement * kSensorAperture;
  }

  inline float ObstacleWidth(float measurement) 
  {
    return std::fmin( SensorBeamWidth(measurement), kMaxObstacleWidth_mm);
  }

  std::unique_ptr<BoundedConvexSet2f> GetClearRegion(const Pose2d& robotPose, const Pose2d& objectPose, float measurement) 
  {
    // sensor points
    const float sensorBeamHalfWidth_mm = SensorBeamWidth(measurement) * .5f;
    const float obstacleHalfWidth_mm   = ObstacleWidth(measurement) * .5;

    // use a slightly larger padding for clear than for obstacle to clean up floating point rounding errors
    const Vec2f sensorOffset1(-kObsPadding_x_mm,  -sensorBeamHalfWidth_mm - kObsPadding_y_mm - .5); 
    const Vec2f sensorOffset2(-kObsPadding_x_mm,   sensorBeamHalfWidth_mm + kObsPadding_y_mm + .5);

    const Point2f sensorCorner1 = objectPose.GetTransform() * sensorOffset1;
    const Point2f sensorCorner2 = objectPose.GetTransform() * sensorOffset2; 

    // clear sensor beam
    // NOTE: making an intersection of a quad and a triangle here results in a 50%-100% speedup over using a
    //       five sided polygon, so use that if we can't insert a triangle FP.
    FastPolygon sensorCone{{sensorCorner1, sensorCorner2, robotPose.GetTranslation()}};
    if ( FLT_LT(sensorBeamHalfWidth_mm, kMaxObstacleWidth_mm * .5) ) {
      // the sensor beam width is smaller than the max obstacle size, we don't need to clamp the sides
      return std::make_unique<FastPolygon>(sensorCone);
    } else {
      const Point2f clampOffset1(-measurement,  -obstacleHalfWidth_mm - kObsPadding_y_mm);   
      const Point2f clampOffset2(-measurement,   obstacleHalfWidth_mm + kObsPadding_y_mm);

      const Point2f clampCorner1  = objectPose.GetTransform() * clampOffset1;
      const Point2f clampCorner2  = objectPose.GetTransform() * clampOffset2; 

      // obstacle points
      const Point2f obstacleOffset1(-kObsPadding_x_mm,  -obstacleHalfWidth_mm - kObsPadding_y_mm);   
      const Point2f obstacleOffset2(-kObsPadding_x_mm,   obstacleHalfWidth_mm + kObsPadding_y_mm);

      const Point2f obstacleP1 = objectPose.GetTransform() * obstacleOffset1;
      const Point2f obstacleP2 = objectPose.GetTransform() * obstacleOffset2; 

      auto retv = MakeIntersection2f( sensorCone, FastPolygon{{clampCorner1, clampCorner2, obstacleP2, obstacleP1}} );
      return std::make_unique<decltype(retv)>(retv);
    }
  }

  FastPolygon GetObstacleRegion(const Pose2d& objectPose, u16 measurement) 
  {
    const float obstacleHalfWidth_mm = ObstacleWidth(measurement) * .5;

    const Point2f obstacleLeft( 0,  -obstacleHalfWidth_mm - kObsPadding_y_mm);   
    const Point2f obstacleRight(0,   obstacleHalfWidth_mm + kObsPadding_y_mm);
    const Point2f obstacleDepth( kObsPadding_x_mm * 2, 0);

    const Point2f obstacleP1 = objectPose.GetTransform() * obstacleLeft;
    const Point2f obstacleP2 = objectPose.GetTransform() * obstacleRight; 
    const Point2f obstacleP3 = objectPose.GetTransform() * (obstacleRight + obstacleDepth);
    const Point2f obstacleP4 = objectPose.GetTransform() * (obstacleLeft  + obstacleDepth);

    return FastPolygon{{obstacleP1, obstacleP2, obstacleP3, obstacleP4}};
  }
}

void ProxSensorComponent::UpdateNavMap()
{
  const bool stillUpdating  = _measurementsAtPose < kNumMeasurementsAtPose;         // if the robot hasn't moved but we still need to update the belief state

  // If we know there is no object we can clear the region in front of us.
  // If there is an object, we can clear up to that object
  if ( (_latestData.unobstructed || _latestData.foundObject ) && stillUpdating ) {  
    const float measurement = _latestData.distance_mm;
    const Pose2d objectPose = static_cast<Pose2d>(_currentRobotPose) * Pose2d(0, {measurement, 0});
    const auto clearRegion  = GetClearRegion(_currentRobotPose, objectPose, measurement);

    _robot->GetMapComponent().ClearRegion(*clearRegion, _lastMsgTimestamp );

    if ( _latestData.foundObject ) {
      MemoryMapData_ProxObstacle proxData(MemoryMapData_ProxObstacle::NOT_EXPLORED, objectPose, _lastMsgTimestamp);
      _robot->GetMapComponent().AddProxData( GetObstacleRegion(objectPose, measurement), proxData );
    }
  }
}


} // Cozmo namespace
} // Anki namespace
