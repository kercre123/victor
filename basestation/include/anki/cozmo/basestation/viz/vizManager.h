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

#include "anki/common/basestation/math/fastPolygon2d.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/polygon.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/types.h"
#include "anki/vision/CameraSettings.h"
#include "anki/planning/shared/path.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/cozmo/shared/VizStructs.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

namespace Anki {
  namespace Cozmo {
    
    // NOTE: this is a singleton class
    class VizManager
    {
    public:
      
      typedef enum : u8 {
        ACTION,
        LOCALIZED_TO,
        VISION_MODE,
        BEHAVIOR_STATE
      } TextLabelType;
      
      using Handle_t = u32;
      static const Handle_t INVALID_HANDLE;
      
      // NOTE: Connect() will call Disconnect() first if already connected.
      Result Connect(const char *udp_host_address, const unsigned short port);
      Result Disconnect();
      
      // Get a pointer to the singleton instance
      inline static VizManager* getInstance();
      static void removeInstance();
      
      // Whether or not to display the viz objects
      void ShowObjects(bool show);
      
      
      // NOTE: This DrawRobot is completely different from the convenience
      // function below which is just a wrapper around DrawObject. This one
      // actually sets the pose of a CozmoBot model in the world providing
      // more detailed visualization capabilities.
      void DrawRobot(const u32 robotID,
                     const Pose3d &pose,
                     const f32 headAngle,
                     const f32 liftAngle);
      
      
      // ===== Convenience object draw functions for specific object types ====
      
      // These convenience functions basically call DrawObject() with the
      // appropriate objectTypeID as well as by offseting the objectID by
      // some base amount so that the caller need not be concerned with
      // making robot and block object IDs that don't collide with each other.
      // A "handle" (unique, internal ID) will be returned that can be
      // used later to reference the visualization, e.g. for a call
      // to EraseVizObject.
      
      Handle_t DrawRobot(const u32 robotID,
                         const Pose3d &pose,
                         const ColorRGBA& color = NamedColors::DEFAULT);
      
      Handle_t DrawCuboid(const u32 blockID,
                          const Point3f &size,
                          const Pose3d &pose,
                          const ColorRGBA& color = NamedColors::DEFAULT);
      
      Handle_t DrawPreDockPose(const u32 preDockPoseID,
                               const Pose3d &pose,
                               const ColorRGBA& color = NamedColors::DEFAULT);
      
      Handle_t DrawRamp(const u32 rampID,
                        const f32 platformLength,
                        const f32 slopeLength,
                        const f32 width,
                        const f32 height,
                        const Pose3d& pose,
                        const ColorRGBA& color = NamedColors::DEFAULT);
      
      Handle_t DrawCharger(const u32 chargerID,
                           const f32 platformLength,
                           const f32 slopeLength,
                           const f32 width,
                           const f32 height,
                           const Pose3d& pose,
                           const ColorRGBA& color = NamedColors::DEFAULT);
      
      //void DrawRamp();
      
      
      void EraseRobot(const u32 robotID);
      void EraseCuboid(const u32 blockID);
      void EraseAllCuboids();
      void ErasePreDockPose(const u32 preDockPoseID);
      
      
      // ===== Static object draw functions ====
      
      // Sets the id objectID to correspond to a drawable object of
      // type objectTypeID (See VizObjectTypes) located at the specified pose.
      // For parameterized types, like VIZ_CUBOID, size determines the dimensions
      // of the object. For other types, like VIZ_ROBOT, size is ignored.
      // Up to 4 other parameters can be specified in an array pointed to
      // by params.
      void DrawObject(const u32 objectID, const u32 objectTypeID,
                      const Point3f &size,
                      const Pose3d &pose,
                      const ColorRGBA& color = NamedColors::DEFAULT,
                      const f32* params = nullptr);
      
      // Erases the object corresponding to the objectID
      void EraseVizObject(const Handle_t objectID);
      
      // Erases all objects. (Not paths)
      void EraseAllVizObjects();
      
      // Erase all objects of a certain type
      void EraseVizObjectType(const VizObjectType type);
      
      
      // ===== Path draw functions ====
      
      void DrawPath(const u32 pathID,
                    const Planning::Path& p,
                    const ColorRGBA& color = NamedColors::DEFAULT);
      
      // Appends the specified line segment to the path with id pathID
      void AppendPathSegmentLine(const u32 pathID,
                                 const f32 x_start_mm, const f32 y_start_mm,
                                 const f32 x_end_mm, const f32 y_end_m);

