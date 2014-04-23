/**
 * File: vizManager.h
 *
 * Author: Kevin Yoon
 * Date:   2/5/2014
 *
 * Description: Implements the VizManager class for vizualizing objects such as 
 *              blocks and robot paths in a Webots simulated world. The Webots 
 *              world needs to invoke the cozmo_physics plugin in order for this to work.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef VIZ_MANAGER_H
#define VIZ_MANAGER_H

#include "anki/common/basestation/math/pose.h"
#include "anki/common/types.h"
#include "anki/vision/CameraSettings.h"
#include "anki/planning/shared/path.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/cozmo/VizStructs.h"

namespace Anki {
  namespace Cozmo {
    
    // List of color IDs
    enum VIZ_COLOR_ID {
      VIZ_COLOR_EXECUTED_PATH,
      VIZ_COLOR_PREDOCKPOSE
    };
    
    // === VizObject ID ranges ===
    // [0,    999]:  Robots
    // [1000, 1099]: Cuboids
    // [1100, 1199]: Ramps
    // [1200, 1299]: PreDockPoses
    const u32 ROBOT_ID_BASE       = 0;
    const u32 CUBOID_ID_BASE      = 1000;
    const u32 RAMP_ID_BASE        = 1100;
    const u32 PREDOCKPOSE_ID_BASE = 1200;    
    
    
    // NOTE: this is a singleton class
    class VizManager
    {
    public:
      
      Result Init();
      
      // Get a pointer to the singleton instance
      inline static VizManager* getInstance();
      
      
      // ===== Convenience object draw functions for specific object types ====
      
      // These convenience functions basically call DrawObject() with the
      // appropriate objectTypeID as well as by offseting the objectID by
      // some base amount so that the caller need not be concerned with
      // making robot and block object IDs that don't collide with each other.
      
      void DrawRobot(const u32 robotID,
                     const Pose3d &pose,
                     const u32 colorID = DEFAULT_COLOR_ID);
      
      void DrawCuboid(const u32 blockID,
                      const Point3f &size,
                      const Pose3d &pose,
                      const u32 colorID = DEFAULT_COLOR_ID);
      
      void DrawPreDockPose(const u32 preDockPoseID,
                           const Pose3d &pose,
                           const u32 colorID = DEFAULT_COLOR_ID);
      
      //void DrawRamp();
      
      
      // ===== Static object draw functions ====
      
      // Sets the id objectID to correspond to a drawable object of
      // type objectTypeID (See VizObjectTypes) located at the specified pose.
      // For parameterized types, like VIZ_CUBOID, size determines the dimensions
      // of the object. For other types, like VIZ_ROBOT, size is ignored.
      void DrawObject(const u32 objectID, const u32 objectTypeID,
                      const Point3f &size,
                      const Pose3d &pose,
                      const u32 colorID = DEFAULT_COLOR_ID);
      
      // Erases the object corresponding to the objectID
      void EraseVizObject(const u32 objectID);
      
      // Erases all objects. (Not paths)
      void EraseAllVizObjects();
      
      
      // ===== Path draw functions ====
      
      void DrawPath(const u32 pathID,
                    const Planning::Path& p,
                    const u32 colorID = DEFAULT_COLOR_ID);
      
      // Appends the specified line segment to the path with id pathID
      void AppendPathSegmentLine(const u32 pathID,
                                 const f32 x_start_mm, const f32 y_start_mm,
                                 const f32 x_end_mm, const f32 y_end_m);

      // Appends the specified arc segment to the path with id pathID
      void AppendPathSegmentArc(const u32 pathID,
                                const f32 x_center_mm, const f32 y_center_mm,
                                const f32 radius_mm, const f32 startRad, const f32 sweepRad);
      
      // Sets the color of the path to the one corresponding to colorID
      void SetPathColor(const u32 pathID, const u32 colorID);
      
      //void ShowPath(u32 pathID, bool show);
      
      //void SetPathHeightOffset(f32 m);

      // Erases the path corresponding to pathID
      void ErasePath(const u32 pathID);
      
      // Erases all paths
      void EraseAllPaths();
    
      // ==== Text functions =====
      void SetText(const u32 labelID, const u32 colorID, const char* format, ...);
      
      
      // ==== Color functions =====
      
      // Sets the index colorID to correspond to the specified color vector
      void DefineColor(const u32 colorID,
                       const f32 red, const f32 green, const f32 blue,
                       const f32 alpha);
      
      //void ClearAllColors();

      
      // ==== Misc. Debug functions =====
      void SetDockingError(const f32 x_dist, const f32 y_dist, const f32 angle);

      void SendGreyImage(const u8* data, const Vision::CameraResolution res);
      
    protected:
      
      // Protected default constructor for singleton.
      VizManager();
      
      void SendMessage(u8 vizMsgID, void* msg);
      
      static VizManager* singletonInstance_;
      
      bool isInitialized_;
      UdpClient vizClient_;
      
      char sendBuf[MAX_VIZ_MSG_SIZE];

      // Image sending
      u8 imgID;
      
    }; // class VizManager
    
    
    inline VizManager* VizManager::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == singletonInstance_) {
        singletonInstance_ = new VizManager();
      }
      
      return singletonInstance_;
    }
    
  } // namespace Cozmo
} // namespace Anki


#endif // VIZ_MANAGER_H