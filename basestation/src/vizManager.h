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
#include "anki/messaging/shared/UdpClient.h"
#include "anki/cozmo/VizStructs.h"

namespace Anki {
  namespace Cozmo {
    
    
    // NOTE: this is a singleton class
    class VizManager
    {
    public:
      
      ReturnCode Init();
      
      // Get a pointer to the singleton instance
      inline static VizManager* getInstance();
      
      
      // ===== Static object draw functions ====
      
      // Sets the id objectID to correspond to a drawable object of
      // type objectTypeID located at the specified pose.
      // The object will be the
      void SetVizObject(u32 objectID, u32 objectTypeID,
                        const Point3f &size,
                        const Pose3d &pose,
                        u32 colorID = DEFAULT_COLOR_ID);
      
      // Erases the object corresponding to the objectID
      void EraseVizObject(u32 objectID);
      
      // Erases all objects. (Not paths)
      void EraseAllVizObjects();
      
      
      // ===== Path draw functions ====
      
      // Appends the specified line segment to the path with id pathID
      void AppendPathSegmentLine(u32 pathID,
                                 f32 x_start_mm, f32 y_start_mm,
                                 f32 x_end_mm, f32 y_end_m);

      // Appends the specified arc segment to the path with id pathID
      void AppendPathSegmentArc(u32 pathID,
                                f32 x_center_mm, f32 y_center_mm,
                                f32 radius_mm, f32 startRad, f32 sweepRad);
      
      // Sets the color of the path to the one corresponding to colorID
      void SetPathColor(u32 pathID, u32 colorID);
      
      //void ShowPath(u32 pathID, bool show);
      
      //void SetPathHeightOffset(f32 m);

      // Erases the path corresponding to pathID
      void ErasePath(u32 pathID);
      
      // Erases all paths
      void EraseAllPaths();
    
      
      // ==== Color functions =====
      
      // Sets the index colorID to correspond to the specified color vector
      void DefineColor(u32 colorID, f32 red, f32 green, f32 blue, f32 transparency);
      
      void ClearAllColors();
      
    
      
    protected:
      
      // Protected default constructor for singleton.
      VizManager();
      
      static VizManager* singletonInstance_;
      
      bool isInitialized_;
      UdpClient vizClient_;
      
      static const u32 MAX_SIZE_SEND_BUF = 128;
      char sendBuf[MAX_SIZE_SEND_BUF];
      
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