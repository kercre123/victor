/**
 * File: rangeSensorComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Component for managing Whiskey's head mounted ToF sensor
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/sensors/rangeSensorComponent.h"

#include "engine/cozmoContext.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"
#include "engine/robot.h"
#include "engine/robotComponents_fwd.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/viz/vizManager.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include "whiskeyToF/tof.h"

namespace Anki {
namespace Vector {
  
RangeSensorComponent::RangeSensorComponent() 
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RangeSensor)
{
}

RangeSensorComponent::~RangeSensorComponent()
{
  ToFSensor::removeInstance();
}

void RangeSensorComponent::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;

  // Subscribe to the SendRangeData request
  auto sendRangeDataLambda = [this](const AnkiEvent<RobotInterface::RobotToEngine>& event)
                             {
                               auto* tof = ToFSensor::getInstance();
                               if(tof == nullptr)
                               {
                                 return;
                               }
                               
                               _sendRangeData = event.GetData().Get_sendRangeData().enable;
                               if(_sendRangeData)
                               {
                                 tof->StartRanging(nullptr);
                               }
                               else
                               {
                                 tof->StopRanging(nullptr);
                               }
                             };
  
  _signalHandle = _robot->GetRobotMessageHandler()->Subscribe(RobotInterface::RobotToEngineTag::sendRangeData,
                                                              sendRangeDataLambda);

}
  


void RangeSensorComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  auto* tof = ToFSensor::getInstance();
  if(tof == nullptr)
  {
    return;
  }
  
  _latestRawRangeData = tof->GetData(_rawDataIsNew);

  // If we have been requested to send range data, then populate a RangeDataToDisplay message
  if(_sendRangeData)
  {
    using namespace RobotInterface;

    RangeDataToDisplay msg;
    auto& disp = msg.data.data;
    memset(&disp, 0, sizeof(disp));
    
    for(const auto& e : _latestRawRangeData.data)
    {
      disp[e.roi].signalRate_mcps = -1;
      disp[e.roi].status = 99;
      for(const auto& reading : e.readings)
      {
        // Use the status and signalRate from the reading that corresponds to the processedRange_mm
        if(reading.rawRange_mm == e.processedRange_mm)
        {
          disp[e.roi].signalRate_mcps = reading.signalRate_mcps;
          disp[e.roi].status = reading.status;
        }
      }

      disp[e.roi].processedRange_mm = e.processedRange_mm;
      disp[e.roi].spadCount = e.spadCount;
      disp[e.roi].roi = e.roi;
      disp[e.roi].roiStatus = e.roiStatus;
    }

    _robot->SendRobotMessage<RangeDataToDisplay>(msg);
  }
  
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

  #ifndef SIMULATOR
  //rp.RotateBy(Rotation3d(DEG_TO_RAD(180), X_AXIS_3D()));
  //lp.RotateBy(Rotation3d(DEG_TO_RAD(180), X_AXIS_3D()));
  #endif
  
  // 
  // const f32 kInnerAngle_rad = TOF_FOV_RAD / 8.f;
  // const f32 kOuterAngle_rad = kInnerAngle_rad * 3.f;
  // const f32 kPixToAngle[] = {kOuterAngle_rad,
  //                            kInnerAngle_rad,
  //                            -kInnerAngle_rad,
  //                            -kOuterAngle_rad};

  // std::vector<RangeData> navMapData;
  
  // for(int r = 0; r < TOF_RESOLUTION; r++)
  // {
  //   const f32 pitch = sin(kPixToAngle[r]);

  //   for(int c = 0; c < TOF_RESOLUTION; c++)
  //   {
  //     const f32 yaw = sin(kPixToAngle[c]);

  //     const f32 leftDist_mm = _latestRawRangeData.data[c + (r*8)].processedRange_mm; 

  //     const f32 yl = yaw * leftDist_mm;
  //     const f32 zl = pitch * leftDist_mm;

  //     Pose3d pl(0, Z_AXIS_3D(), {leftDist_mm, yl, zl}, lp, "point");
  //     Pose3d rootl = pl.GetWithRespectToRoot();
  //     _robot->GetContext()->GetVizManager()->DrawCuboid(r*8 + c + 1,
  //                                                       {3, 3, 3},
  //                                                       rootl);
  //     _latestRangeData[r*8 + c] = pl.GetTranslation();      
  //   }
  // }
}

} // Cozmo namespace
} // Anki namespace