      // Appends the specified arc segment to the path with id pathID
      void AppendPathSegmentArc(const u32 pathID,
                                const f32 x_center_mm, const f32 y_center_mm,
                                const f32 radius_mm, const f32 startRad, const f32 sweepRad);
      
      // Sets the color of the path to the one corresponding to colorID
      void SetPathColor(const u32 pathID, const ColorRGBA& color);
      
      //void ShowPath(u32 pathID, bool show);
      
      //void SetPathHeightOffset(f32 m);

      // Erases the path corresponding to pathID
      void ErasePath(const u32 pathID);
      
      // Erases all paths
      void EraseAllPaths();
      
      // ==== Quad functions =====
    
      
      // Draws a generic 3D quadrilateral
      template<typename T>
      void DrawGenericQuad(const u32 quadID,
                           const Quadrilateral<3,T>& quad,
                           const ColorRGBA& color);

      // Draws a generic 2D quadrilateral in the XY plane at the specified Z height
      template<typename T>
      void DrawGenericQuad(const u32 quadID,
                           const Quadrilateral<2,T>& quad,
                           const T zHeight,
                           const ColorRGBA& color);
      
      // Draw a generic 2D quad in the camera display
      template<typename T>
      void DrawCameraQuad(const u32 quadID,
                          const Quadrilateral<2,T>& quad,
                          const ColorRGBA& color);
      
      template<typename T>
      void DrawMatMarker(const u32 quadID,
                         const Quadrilateral<3,T>& quad,
                         const ColorRGBA& color);
      
      template<typename T>
      void DrawRobotBoundingBox(const u32 quadID,
                                const Quadrilateral<3,T>& quad,
                                const ColorRGBA& color);
      
      template<typename T>
      void DrawPlannerObstacle(const bool isReplan,
                               const u32 quadID,
                               const Polygon<2,T>& poly,
                               const ColorRGBA& color);

      void DrawPlannerObstacle(const bool isReplan,
                               const u32 quadID,
                               const FastPolygon& poly,
                               const ColorRGBA& color);

      template<typename T>
      void DrawPoseMarker(const u32 quadID,
                          const Quadrilateral<2,T>& quad,
                          const ColorRGBA& color);

      
      // Draw quads of a specified type (usually called as a helper by the
      // above methods for specific types)
      template<typename T>
      void DrawQuad(const u32 quadType,
                    const u32 quadID,
                    const Quadrilateral<3,T>& quad,
                    const ColorRGBA& color);

      template<typename T>
      void DrawQuad(const u32 quadType,
                    const u32 quadID,
                    const Quadrilateral<2,T>& quad,
                    const T zHeight,
                    const ColorRGBA& color);

      template<typename T>
      void DrawPoly(const u32 polyID,
                    const Polygon<2,T>& poly,
                    const ColorRGBA& color);

      void DrawPoly(const u32 polyID,
                    const FastPolygon& poly,
                    const ColorRGBA& color);
      
      // Erases the quad with the specified type and ID
      void EraseQuad(const u32 quadType, const u32 quadID);
      
      // Erases all the quads fo the specified type
      void EraseAllQuadsWithType(const u32 quadType);
      
      // Erases all quads
      void EraseAllQuads();
      
      void EraseAllPlannerObstacles(const bool isReplan);
      
      void EraseAllMatMarkers();
    
      // ==== Text functions =====
      void SetText(const TextLabelType& labelType, const ColorRGBA& color, const char* format, ...);
      
      
      // ==== Color functions =====
      /*
      // Sets the index colorID to correspond to the specified color vector
      void DefineColor(const u32 colorID,
                       const f32 red, const f32 green, const f32 blue,
                       const f32 alpha);
      */
      //void ClearAllColors();

        
      // ==== Misc. Debug functions =====
      void SetDockingError(const f32 x_dist, const f32 y_dist, const f32 angle);

      void EnableImageSend(bool tf) { _sendImages = tf; }
      void SendGreyImage(const RobotID_t robotID, const u8* data, const Vision::CameraResolution res, const TimeStamp_t timestamp);
      void SendColorImage(const RobotID_t robotID, const u8* data, const Vision::CameraResolution res, const TimeStamp_t timestamp);
      
      void SendImage(const RobotID_t robotID, const u8* data, const u32 dataLength,
                     const Vision::CameraResolution res,
                     const TimeStamp_t timestamp,
                     const Vision::ImageEncoding_t encoding);
      
