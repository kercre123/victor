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
#include "anki/cozmo/shared/cozmoTypes.h"
#include "clad/types/imageTypes.h"
#include "clad/types/vizTypes.h"
#include "clad/types/objectTypes.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/vizInterface/messageViz.h"

namespace Anki {
  
  // Forward declaration
  namespace Vision {
    class TrackedFace;
  }
  
  namespace Cozmo {

  namespace VizInterface {
  class MessageViz;
  enum class MessageVizTag : uint8_t;
  } // end namespace VizInterface

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
                         const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
      Handle_t DrawCuboid(const u32 blockID,
                          const Point3f &size,
                          const Pose3d &pose,
                          const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
      Handle_t DrawPreDockPose(const u32 preDockPoseID,
                               const Pose3d &pose,
                               const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
      Handle_t DrawRamp(const u32 rampID,
                        const f32 platformLength,
                        const f32 slopeLength,
                        const f32 width,
                        const f32 height,
                        const Pose3d& pose,
                        const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
      Handle_t DrawCharger(const u32 chargerID,
                           const f32 platformLength,
                           const f32 slopeLength,
                           const f32 width,
                           const f32 height,
                           const Pose3d& pose,
                           const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
      Handle_t DrawHumanHead(const u32 headID,
                             const Point3f& size,
                             const Pose3d& pose,
                             const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
      void DrawCameraFace(const Vision::TrackedFace& face,
                          const ColorRGBA& color);
      
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
      void DrawObject(const u32 objectID, VizObjectType objectTypeID,
        const Point3f &size,
        const Pose3d &pose,
        const ColorRGBA& color = ::Anki::NamedColors::DEFAULT,
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
                    const ColorRGBA& color = ::Anki::NamedColors::DEFAULT);
      
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
      void DrawCameraQuad(const Quadrilateral<2,T>& quad,
                          const ColorRGBA& color);
      
      // Draw a line segment in the camera display
      void DrawCameraLine(const Point2f& start,
                          const Point2f& end,
                          const ColorRGBA& color);
      
      // Draw an oval in the camera display
      void DrawCameraOval(const Point2f& center,
                          float xRadius, float yRadius,
                          const ColorRGBA& color);
      
      // Draw text in the camera display
      void DrawCameraText(const Point2f& position,
                          const std::string& text,
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
      void DrawQuad(const VizQuadType quadType, const u32 quadID, const Quadrilateral<2,T>& quad,
        const T zHeight_mm, const ColorRGBA& color);

      template<typename T>
      void DrawQuad(const VizQuadType quadType, const u32 quadID, const Quadrilateral<3,T>& quad,
        const ColorRGBA& color);

      template<typename T>
      void DrawPoly(const u32 polyID,
                    const Polygon<2,T>& poly,
                    const ColorRGBA& color);

      void DrawPoly(const u32 polyID,
                    const FastPolygon& poly,
                    const ColorRGBA& color);
      
      void ErasePoly(u32 polyID);
      
      // Erases the quad with the specified type and ID
      void EraseQuad(const uint32_t quadType, const u32 quadID);
      
      // Erases all the quads fo the specified type
      void EraseAllQuadsWithType(const uint32_t quadType);
      
      // Erases all quads
      void EraseAllQuads();
      
      void EraseAllPlannerObstacles(const bool isReplan);
      
      void EraseAllMatMarkers();
      
      // ==== Circle functions =====
      template<typename T>
      void DrawXYCircle(u32 polyID,
                      const ColorRGBA& color,
                      const Point<2, T>& center,
                      const T radius,
                      u32 numSegments = 20);
      
      void EraseCircle(u32 polyID);
    
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
      /*
      void SendGreyImage(const RobotID_t robotID, const u8* data, const Vision::CameraResolution res, const TimeStamp_t timestamp);
      void SendColorImage(const RobotID_t robotID, const u8* data, const Vision::CameraResolution res, const TimeStamp_t timestamp);

      void SendImage(const RobotID_t robotID, const u8* data, const u32 dataLength,
                     const Vision::CameraResolution res,
                     const TimeStamp_t timestamp,
                     const Vision::ImageEncoding_t encoding);
      */

      void SendImageChunk(const RobotID_t robotID, const ImageChunk& robotImageChunk);
      
      void SendVisionMarker(const u16 topLeft_x, const u16 topLeft_y,
                            const u16 topRight_x, const u16 topRight_y,
                            const u16 bottomRight_x, const u16 bottomRight_y,
                            const u16 bottomLeft_x, const u16 bottomLeft_y,
                            bool verified);
      
      void SendTrackerQuad(const u16 topLeft_x, const u16 topLeft_y,
                           const u16 topRight_x, const u16 topRight_y,
                           const u16 bottomRight_x, const u16 bottomRight_y,
                           const u16 bottomLeft_x, const u16 bottomLeft_y);
      
      void SendRobotState(const RobotState &msg,
                          const s32 numAnimBytesFree,
                          const u8 videoFramefateHz, const u8 animTag);
      
    protected:
      
      // Protected default constructor for singleton.
      VizManager();
      
      void SendMessage(const VizInterface::MessageViz& message);

      static VizManager* _singletonInstance;
      
      bool               _isInitialized;
      UdpClient          _vizClient;
      

      /*
      // Image sending
      std::map<RobotID_t, u8> _imgID;
      */

      bool               _sendImages;
      
      // Stores the maximum ID permitted for a given VizObject type
      u32 _VizObjectMaxID[(int)VizObjectType::NUM_VIZ_OBJECT_TYPES];
      
      // TODO: Won't need this offest once Polygon is implmeneted correctly (not drawing with path)
      const u32 _polyIDOffset = 2200;
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
    void VizManager::DrawQuad(const VizQuadType quadType, const u32 quadID, const Quadrilateral<2,T>& quad,
      const T zHeight_mm, const ColorRGBA& color)
    {
      using namespace Quad;
      VizInterface::Quad v;
      v.quadType = quadType;
      v.quadID = quadID;
      
      const f32 zHeight_m = (float)MM_TO_M(static_cast<float>(zHeight_mm));
      
      v.xUpperLeft  = (float)MM_TO_M(static_cast<float>(quad[TopLeft].x()));
      v.yUpperLeft  = (float)MM_TO_M(static_cast<float>(quad[TopLeft].y()));
      v.zUpperLeft  = zHeight_m;
      
      v.xLowerLeft  = (float)MM_TO_M(static_cast<float>(quad[BottomLeft].x()));
      v.yLowerLeft  = (float)MM_TO_M(static_cast<float>(quad[BottomLeft].y()));
      v.zLowerLeft  = zHeight_m;
      
      v.xUpperRight = (float)MM_TO_M(static_cast<float>(quad[TopRight].x()));
      v.yUpperRight = (float)MM_TO_M(static_cast<float>(quad[TopRight].y()));
      v.zUpperRight = zHeight_m;
      
      v.xLowerRight = (float)MM_TO_M(static_cast<float>(quad[BottomRight].x()));
      v.yLowerRight = (float)MM_TO_M(static_cast<float>(quad[BottomRight].y()));
      v.zLowerRight = zHeight_m;
      v.color = (uint32_t)color;
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }

    template<typename T>
    void VizManager::DrawQuad(const VizQuadType quadType, const u32 quadID, const Quadrilateral<3,T>& quad,
      const ColorRGBA& color)
    {
      using namespace Quad;
      VizInterface::Quad v;
      v.quadType = quadType;
      v.quadID = quadID;
      
      v.xUpperLeft  = (float)MM_TO_M(static_cast<float>(quad[TopLeft].x()));
      v.yUpperLeft  = (float)MM_TO_M(static_cast<float>(quad[TopLeft].y()));
      v.zUpperLeft  = (float)MM_TO_M(static_cast<float>(quad[TopLeft].z()));
      
      v.xLowerLeft  = (float)MM_TO_M(static_cast<float>(quad[BottomLeft].x()));
      v.yLowerLeft  = (float)MM_TO_M(static_cast<float>(quad[BottomLeft].y()));
      v.zLowerLeft  = (float)MM_TO_M(static_cast<float>(quad[BottomLeft].z()));
      
      v.xUpperRight = (float)MM_TO_M(static_cast<float>(quad[TopRight].x()));
      v.yUpperRight = (float)MM_TO_M(static_cast<float>(quad[TopRight].y()));
      v.zUpperRight = (float)MM_TO_M(static_cast<float>(quad[TopRight].z()));
      
      v.xLowerRight = (float)MM_TO_M(static_cast<float>(quad[BottomRight].x()));
      v.yLowerRight = (float)MM_TO_M(static_cast<float>(quad[BottomRight].y()));
      v.zLowerRight = (float)MM_TO_M(static_cast<float>(quad[BottomRight].z()));
      v.color = (uint32_t)color;
      SendMessage(VizInterface::MessageViz(std::move(v)));
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
      u32 pathId = __polyID + _polyIDOffset;

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
      DrawQuad(VizQuadType::VIZ_QUAD_GENERIC_2D, quadID, quad, zHeight_mm, color);
    }
    
    template<typename T>
    void VizManager::DrawGenericQuad(const u32 quadID,
                                     const Quadrilateral<3,T>& quad,
                                     const ColorRGBA& color)
    {
      DrawQuad(VizQuadType::VIZ_QUAD_GENERIC_3D, quadID, quad, color);
    }
    
    template<typename T>
    void VizManager::DrawCameraQuad(const Quadrilateral<2,T>& quad,
                                    const ColorRGBA& color)
    {
      using namespace Quad;
      VizInterface::CameraQuad v;
      
      v.xUpperLeft  = static_cast<float>(quad[TopLeft].x());
      v.yUpperLeft  = static_cast<float>(quad[TopLeft].y());
      
      v.xLowerLeft  = static_cast<float>(quad[BottomLeft].x());
      v.yLowerLeft  = static_cast<float>(quad[BottomLeft].y());
      
      v.xUpperRight = static_cast<float>(quad[TopRight].x());
      v.yUpperRight = static_cast<float>(quad[TopRight].y());
      
      v.xLowerRight = static_cast<float>(quad[BottomRight].x());
      v.yLowerRight = static_cast<float>(quad[BottomRight].y());
      v.color = (uint32_t)color;
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }

    
    template<typename T>
    void VizManager::DrawMatMarker(const u32 quadID,
                                   const Quadrilateral<3,T>& quad,
                                   const ColorRGBA& color)
    {
      DrawQuad(VizQuadType::VIZ_QUAD_MAT_MARKER, quadID, quad, color);
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
      DrawQuad(VizQuadType::VIZ_QUAD_ROBOT_BOUNDING_BOX, quadID, quad, color);
    }

    template<typename T>
    void VizManager::DrawPoseMarker(const u32 quadID,
                                    const Quadrilateral<2,T>& quad,
                                    const ColorRGBA& color)
    {
      DrawQuad(VizQuadType::VIZ_QUAD_POSE_MARKER, quadID, quad, 0.5f, color);
    }
    
    template <typename T>
    void VizManager::DrawXYCircle(u32 polyID,
                                const ColorRGBA& color,
                                const Point<2, T>& center,
                                const T radius,
                                u32 numSegments)
    {
      // Note we create the polygon clockwise intentionally
      T anglePerSegment = static_cast<T>(-2) * static_cast<T>(PI) / static_cast<T>(numSegments);
      
      // Use the tangential and radial factors to draw the segments without recalculating every time.
      // Algorithm found here: http://slabode.exofire.net/circle_draw.shtml
      T tangentialFactor = std::tan(anglePerSegment);
      T radialFactor = std::cos(anglePerSegment);
      
      // Start at angle 0
      T newX = radius;
      T newY = 0;
      
      Polygon<2, T> newCircle;
      for (u32 i=0; i<numSegments; ++i)
      {
        newCircle.push_back(Point<2,T>(newX + center.x(), newY + center.y()));
        
        T tx = -newY;
        T ty = newX;
        
        newX += tx * tangentialFactor;
        newY += ty * tangentialFactor;
        
        newX *= radialFactor;
        newY *= radialFactor;
      }
      DrawPoly(polyID, newCircle, color);
    }
  } // namespace Cozmo
} // namespace Anki


#endif // VIZ_MANAGER_H
