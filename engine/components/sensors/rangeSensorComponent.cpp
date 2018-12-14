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
#include "engine/robot.h"
#include "engine/robotComponents_fwd.h"
#include "engine/viz/vizManager.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "whiskeyToF/tof.h"

namespace Anki {
namespace Cozmo {
  
RangeSensorComponent::RangeSensorComponent() 
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RangeSensor)
{
}

void RangeSensorComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  const RangeDataRaw data = ToFSensor::getInstance()->GetData();
 
  Pose3d co = _robot->GetCameraPose(_robot->GetHeadAngle());
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

  #ifndef SIMULATOR
  rp.RotateBy(Rotation3d(DEG_TO_RAD(180), X_AXIS_3D()));
  lp.RotateBy(Rotation3d(DEG_TO_RAD(180), X_AXIS_3D()));
  #endif
  
  // 
  const f32 kInnerAngle_rad = TOF_FOV_RAD / 8.f;
  const f32 kOuterAngle_rad = kInnerAngle_rad * 3.f;
  const f32 kPixToAngle[] = {kOuterAngle_rad,
                             kInnerAngle_rad,
                             -kInnerAngle_rad,
                             -kOuterAngle_rad};

  std::vector<RangeData> navMapData;
  
  for(int r = 0; r < TOF_RESOLUTION; r++)
  {
    const f32 pitch = sin(kPixToAngle[r]);

    for(int c = 0; c < TOF_RESOLUTION; c++)
    {
      RangeData left;
      RangeData right;
      
      const f32 yaw = sin(kPixToAngle[c]);

      const f32 leftDist_mm = data.data[c + (r*8)] * 1000; 

      const f32 yl = yaw * leftDist_mm;
      const f32 zl = pitch * leftDist_mm;

      Pose3d pl(0, Z_AXIS_3D(), {leftDist_mm, yl, zl}, lp, "point");
      Pose3d rootl = pl.GetWithRespectToRoot();
      _robot->GetContext()->GetVizManager()->DrawCuboid(r*8 + c + 1,
                                                        {3, 3, 3},
                                                        rootl);
      left.sensorPoint = lp.GetWithRespectToRoot();
      left.worldPoint = rootl;
      
      const f32 rightDist_mm = data.data[4+c + (r*8)] * 1000;
      const f32 yr = yaw * rightDist_mm;
      const f32 zr = pitch * rightDist_mm;
        
      Pose3d pr(0, Z_AXIS_3D(), {rightDist_mm, yr, zr}, rp, "point");
      Pose3d rootr = pr.GetWithRespectToRoot();
      _robot->GetContext()->GetVizManager()->DrawCuboid(r*8 + c+4 + 1,
                                                        {3, 3, 3},
                                                        rootr);
      right.sensorPoint = rp.GetWithRespectToRoot();
      right.worldPoint = rootr;

      navMapData.push_back(left);
      navMapData.push_back(right);
    }
  }

  UpdateNavMap(navMapData);
}
  
void RangeSensorComponent::UpdateNavMap(const std::vector<RangeData>& data)
{
  for(const auto& rangeData : data)
  {
    (void)rangeData;
    // TODO Find intersection point of vector formed between rangeData.sensorPoint and rangeData.worldPoint
    // and the XY ground plane to update nav map
  }
  
}

} // Cozmo namespace
} // Anki namespace