      void SendImageChunk(const RobotID_t robotID, MessageImageChunk robotImageChunk);
      
      void SendVisionMarker(const u16 topLeft_x, const u16 topLeft_y,
                            const u16 topRight_x, const u16 topRight_y,
                            const u16 bottomRight_x, const u16 bottomRight_y,
                            const u16 bottomLeft_x, const u16 bottomLeft_y,
                            bool verified);
      
      void SendTrackerQuad(const u16 topLeft_x, const u16 topLeft_y,
                           const u16 topRight_x, const u16 topRight_y,
                           const u16 bottomRight_x, const u16 bottomRight_y,
                           const u16 bottomLeft_x, const u16 bottomLeft_y);
      
      void SendRobotState(const MessageRobotState &msg,
                          const u8 &videoFramefateHz);
      
    protected:
      
      // Protected default constructor for singleton.
      VizManager();
      
      void SendMessage(u8 vizMsgID, void* msg);
      
      static VizManager* _singletonInstance;
      
      bool               _isInitialized;
      UdpClient          _vizClient;
      
      char               _sendBuf[MAX_VIZ_MSG_SIZE];

      // Image sending
      std::map<RobotID_t, u8> _imgID;
      
      bool               _sendImages;
      
      // Stores the maximum ID permitted for a given VizObject type
      u32 _VizObjectMaxID[NUM_VIZ_OBJECT_TYPES];
      
    }; // class VizManager
    
    
    inline VizManager* VizManager::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == _singletonInstance) {
        _singletonInstance = new VizManager();
      }
      
