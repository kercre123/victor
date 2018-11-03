/**
 * File: rangeSensorComponent.cpp
 *
 * Author: Matt Michini
 * Created: 8/30/2017
 *
 * Description: Component for managing forward distance sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/sensors/rangeSensorComponent.h"

#include "engine/cozmoContext.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "engine/robot.h"
#include "engine/robotComponents_fwd.h"
#include "engine/viz/vizManager.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "whiskeyToF/tof.h"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kLogDirName = "rangeSensor";
}

RangeSensorComponent::RangeSensorComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RangeSensor)
{
}

std::string RangeSensorComponent::GetLogHeader()
{
  return std::string("");
}


std::string RangeSensorComponent::GetLogRow()
{
  return "";
}

void RangeSensorComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  const RangeDataRaw data = ToFSensor::getInstance()->GetData();
 
  Pose3d co = _robot->GetCameraPose(_robot->GetComponent<FullRobotPose>().GetHeadAngle());
  // Parent a pose to the camera so we can rotate our current camera axis (Z out of camera) to match world axis (Z up)
  // also account for angle tof sensor is relative to camera
  Pose3d c(TOF_ANGLE_DOWN_REL_CAMERA_RAD, Y_AXIS_3D(), {0, 0, 0}, co);
  c.RotateBy(Rotation3d(DEG_TO_RAD(-90), Y_AXIS_3D()));
  c.RotateBy(Rotation3d(DEG_TO_RAD(90), Z_AXIS_3D()));

#if TOF_CONFIGURATION ==TOF_SIDE_BY_SIDE
  const auto leftAngle = TOF_LEFT_ROT_Z_REL_CAMERA_RAD;
  const auto rightAngle = TOF_RIGHT_ROT_Z_REL_CAMERA_RAD;
  const auto axis = Z_AXIS_3D();  
#elif TOF_CONFIGURATION == TOF_ABOVE_BELOW || TOF_CONFIGURATION == TOF_CENTER_OF_FACE
  const auto leftAngle = TOF_LEFT_ROT_Y_REL_CAMERA_RAD;
  const auto rightAngle = TOF_RIGHT_ROT_Y_REL_CAMERA_RAD;
  const auto axis = Y_AXIS_3D();
#endif

  // Construct left and right sensor poses parented to the rotated camera pose
  Pose3d lp(leftAngle,
            axis,
            {TOF_LEFT_TRANS_REL_CAMERA_MM[0],
             TOF_LEFT_TRANS_REL_CAMERA_MM[1],
             TOF_LEFT_TRANS_REL_CAMERA_MM[2]},
            c,
            "leftProx");
  Pose3d rp(rightAngle,
            axis,
            {TOF_RIGHT_TRANS_REL_CAMERA_MM[0],
             TOF_RIGHT_TRANS_REL_CAMERA_MM[1],
             TOF_RIGHT_TRANS_REL_CAMERA_MM[2]},
            c,
            "rightProx");

  // 
  const f32 kInnerAngle_rad = TOF_FOV_RAD / 8.f;
  const f32 kOuterAngle_rad = kInnerAngle_rad * 3.f;
  const f32 kPixToAngle[] = {kOuterAngle_rad,
                             kInnerAngle_rad,
                             -kInnerAngle_rad,
                             -kOuterAngle_rad};

  std::vector<Point3f> navMapData;
  
  for(int r = 0; r < TOF_RESOLUTION; r++)
  {
    const f32 pitch = sin(kPixToAngle[r]);

    for(int c = 0; c < TOF_RESOLUTION; c++)
    {
      Point3f left;
      Point3f right;
      
      const f32 yaw = sin(kPixToAngle[c]);

      const f32 leftDist_mm = data.data[c + (r*8)] * 1000; 
      const f32 yl = yaw * leftDist_mm;
      const f32 zl = pitch * leftDist_mm;

      Pose3d pl(0, Z_AXIS_3D(), {leftDist_mm, yl, zl}, lp, "point");
      Pose3d rootl = pl.GetWithRespectToRoot();

      const f32 rightDist_mm = data.data[4+c + (r*8)] * 1000;
      const f32 yr = yaw * rightDist_mm;
      const f32 zr = pitch * rightDist_mm;
        
      Pose3d pr(0, Z_AXIS_3D(), {rightDist_mm, yr, zr}, rp, "point");
      Pose3d rootr = pr.GetWithRespectToRoot();

      navMapData.push_back(rootl.GetTranslation());
      navMapData.push_back(rootr.GetTranslation());
    }
  }

  UpdateNavMap(navMapData);
}

namespace {
  Point3f lineIntersection(Point3f lineOrigin, Vec3f lineDirection) {

    if (lineDirection.z() == 0) {
        return {0,0,0};
    }

    Point3f normal(0,0,1);
    float t = DotProduct(normal, lineOrigin) / DotProduct(normal, lineDirection);
    return lineOrigin + (lineDirection * t);
  }
} 

void RangeSensorComponent::UpdateNavMap(const std::vector<Point3f>& data)
{
  int n = 0;
  std::vector<Point2f> groundPlane, cliffPoints;
  Point3f robotPt = _robot->GetPose().GetTranslation();
  Point3f camera = _robot->GetCameraPose(_robot->GetComponent<FullRobotPose>().GetHeadAngle()).GetTranslation() + _robot->GetPose().GetTranslation();
  for(const auto& pt : data) {
    _robot->GetContext()->GetVizManager()->DrawCuboid(n++, {3, 3, 3}, Pose3d(0, Z_AXIS_3D(), pt), NEAR(pt.z(), 0.f, 2) ? NamedColors::GREEN : NamedColors::RED);
    if (NEAR(pt.z(), 0.f, 2) && ((pt - robotPt).Length() < 500)) { groundPlane.emplace_back(pt); }
    if (FLT_LT(pt.z(), -10.f)) { 
      // Get ground plane intersection
      Vec3f ray = pt - camera;
      ray.MakeUnitLength();
      cliffPoints.emplace_back( lineIntersection(camera, ray) ); 
    }
  }

  if (groundPlane.size() >= 2) {
    auto ch = ConvexPolygon::ConvexHull( std::move(groundPlane) );

    // TODO: get actual message timestamp from incoming message
    _robot->GetMapComponent().InsertData(ch, MemoryMapData(MemoryMapTypes::EContentType::ClearOfObstacle, _robot->GetLastMsgTimestamp()));
  }

  for(const auto& pt : cliffPoints) {
    Ball2f b( pt, 4.f );
    Pose3d cliffPose(0.f, Z_AXIS_3D(), Point3f(pt.x(), pt.y(), 0.f));

    // TODO: get actual message timestamp from incoming message
    _robot->GetMapComponent().InsertData( b, MemoryMapData_Cliff(cliffPose, _robot->GetLastMsgTimestamp()) );
  }

}

} // Cozmo namespace
} // Anki namespace
