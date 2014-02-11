/**
 * File: vizManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/5/2014
 *
 * Description: Implements the singleton VizManager object. See
 *              corresponding header for more detail.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "vizManager.h"
#include "anki/cozmo/robot/cozmoConfig.h"

namespace Anki {
  namespace Cozmo {
    
    VizManager* VizManager::singletonInstance_ = 0;
    
    ReturnCode VizManager::Init()
    {
      
      if (!vizClient_.Connect(ROBOT_SIM_WORLD_HOST, VIZ_SERVER_PORT)) {
        isInitialized_ = false;
      }
      
      return isInitialized_ ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    
    VizManager::VizManager()
    {
      isInitialized_ = false;
    }
    
    void VizManager::SetVizObject(u32 objectID, u32 objectTypeID,
                                  const Anki::Point3f &size_mm,
                                  const Anki::Pose3d &pose, u32 colorID)
    {
      VizObject v;
      v.objectID = objectID;
      v.objectTypeID = objectTypeID;
      
      v.x_size_m = MM_TO_M(size_mm.x());
      v.y_size_m = MM_TO_M(size_mm.y());
      v.z_size_m = MM_TO_M(size_mm.z());
      
      v.x_trans_m = MM_TO_M(pose.get_translation().x());
      v.y_trans_m = MM_TO_M(pose.get_translation().y());
      v.z_trans_m = MM_TO_M(pose.get_translation().z());
      
      
      // TODO: rotation...
      v.rot_deg = RAD_TO_DEG( pose.get_rotationAngle().ToFloat() );
      v.rot_axis_x = pose.get_rotationAxis().x();
      v.rot_axis_y = pose.get_rotationAxis().y();
      v.rot_axis_z = pose.get_rotationAxis().z();
      
      v.color = colorID;

      //printf("Sending msg %d with %d bytes\n", VizObject_ID, (int)(sizeof(v) + 1));
      
      // TODO: Does this work for poorly packed structs?  Just use Andrew's message class creator?
      sendBuf[0] = VizObject_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }
    
    
    void VizManager::EraseVizObject(u32 objectID)
    {
      VizEraseObject v;
      v.objectID = objectID;
      
      sendBuf[0] = VizEraseObject_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }
    

    void VizManager::EraseAllVizObjects()
    {
      VizEraseObject v;
      v.objectID = ALL_OBJECT_IDs;
      
      sendBuf[0] = VizEraseObject_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }
    
    
    
    
    void VizManager::AppendPathSegmentLine(u32 pathID,
                                           f32 x_start_mm, f32 y_start_mm,
                                           f32 x_end_mm, f32 y_end_mm)
    {
      VizAppendPathSegmentLine v;
      v.pathID = pathID;
      v.x_start_m = MM_TO_M(x_start_mm);
      v.y_start_m = MM_TO_M(y_start_mm);
      v.z_start_m = 0;
      v.x_end_m = MM_TO_M(x_end_mm);
      v.y_end_m = MM_TO_M(y_end_mm);
      v.z_end_m = 0;
      
      sendBuf[0] = VizAppendPathSegmentLine_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }
    
    void VizManager::AppendPathSegmentArc(u32 pathID,
                                          f32 x_center_mm, f32 y_center_mm,
                                          f32 radius_mm, f32 startRad, f32 sweepRad)
    {
      VizAppendPathSegmentArc v;
      v.pathID = pathID;
      v.x_center_m = MM_TO_M(x_center_mm);
      v.y_center_m = MM_TO_M(y_center_mm);
      v.radius_m = MM_TO_M(radius_mm);
      v. start_rad = startRad;
      v. sweep_rad = sweepRad;
      
      
      sendBuf[0] = VizAppendPathSegmentArc_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }
    

    void VizManager::ErasePath(u32 pathID)
    {
      VizErasePath v;
      v.pathID = pathID;
      
      sendBuf[0] = VizErasePath_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }
    

    void VizManager::EraseAllPaths()
    {
      VizErasePath v;
      v.pathID = ALL_PATH_IDs;
      
      sendBuf[0] = VizErasePath_ID;
      memcpy(sendBuf + 1, &v, sizeof(v));
      vizClient_.Send(sendBuf, sizeof(v)+1);
    }

    
    
    
  } // namespace Cozmo
} // namespace Anki