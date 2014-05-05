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
#include "anki/common/basestation/general.h"

namespace Anki {
  namespace Cozmo {
    
    VizManager* VizManager::singletonInstance_ = 0;
    
    Result VizManager::Init()
    {
      
      if (!vizClient_.Connect(ROBOT_SIM_WORLD_HOST, VIZ_SERVER_PORT)) {
        PRINT_INFO("Failed to init VizManager client (%s:%d)\n", ROBOT_SIM_WORLD_HOST, VIZ_SERVER_PORT);
        isInitialized_ = false;
      }
    
      // Define colors
      DefineColor(VIZ_COLOR_EXECUTED_PATH, 1.0, 0.0, 0.0, 1.0);
      DefineColor(VIZ_COLOR_PREDOCKPOSE,   1.0, 0.0, 0.0, 0.75);
      
      return isInitialized_ ? RESULT_OK : RESULT_FAIL;
    }
    
    VizManager::VizManager()
    {
      isInitialized_ = false;
      imgID = 0;
    }

    void VizManager::SendMessage(u8 vizMsgID, void* msg)
    {
      //printf("Sending viz msg %d with %d bytes\n", vizMsgID, msgSize + 1);
      
      // TODO: Does this work for poorly packed structs?  Just use Andrew's message class creator?
      u32 msgSize = Anki::Cozmo::VizMsgLookupTable_[vizMsgID].size;
      
      sendBuf[0] = vizMsgID;
      memcpy(sendBuf + 1, msg, msgSize);
      if (vizClient_.Send(sendBuf, msgSize+1) <= 0) {
        printf("Send msg %d of size %d failed\n", vizMsgID, msgSize+1);
      }
    }

    
    void VizManager::ShowObjects(bool show)
    {
      VizShowObjects v;
      v.show = show ? 1 : 0;
      
      SendMessage(GET_MESSAGE_ID(VizShowObjects), &v);
    }
    
    
    // ===== Robot drawing function =======
    
    void VizManager::DrawRobot(const u32 robotID,
                               const Pose3d &pose,
                               const f32 headAngle,
                               const f32 liftAngle)
    {
      VizSetRobot v;
      
      v.robotID = robotID;
      
      v.x_trans_m = MM_TO_M(pose.get_translation().x());
      v.y_trans_m = MM_TO_M(pose.get_translation().y());
      v.z_trans_m = MM_TO_M(pose.get_translation().z());
      
      v.rot_rad = pose.get_rotationAngle().ToFloat();
      v.rot_axis_x = pose.get_rotationAxis().x();
      v.rot_axis_y = pose.get_rotationAxis().y();
      v.rot_axis_z = pose.get_rotationAxis().z();

      v.head_angle = headAngle;
      v.lift_angle = liftAngle;
      
      SendMessage(GET_MESSAGE_ID(VizSetRobot), &v);
    }
    
    
    // ===== Convenience object draw functions for specific object types ====
    
    void VizManager::DrawRobot(const u32 robotID,
                               const Pose3d &pose,
                               const u32 colorID)
    {
      CORETECH_ASSERT(robotID < CUBOID_ID_BASE);
      
      Anki::Point3f dims; // junk
      DrawObject(ROBOT_ID_BASE + robotID,
                 VIZ_ROBOT,
                 dims,
                 pose,
                 colorID);
    }
    
    void VizManager::DrawCuboid(const u32 blockID,
                                const Point3f &size,
                                const Pose3d &pose,
                                const u32 colorID)
    {
      CORETECH_ASSERT(blockID < (RAMP_ID_BASE - CUBOID_ID_BASE));
      
      DrawObject(CUBOID_ID_BASE + blockID,
                 VIZ_CUBOID,
                 size,
                 pose,
                 colorID);
    }
    
    void VizManager::DrawPreDockPose(const u32 preDockPoseID,
                                     const Pose3d &pose,
                                     const u32 colorID)
    {
      //CORETECH_ASSERT(preDockPoseID < ([[NEXT_OBJECT_ID_BASE]] - PREDOCKPOSE_ID_BASE));
      
      Anki::Point3f dims; // junk
      DrawObject(PREDOCKPOSE_ID_BASE + preDockPoseID,
                 VIZ_PREDOCKPOSE,
                 dims,
                 pose,
                 colorID);
    }
    
    
    // ================== Object drawing methods ====================
    
    void VizManager::DrawObject(const u32 objectID,
                                const u32 objectTypeID,
                                const Anki::Point3f &size_mm,
                                const Anki::Pose3d &pose,
                                const u32 colorID)
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
      
      SendMessage( GET_MESSAGE_ID(VizObject), &v );
    }
    
    
    void VizManager::EraseVizObject(const u32 objectID)
    {
      VizEraseObject v;
      v.objectID = objectID;
      
      SendMessage( GET_MESSAGE_ID(VizEraseObject), &v );
    }
    

    void VizManager::EraseAllVizObjects()
    {
      VizEraseObject v;
      v.objectID = ALL_OBJECT_IDs;
      
      SendMessage( GET_MESSAGE_ID(VizEraseObject), &v );
    }
    
    // ================== Path drawing methods ====================
    
