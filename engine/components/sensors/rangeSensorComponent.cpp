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

// #include "engine/components/carryingComponent.h"
#include "engine/components/sensors/rangeSensorComponent.h"

#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"

// #include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/math/convexIntersection.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kLogDirName = "rangeSensor";


  const Point3f kSensorTranslation_mm = Point3f(18.84, 0.f, -7.96);
  const Point3f kHeadTranslation_mm   = Point3f(-13.f, 0.f, 34.5f);

  const f32     kSideFov_rad          = DEG_TO_RAD(38);
  const f32     kTilt_rad             = DEG_TO_RAD(30);
  const f32     kRayDirections[4]     = {tan(kSideFov_rad/2.f), tan(kSideFov_rad/6.f), -tan(kSideFov_rad/6.f), -tan(kSideFov_rad/2.f) };

  // const Point2i resolution = {4, 4};

  // const Pose3d kSensorPose1(DEG_TO_RAD(-30), Y_AXIS_3D(), Point3f(0.008, 0.006. -0.01));
  // const Rotation3d kRayOffsets[16];

  Rotation3d Get_YRotation_rad(size_t row) {
    return Rotation3d(kRayDirections[row] + kTilt_rad, Y_AXIS_3D());
    // switch(row) {
    //   case 0:  return Rotation3d( 0.165806        + kTilt_rad, Y_AXIS_3D());
    //   case 1:  return Rotation3d( 0.0552687602281 + kTilt_rad, Y_AXIS_3D());
    //   case 2:  return Rotation3d(-0.0552687602281 + kTilt_rad, Y_AXIS_3D());
    //   case 3:  return Rotation3d(-0.165806        + kTilt_rad, Y_AXIS_3D());
    //   default: return Rotation3d(0.f, Y_AXIS_3D());
    // }
  }  

  Vec3f Get_XVector(size_t col){ 
    Point3f retv = Point3f(1.f, kRayDirections[col], 0.f);
    retv.MakeUnitLength();
    return retv;
    // switch(col) {
    //   case 0:  return Point3f(0.986286, 2 *  0.165048,  0.f);
    //   case 1:  return Point3f(0.998473, 2 *  0.0552406, 0.f);
    //   case 2:  return Point3f(0.998473, 2 * -0.0552406, 0.f);
    //   case 3:  return Point3f(0.986286, 2 * -0.165048,  0.f);
    //   default: return {1.f, 0.f, 0.f};
    // }
  } 

  Point3f GetRayUnitVector(size_t idx) { 
    size_t row = idx / 4;
    size_t col = idx % 4;
    
    Point3f retv = (Get_YRotation_rad(row) * Get_XVector(col));
    retv.MakeUnitLength();
    return retv;
  }
} // end anonymous namespace


RangeSensorComponent::RangeSensorComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RangeSensor)
{
}


void RangeSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  // _lastMsgTimestamp = msg.timestamp;
  // _latestDataRaw = msg.rangeData;

  // camera uses x as right, y as down, and z as forward, so convert it to map orietiation, where
  // x is forward, y is left, and z is up
  // (Grabbed from fullRobotPose.h)
  RotationMatrix3d kDefaultHeadCamRotation({
    0,      -0.0698f,  0.9976f,
  -1.0000f,  0,        0,
    0,      -0.9976f, -0.0698f,
  });

  Pose3d relativeHeadPose = _robot->GetCameraPose(_robot->GetComponent<FullRobotPose>().GetHeadAngle());
  relativeHeadPose.RotateBy(kDefaultHeadCamRotation.Invert());
  // Pose3d relativeHeadPose = _robot->GetComponent<FullRobotPose>().GetNeckPose();


  Pose3d headPoseInWorld = relativeHeadPose.GetWithRespectToRoot();

  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head position %s", headPoseInWorld.GetTranslation().ToString().c_str());

  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head positionX %s", (headPoseInWorld.GetTransform() * X_AXIS_3D() - headPoseInWorld.GetTranslation()).ToString().c_str());
  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head positionY %s", (headPoseInWorld.GetTransform() * Y_AXIS_3D() - headPoseInWorld.GetTranslation()).ToString().c_str());
  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head positionZ %s", (headPoseInWorld.GetTransform() * Z_AXIS_3D() - headPoseInWorld.GetTranslation()).ToString().c_str());


  std::vector<Point2f> groundPoints, cliffPoints;
  for (int i = 0; i < 16; ++i) {
    Point3f pointInHeadFrame = GetRayUnitVector(i) * msg.rangeData.depth[i] * 1000 ;
    Point3f pointInWorldFrame = headPoseInWorld.GetTransform() * pointInHeadFrame;
    
    // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "Found range pixel %d at %s (head frame)", i, pointInHeadFrame.ToString().c_str());
    // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "Found range pixel %d at %s (world frame)", i, pointInWorldFrame.ToString().c_str());

    if ( NEAR(pointInWorldFrame.z(), 0.f, 20.f) && (msg.rangeData.depth[i] < 200)) { 
      groundPoints.emplace_back(pointInWorldFrame); 
    } 

    if ( FLT_LT(pointInWorldFrame.z(), -100.f) ) { 
      PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "Found range pixel %d at %s (world frame)", i, pointInWorldFrame.ToString().c_str());
      cliffPoints.emplace_back(pointInWorldFrame); 
    } 
  }

  if (groundPoints.size() > 2) {
    ConvexPolygon ground = ConvexPolygon::ConvexHull( std::move(groundPoints) );
    _robot->GetMapComponent().ClearRegion( FastPolygon(ground),  msg.timestamp);
  }

  // for (const auto& p: groundPoints) {
  //   _robot->GetMapComponent().ClearRegion( FastPolygon({p}),  msg.timestamp);
  // }

  // for (const auto& p: cliffPoints) {
  //   _robot->GetMapComponent().InsertData( Ball2f(p, 5.f),  MemoryMapData_Cliff( _robot->GetPose(), msg.timestamp));
  // }

}


std::string RangeSensorComponent::GetLogHeader()
{
  return std::string("");
}


std::string RangeSensorComponent::GetLogRow()
{
  // const auto& d = _latestDataRaw;
  std::stringstream ss;
  return ss.str();
}


} // Cozmo namespace
} // Anki namespace
