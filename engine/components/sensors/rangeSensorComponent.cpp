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
#include "engine/robotStateHistory.h"
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
  // TODO: use historical data for all pose stuff...

  const RangeDataRaw data = ToFSensor::getInstance()->GetData();


  RobotTimeStamp_t t, msgT;
  msgT = _robot->GetLastImageTimeStamp();
  HistRobotState state;
  _robot->GetStateHistory()->ComputeStateAt(data.time, t, state);

  if (msgT != t) {
    const auto msgTc = msgT;
    const auto tc = data.time;
    auto nop = tc + msgTc;
    nop = tc;
  }
 
  Pose3d robotPose = state.GetPose().GetWithRespectToRoot();
  Pose3d co = _robot->GetHistoricalCameraPose(state, t).GetWithRespectToRoot();
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
            Point3f{ TOF_LEFT_TRANS_REL_CAMERA_MM },
            c,
            "leftProx");
  Pose3d rp(rightAngle,
            axis,
            Point3f{ TOF_RIGHT_TRANS_REL_CAMERA_MM },
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
      // Point3f left;
      // Point3f right;
      
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

  
  UpdateNavMap(navMapData, c.GetWithRespectToRoot().GetTranslation(), robotPose.GetTranslation());
}

namespace {
  inline Point3f groundIntersection(Point3f lineOrigin, Vec3f lineDirection) {
    return (lineDirection.z() == 0) ? Point3f{0,0,0}
                                    : lineOrigin - lineDirection * (lineOrigin.z() / lineDirection.z());
  }
} 

void RangeSensorComponent::UpdateNavMap(const std::vector<Point3f>& data, const Point3f& camera, const Point3f& robot)
{
  std::vector<Point2f> groundPlane, cliffPoints, obstaclePoints;
  // int n = 0;
  for(const auto& pt : data) {
    // _robot->GetContext()->GetVizManager()->DrawCuboid(n++, {3, 3, 3}, Pose3d(0, Z_AXIS_3D(), pt), NEAR(pt.z(), 0.f, 2) ? NamedColors::GREEN : NamedColors::RED);

    if (NEAR(pt.z(), 0.f, 2) && ((pt - robot).Length() < 500)) { 
      groundPlane.emplace_back(pt); 
    }

    if (FLT_GT(pt.z(), 5.f) && ((pt - robot).Length() < 500)) { 
      obstaclePoints.emplace_back(pt); 
    }

    if (FLT_LT(pt.z(), -10.f)) { 
      cliffPoints.emplace_back( groundIntersection(camera, pt - camera) ); 
    }
  }

  if (groundPlane.size() >= 2) {
    auto ch = ConvexPolygon::ConvexHull( std::move(groundPlane) );

    // TODO: get actual message timestamp from incoming message
    _robot->GetMapComponent().InsertData(ch, MemoryMapData(MemoryMapTypes::EContentType::ClearOfCliff, _robot->GetLastMsgTimestamp()));
  }

  for(const auto& pt : cliffPoints) {
    Ball2f b( pt, 8.f );

    Pose3d cliffPose(0.f, Z_AXIS_3D(), Point3f(pt.x(), pt.y(), 0.f));
    _robot->GetMapComponent().InsertData( b, MemoryMapData_Cliff(cliffPose, _robot->GetLastMsgTimestamp()) );
  }

  for(const auto& pt : obstaclePoints) {
    Ball2f b( pt, 5.f );

    Pose2d proxPose(0.f, pt);
    _robot->GetMapComponent().InsertData( b, MemoryMapData_ProxObstacle(MemoryMapData_ProxObstacle::NOT_EXPLORED, proxPose, _robot->GetLastMsgTimestamp()) );
  }

}

} // Cozmo namespace
} // Anki namespace