      return _singletonInstance;
    }
    
    template<typename T>
    void VizManager::DrawQuad(const u32 quadType,
                              const u32 quadID,
                              const Quadrilateral<2,T>& quad,
                              const T zHeight_mm,
                              const ColorRGBA& color)
    {
      using namespace Quad;
      VizQuad v;
      v.quadType = quadType;
      v.quadID = quadID;
      
      const f32 zHeight_m = MM_TO_M(static_cast<f32>(zHeight_mm));
      
      v.xUpperLeft  = MM_TO_M(static_cast<f32>(quad[TopLeft].x()));
      v.yUpperLeft  = MM_TO_M(static_cast<f32>(quad[TopLeft].y()));
      v.zUpperLeft  = zHeight_m;
      
      v.xLowerLeft  = MM_TO_M(static_cast<f32>(quad[BottomLeft].x()));
      v.yLowerLeft  = MM_TO_M(static_cast<f32>(quad[BottomLeft].y()));
      v.zLowerLeft  = zHeight_m;
      
      v.xUpperRight = MM_TO_M(static_cast<f32>(quad[TopRight].x()));
      v.yUpperRight = MM_TO_M(static_cast<f32>(quad[TopRight].y()));
      v.zUpperRight = zHeight_m;
      
      v.xLowerRight = MM_TO_M(static_cast<f32>(quad[BottomRight].x()));
      v.yLowerRight = MM_TO_M(static_cast<f32>(quad[BottomRight].y()));
      v.zLowerRight = zHeight_m;
      
      v.color = u32(color);
      
      SendMessage( GET_MESSAGE_ID(VizQuad), &v );
    }

    template<typename T>
    void VizManager::DrawQuad(const u32 quadType,
                              const u32 quadID,
                              const Quadrilateral<3,T>& quad,
                              const ColorRGBA& color)
    {
      using namespace Quad;
      VizQuad v;
      v.quadType = quadType;
      v.quadID = quadID;
      
      v.xUpperLeft  = MM_TO_M(static_cast<f32>(quad[TopLeft].x()));
      v.yUpperLeft  = MM_TO_M(static_cast<f32>(quad[TopLeft].y()));
      v.zUpperLeft  = MM_TO_M(static_cast<f32>(quad[TopLeft].z()));
      
      v.xLowerLeft  = MM_TO_M(static_cast<f32>(quad[BottomLeft].x()));
      v.yLowerLeft  = MM_TO_M(static_cast<f32>(quad[BottomLeft].y()));
      v.zLowerLeft  = MM_TO_M(static_cast<f32>(quad[BottomLeft].z()));
      
      v.xUpperRight = MM_TO_M(static_cast<f32>(quad[TopRight].x()));
      v.yUpperRight = MM_TO_M(static_cast<f32>(quad[TopRight].y()));
      v.zUpperRight = MM_TO_M(static_cast<f32>(quad[TopRight].z()));
      
      v.xLowerRight = MM_TO_M(static_cast<f32>(quad[BottomRight].x()));
      v.yLowerRight = MM_TO_M(static_cast<f32>(quad[BottomRight].y()));
      v.zLowerRight = MM_TO_M(static_cast<f32>(quad[BottomRight].z()));
      
      v.color = u32(color);
      
      SendMessage( GET_MESSAGE_ID(VizQuad), &v );
    }
    
    template<typename T>
    void VizManager::DrawPoly(const u32 __polyID,
                              const Polygon<2,T>& poly,
                              const ColorRGBA& color)
    {
      // we don't have a poly viz message (yet...) so construct a path
      // from the poly, and use the viz path stuff instead

      Planning::Path polyPath;

      // hack! don't want to collide with path ids
      u32 pathId = __polyID + 2200;

      size_t numPts = poly.size();

      for(size_t i=0; i<numPts; ++i) {
        size_t j = (i + 1) % numPts;
        polyPath.AppendLine(0,
                            poly[i].x(), poly[i].y(),
                            poly[j].x(), poly[j].y(),
                            1.0, 1.0, 1.0);
      }

      DrawPath(pathId, polyPath, color);
    }

    
    template<typename T>
    void VizManager::DrawGenericQuad(const u32 quadID,
                                     const Quadrilateral<2,T>& quad,
                                     const T zHeight_mm,
                                     const ColorRGBA& color)
    {
      DrawQuad(VIZ_QUAD_GENERIC_2D, quadID, quad, zHeight_mm, color);
    }
    
    template<typename T>
    void VizManager::DrawGenericQuad(const u32 quadID,
                                     const Quadrilateral<3,T>& quad,
                                     const ColorRGBA& color)
    {
      DrawQuad(VIZ_QUAD_GENERIC_3D, quadID, quad, color);
    }
    
    template<typename T>
    void VizManager::DrawCameraQuad(const u32 quadID,
                                    const Quadrilateral<2,T>& quad,
                                    const ColorRGBA& color)
    {
      using namespace Quad;
      VizCameraQuad v;
      v.quadID = quadID;
      
      v.xUpperLeft  = static_cast<f32>(quad[TopLeft].x());
      v.yUpperLeft  = static_cast<f32>(quad[TopLeft].y());
      
      v.xLowerLeft  = static_cast<f32>(quad[BottomLeft].x());
      v.yLowerLeft  = static_cast<f32>(quad[BottomLeft].y());
      
      v.xUpperRight = static_cast<f32>(quad[TopRight].x());
      v.yUpperRight = static_cast<f32>(quad[TopRight].y());
      
      v.xLowerRight = static_cast<f32>(quad[BottomRight].x());
      v.yLowerRight = static_cast<f32>(quad[BottomRight].y());
      
      v.color = u32(color);
      
      SendMessage( GET_MESSAGE_ID(VizCameraQuad), &v );
    }

    
    template<typename T>
    void VizManager::DrawMatMarker(const u32 quadID,
                                   const Quadrilateral<3,T>& quad,
                                   const ColorRGBA& color)
    {
      DrawQuad(VIZ_QUAD_MAT_MARKER, quadID, quad, color);
    }
    
    template<typename T>
    void VizManager::DrawPlannerObstacle(const bool isReplan,
                                         const u32 polyID,
                                         const Polygon<2,T>& poly,
                                         const ColorRGBA& color)
    {
      // const u32 polyType = (isReplan ? VIZ_QUAD_PLANNER_OBSTACLE_REPLAN : VIZ_QUAD_PLANNER_OBSTACLE);
      
      DrawPoly(polyID, poly, color);
    }

    
    template<typename T>
    void VizManager::DrawRobotBoundingBox(const u32 quadID,
                                          const Quadrilateral<3,T>& quad,
                                          const ColorRGBA& color)
    {
      DrawQuad(VIZ_QUAD_ROBOT_BOUNDING_BOX, quadID, quad, color);
    }

    template<typename T>
    void VizManager::DrawPoseMarker(const u32 quadID,
                                    const Quadrilateral<2,T>& quad,
                                    const ColorRGBA& color)
    {
      DrawQuad(VIZ_QUAD_POSE_MARKER, quadID, quad, 0.5f, color);
    }
    
  } // namespace Cozmo
} // namespace Anki


#endif // VIZ_MANAGER_H