    void VizManager::DrawPath(const u32 pathID,
                              const Planning::Path& p,
                              const u32 colorID)
    {
      ErasePath(pathID);
      
      for (int s=0; s < p.GetNumSegments(); ++s) {
        const Planning::PathSegmentDef& seg = p[s].GetDef();
        switch(p[s].GetType()) {
          case Planning::PST_LINE:
            AppendPathSegmentLine(pathID,
                                  seg.line.startPt_x, seg.line.startPt_y,
                                  seg.line.endPt_x, seg.line.endPt_y);
            break;
          case Planning::PST_ARC:
            AppendPathSegmentArc(pathID,
                                 seg.arc.centerPt_x, seg.arc.centerPt_y,
                                 seg.arc.radius,
                                 seg.arc.startRad,
                                 seg.arc.sweepRad);
            break;
          default:
            break;
        }
      }
      
      SetPathColor(pathID, colorID);
    }
    
    
    void VizManager::AppendPathSegmentLine(const u32 pathID,
                                           const f32 x_start_mm, const f32 y_start_mm,
                                           const f32 x_end_mm, const f32 y_end_mm)
    {
      VizAppendPathSegmentLine v;
      v.pathID = pathID;
      v.x_start_m = MM_TO_M(x_start_mm);
      v.y_start_m = MM_TO_M(y_start_mm);
      v.z_start_m = 0;
      v.x_end_m = MM_TO_M(x_end_mm);
      v.y_end_m = MM_TO_M(y_end_mm);
      v.z_end_m = 0;
      
      SendMessage( GET_MESSAGE_ID(VizAppendPathSegmentLine), &v );
    }
    
    void VizManager::AppendPathSegmentArc(const u32 pathID,
                                          const f32 x_center_mm, const f32 y_center_mm,
                                          const f32 radius_mm, const f32 startRad, const f32 sweepRad)
    {
      VizAppendPathSegmentArc v;
      v.pathID = pathID;
      v.x_center_m = MM_TO_M(x_center_mm);
      v.y_center_m = MM_TO_M(y_center_mm);
      v.radius_m = MM_TO_M(radius_mm);
      v. start_rad = startRad;
      v. sweep_rad = sweepRad;
      
      
      SendMessage( GET_MESSAGE_ID(VizAppendPathSegmentArc), &v );
    }
    

    void VizManager::ErasePath(const u32 pathID)
    {
      VizErasePath v;
      v.pathID = pathID;
      
      SendMessage( GET_MESSAGE_ID(VizErasePath), &v );
    }

    void VizManager::EraseAllPaths()
    {
      VizErasePath v;
      v.pathID = ALL_PATH_IDs;
      
      SendMessage( GET_MESSAGE_ID(VizErasePath), &v );
    }


    void VizManager::SetPathColor(const u32 pathID, const u32 colorID)
    {
      VizSetPathColor v;
      v.pathID = pathID;
      v.colorID = colorID;
      
      SendMessage( GET_MESSAGE_ID(VizSetPathColor), &v );
    }
    
    // =============== Text methods ==================

    void VizManager::SetText(const u32 labelID, const u32 colorID, const char* format, ...)
    {
      VizSetLabel v;
      v.labelID = labelID;
      v.colorID = colorID;
      
      va_list argptr;
      va_start(argptr, format);
      vsnprintf((char*)v.text, sizeof(v.text), format, argptr);
      va_end(argptr);
      
      SendMessage( GET_MESSAGE_ID(VizSetLabel), &v );
    }
    
    
    // ================== Color methods ====================
    
    // Sets the index colorID to correspond to the specified color vector
    void VizManager::DefineColor(const u32 colorID,
                                 const f32 red, const f32 green, const f32 blue,
                                 const f32 alpha)
    {
      VizDefineColor v;
      v.colorID = colorID;
      v.r = red;
      v.g = green;
      v.b = blue;
      v.alpha = alpha;
      
      SendMessage( GET_MESSAGE_ID(VizDefineColor), &v );
    }
    
    
    // ============== Misc. Debug methods =================
    void VizManager::SetDockingError(const f32 x_dist, const f32 y_dist, const f32 angle)
    {
      VizDockingErrorSignal v;
      v.x_dist = x_dist;
      v.y_dist = y_dist;
      v.angle = angle;
      
      SendMessage( GET_MESSAGE_ID(VizDockingErrorSignal), &v );
    }
    

    void VizManager::SendGreyImage(const u8* data, const Vision::CameraResolution res)
    {
      VizImageChunk v;
      v.resolution = res;
      v.imgId = ++imgID;
      v.chunkId = 0;
      v.chunkSize = MAX_VIZ_IMAGE_CHUNK_SIZE;
      
      s32 bytesToSend = Vision::CameraResInfo[res].width * Vision::CameraResInfo[res].height;
      

      while (bytesToSend > 0) {
        if (bytesToSend < MAX_VIZ_IMAGE_CHUNK_SIZE) {
          v.chunkSize = bytesToSend;
        }
        bytesToSend -= v.chunkSize;

        //printf("Sending CAM image %d chunk %d (size: %d), bytesLeftToSend %d\n", v.imgId, v.chunkId, v.chunkSize, bytesToSend);
        memcpy(v.data, &data[v.chunkId * MAX_VIZ_IMAGE_CHUNK_SIZE], v.chunkSize);
        SendMessage( GET_MESSAGE_ID(VizImageChunk), &v );
        
        ++v.chunkId;
      }
    }
    
  } // namespace Cozmo
} // namespace Anki