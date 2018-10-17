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

  // make sure these match definitions in the proto
  const Point3f kSensorTranslation_mm = Point3f(18.84, 0.f, -7.96);

  const f32     kSideFov_rad          = DEG_TO_RAD(38);
  const f32     kTilt_rad             = DEG_TO_RAD(30);
  const f32     kRayDirections[4]     = {tan(kSideFov_rad/2.f), tan(kSideFov_rad/6.f), -tan(kSideFov_rad/6.f), -tan(kSideFov_rad/2.f) }; // chunk the FOV into 4 rays

  Rotation3d Get_YRotation_rad(size_t row) {
    return Rotation3d(kRayDirections[row] + kTilt_rad, Y_AXIS_3D());
  }  

  Vec3f Get_XVector(size_t col){ 
    Point3f retv = Point3f(1.f, kRayDirections[col], 0.f);
    retv.MakeUnitLength();
    return retv;
  } 

  Point3f GetRayUnitVector(size_t idx) { 
    size_t row = idx / 4;
    size_t col = idx % 4;
    
    Point3f retv = (Get_YRotation_rad(row) * Get_XVector(col));
    retv.MakeUnitLength();
    return retv;
  }

  /**
 * Determines the point of intersection between a plane defined by a point and a normal vector and a line defined by a point and a direction vector.
 *
 * @param linePoint     A point on the line.
 * @param lineDirection The direction vector of the line.
 * @return The point of intersection between the line and the plane, null if the line is parallel to the plane.
 */
//   Point3f groundIntersection(Point3f linePoint, Point3f lineDirection) {
//     if ( NEAR_ZERO(DotProduct(Z_AXIS_3D(), lineDirection)) ) {
//         return {0,0,0};
//     }

//     lineDirection.MakeUnitLength();
//     float t = DotProduct(Z_AXIS_3D(), linePoint) / DotProduct(Z_AXIS_3D(), lineDirection);
//     return linePoint + lineDirection * t;
// }

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
  Pose3d headPoseInWorld = relativeHeadPose.GetWithRespectToRoot();


  // try and calcuate sensor pose relative to the neck, though we need to manually add head angle
  // Pose3d neckPose  = _robot->GetComponent<FullRobotPose>().GetNeckPose().GetWithRespectToRoot();
  // float  neckAngle = _robot->GetComponent<FullRobotPose>().GetHeadAngle();

  // Transform3d sensorToBody = Transform3d(Rotation3d(-neckAngle, Y_AXIS_3D()), kSensorTranslation_mm) * neckPose.GetTransform();
  // Pose3d headPoseInWorld = relativeHeadPose.GetWithRespectToRoot();



  // check head pose in world coordinates, and verify Roation by comparing to XYZ axis
  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head position %s", headPoseInWorld.GetTranslation().ToString().c_str());

  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head positionX %s", (headPoseInWorld.GetTransform() * X_AXIS_3D() - headPoseInWorld.GetTranslation()).ToString().c_str());
  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head positionY %s", (headPoseInWorld.GetTransform() * Y_AXIS_3D() - headPoseInWorld.GetTranslation()).ToString().c_str());
  // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "head positionZ %s", (headPoseInWorld.GetTransform() * Z_AXIS_3D() - headPoseInWorld.GetTranslation()).ToString().c_str());


  std::vector<Point2f> groundPoints, cliffPoints;
  for (int i = 0; i < 16; ++i) {
    Point3f pointInSensorFrame = GetRayUnitVector(i) * msg.rangeData.depth[i] * 1000 ;
    Point3f pointInWorldFrame = headPoseInWorld.GetTransform() * pointInSensorFrame;

    // from head pose
    // Point3f pointInWorldFrame = sensorToBody * pointInSensorFrame;

    
    
    // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "Found range pixel %d at %s (head frame)", i, pointInHeadFrame.ToString().c_str());
    // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "Found range pixel %d at %s (world frame)", i, pointInWorldFrame.ToString().c_str());

    if ( NEAR(pointInWorldFrame.z(), 0.f, 25.f) && (msg.rangeData.depth[i] < 200)) { 
      groundPoints.emplace_back(pointInWorldFrame); 
    } 

    // add point to cliff set if the ground plane intesection is close enough to the robot
    // if ( FLT_LT(pointInWorldFrame.z(), -100.f) ) { 
    //   // PRINT_NAMED_WARNING("RangeSensorData.ParseIt", "Found range pixel %d at %s (world frame)", i, pointInWorldFrame.ToString().c_str());
    //   // add cliff only if it would have intersected with the ground plane within 300mm
    //   Point3f groundPoint = groundIntersection(sensorToBody.GetTranslation(), GetRayUnitVector(i));
    //   if ((groundPoint - sensorToBody.GetTranslation()).Length() < 300.f) {
    //     cliffPoints.emplace_back(pointInWorldFrame);  
    //   }
    // } 
  }

  // populate driveable space via convex hull of good points
  if (groundPoints.size() > 2) {
    ConvexPolygon ground = ConvexPolygon::ConvexHull( std::move(groundPoints) );
    _robot->GetMapComponent().ClearRegion( FastPolygon(ground),  msg.timestamp);
  }

  // populate drivable space via point inserts with radius
  // for (const auto& p: groundPoints) {
  //   _robot->GetMapComponent().ClearRegion( Ball2f(p, 5.f),  msg.timestamp);
  // }
  
  // populate cliffs as point inserts with radius
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
