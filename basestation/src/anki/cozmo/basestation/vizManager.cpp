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

#include <fstream>

#if ANKICORETECH_USE_OPENCV
#include <opencv2/highgui/highgui.hpp>
#endif

#include "anki/cozmo/basestation/viz/vizManager.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/vision/basestation/imageIO.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"


namespace Anki {
  namespace Cozmo {
    
    VizManager* VizManager::_singletonInstance = nullptr;
    
    const VizManager::Handle_t VizManager::INVALID_HANDLE = u32_MAX;
    
    void VizManager::removeInstance()
    {
      // check if the instance has been created yet
      if(nullptr != _singletonInstance) {
        delete _singletonInstance;
        _singletonInstance = nullptr;
      }
    }
    
    Result VizManager::Connect(const char *udp_host_address, const unsigned short port)
    {
      if(_isInitialized) {
        Disconnect();
      }
      
      if (!_vizClient.Connect(udp_host_address, port)) {
        PRINT_NAMED_INFO("VizManager.Connect", "Failed to init VizManager client (%s:%d)", udp_host_address, port);
        //_isInitialized = false;
      }
     
      PRINT_STREAM_INFO("VizManager.Connect", "VizManager connected.");
      _isInitialized = true;
      
      return _isInitialized ? RESULT_OK : RESULT_FAIL;
    }
    
    Result VizManager::Disconnect()
    {
      if(_isInitialized) {
        if (_vizClient.Disconnect()) {
          _isInitialized = false;
          PRINT_NAMED_INFO("VizManager.Connect", "VizManager disconnected.");
          return RESULT_OK;
        }
        return RESULT_FAIL;
      } else {
        return RESULT_OK;
      }
    }
    
    VizManager::VizManager()
    : _isInitialized(false)
    , _sendImages(false)
    {
      // Compute the max IDs permitted by VizObject type
      for (u32 i=0; i<NUM_VIZ_OBJECT_TYPES; ++i) {
        _VizObjectMaxID[i] = VizObjectBaseID[i+1] - VizObjectBaseID[i];
      }
    }

    void VizManager::SendMessage(u8 vizMsgID, void* msg)
    {
      if (!_isInitialized)
        return;
      
      //printf("Sending viz msg %d with %d bytes\n", vizMsgID, msgSize + 1);
      
      // TODO: Does this work for poorly packed structs?  Just use Andrew's message class creator?
      u32 msgSize = Anki::Cozmo::VizMsgLookupTable_[vizMsgID].size;
      
      _sendBuf[0] = vizMsgID;
      memcpy(_sendBuf + 1, msg, msgSize);
      if (_vizClient.Send(_sendBuf, msgSize+1) <= 0) {
        PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %d of size %d failed\n", vizMsgID, msgSize+1);
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
      
      v.x_trans_m = MM_TO_M(pose.GetTranslation().x());
      v.y_trans_m = MM_TO_M(pose.GetTranslation().y());
      v.z_trans_m = MM_TO_M(pose.GetTranslation().z());
      
      v.rot_rad = pose.GetRotationAngle().ToFloat();
      v.rot_axis_x = pose.GetRotationAxis().x();
      v.rot_axis_y = pose.GetRotationAxis().y();
      v.rot_axis_z = pose.GetRotationAxis().z();

      v.head_angle = headAngle;
      v.lift_angle = liftAngle;
      
      SendMessage(GET_MESSAGE_ID(VizSetRobot), &v);
    }
    
    
    // ===== Convenience object draw functions for specific object types ====
    
    VizManager::Handle_t VizManager::DrawRobot(const u32 robotID,
                                               const Pose3d &pose,
                                               const ColorRGBA& color)
    {
      if(robotID >= _VizObjectMaxID[VIZ_OBJECT_ROBOT]) {
        PRINT_NAMED_ERROR("VizManager.DrawRobot.IDtooLarge",
                          "Specified robot ID=%d larger than maxID=%d\n",
                          robotID, _VizObjectMaxID[VIZ_OBJECT_ROBOT]);
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_ROBOT] + robotID;
      Anki::Point3f dims; // junk
      DrawObject(vizID,
                 VIZ_OBJECT_ROBOT,
                 dims,
                 pose,
                 color);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawCuboid(const u32 blockID,
                                                const Point3f &size,
                                                const Pose3d &pose,
                                                const ColorRGBA& color)
    {
      if(blockID >= _VizObjectMaxID[VIZ_OBJECT_CUBOID]) {
        PRINT_NAMED_ERROR("VizManager.DrawCuboid.IDtooLarge",
                          "Specified block ID=%d larger than maxID=%d\n",
                          blockID, _VizObjectMaxID[VIZ_OBJECT_CUBOID]);
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_CUBOID] + blockID;
      DrawObject(vizID,
                 VIZ_OBJECT_CUBOID,
                 size,
                 pose,
                 color);
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawPreDockPose(const u32 preDockPoseID,
                                                     const Pose3d &pose,
                                                     const ColorRGBA& color)
    {
      if(preDockPoseID >= _VizObjectMaxID[VIZ_OBJECT_PREDOCKPOSE]) {
        PRINT_NAMED_ERROR("VizManager.DrawPreDockPose.IDtooLarge",
                          "Specified robot ID=%d larger than maxID=%d\n",
                          preDockPoseID, _VizObjectMaxID[VIZ_OBJECT_PREDOCKPOSE]);
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_PREDOCKPOSE] + preDockPoseID;
      Anki::Point3f dims; // junk
      DrawObject(vizID,
                 VIZ_OBJECT_PREDOCKPOSE,
                 dims,
                 pose,
                 color);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawRamp(const u32 rampID,
                                              const f32 platformLength,
                                              const f32 slopeLength,
                                              const f32 width,
                                              const f32 height,
                                              const Pose3d& pose,
                                              const ColorRGBA& color)
    {
      if(rampID >= _VizObjectMaxID[VIZ_OBJECT_RAMP]) {
        PRINT_NAMED_ERROR("VizManager.DrawRamp.IDtooLarge",
                          "Specified ramp ID=%d larger than maxID=%d\n",
                          rampID, _VizObjectMaxID[VIZ_OBJECT_RAMP]);
        return INVALID_HANDLE;
      }
      
      // Ramps use one extra parameter which is the ratio of slopeLength to
      // platformLength, which is stored as the x size.  So slopeLength
      // can easily be computed from x size internally (in whatever dimensions
      // the visuzalization uses).
      f32 params[4] = {slopeLength/platformLength, 0, 0, 0};
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_RAMP] + rampID;
      DrawObject(vizID, VIZ_OBJECT_RAMP,
                 {{platformLength, width, height}}, pose, color, params);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawCharger(const u32 chargerID,
                                                 const f32 platformLength,
                                                 const f32 slopeLength,
                                                 const f32 width,
                                                 const f32 height,
                                                 const Pose3d& pose,
                                                 const ColorRGBA& color)
    {
      if(chargerID >= _VizObjectMaxID[VIZ_OBJECT_CHARGER]) {
        PRINT_NAMED_ERROR("VizManager.DrawCharger.IDtooLarge",
                          "Specified charger ID=%d larger than maxID=%d\n",
                          chargerID, _VizObjectMaxID[VIZ_OBJECT_CHARGER]);
        return INVALID_HANDLE;
      }
      
      // Ramps use one extra parameter which is the ratio of slopeLength to
      // platformLength, which is stored as the x size.  So slopeLength
      // can easily be computed from x size internally (in whatever dimensions
      // the visuzalization uses).
      f32 params[4] = {slopeLength/platformLength, 0, 0, 0};
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_CHARGER] + chargerID;
      DrawObject(vizID, VIZ_OBJECT_CHARGER,
                 {{platformLength, width, height}}, pose, color, params);
      
      return vizID;
    }

    VizManager::Handle_t VizManager::DrawHumanHead(const u32 headID,
                                                   const Point3f& size,
                                                   const Pose3d& pose,
                                                   const ColorRGBA& color)
    {
      if(headID >= _VizObjectMaxID[VIZ_OBJECT_HUMAN_HEAD]) {
        PRINT_NAMED_ERROR("VizManager.DrawHumanHead.IDtooLarge",
                          "Specified head ID=%d larger than maxID=%d\n",
                          headID, _VizObjectMaxID[VIZ_OBJECT_HUMAN_HEAD]);
        return INVALID_HANDLE;

      }
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_HUMAN_HEAD] + headID;
      DrawObject(vizID, VIZ_OBJECT_HUMAN_HEAD, size, pose, color);
      return vizID;
    }
    
    
    void VizManager::DrawCameraLine(const u32 lineID,
                                    const Point2f& start,
                                    const Point2f& end,
                                    const ColorRGBA& color)
    {
      VizCameraLine v;
      v.lineID = lineID;
      v.color  = u32(color);
      v.xStart = start.x();
      v.yStart = start.y();
      v.xEnd   = end.x();
      v.yEnd   = end.y();
      
      SendMessage( GET_MESSAGE_ID(VizCameraLine), &v );
    }
    
    VizManager::Handle_t VizManager::DrawCameraFace(const u32 faceID,
                                                    const Vision::TrackedFace& face,
                                                    const ColorRGBA& color)
    {
      if(faceID >= _VizObjectMaxID[VIZ_OBJECT_CAMERA_FACE]) {
        PRINT_NAMED_ERROR("VizManager.DrawCameraFace.IDtooLarge",
                          "Specified face ID=%d larger than maxID=%d\n",
                          faceID, _VizObjectMaxID[VIZ_OBJECT_CAMERA_FACE]);
        return INVALID_HANDLE;
      }
      
      Vision::TrackedFace::LandmarkPolygons landmarks = face.GetLandmarkPolygons();
      
      if(landmarks.empty()) {
        PRINT_NAMED_ERROR("VizManager.DrawCameraFace.EmptyLandmarks", "");
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[VIZ_OBJECT_CAMERA_FACE] + faceID;
      
      for(auto & poly : landmarks)
      {
        if(poly.empty()) {
          PRINT_NAMED_WARNING("VizManager.DrawCameraFace.EmptyPoly",
                              "Skipping empty polygon in landmarks.");
        }
        for(size_t crntPoint=0, nextPoint=1; nextPoint < poly.size(); ++crntPoint, ++nextPoint) {
          DrawCameraLine(vizID, poly[crntPoint], poly[nextPoint], color);
        }
      }
      
      return vizID;
    }
    
    void VizManager::EraseRobot(const u32 robotID)
    {
      CORETECH_ASSERT(robotID < _VizObjectMaxID[VIZ_OBJECT_ROBOT]);
      EraseVizObject(VizObjectBaseID[VIZ_OBJECT_ROBOT] + robotID);
    }
    
    void VizManager::EraseCuboid(const u32 blockID)
    {
      CORETECH_ASSERT(blockID < _VizObjectMaxID[VIZ_OBJECT_CUBOID]);
      EraseVizObject(_VizObjectMaxID[VIZ_OBJECT_CUBOID] + blockID);
    }

    void VizManager::EraseAllCuboids()
    {
      EraseVizObjectType(VIZ_OBJECT_CUBOID);
    }
    
    void VizManager::ErasePreDockPose(const u32 preDockPoseID)
    {
      CORETECH_ASSERT(preDockPoseID < _VizObjectMaxID[VIZ_OBJECT_PREDOCKPOSE]);
      EraseVizObject(VizObjectBaseID[VIZ_OBJECT_PREDOCKPOSE] + preDockPoseID);
    }


    void VizManager::DrawPoly(const u32 polyID,
                              const FastPolygon& poly,
                              const ColorRGBA& color)
    {
      // draw bounding circles, then draw the actual polygon
      Planning::Path innerCircle;

      // PRINT_NAMED_INFO("VizManager.DrawPoly", "Drawing poly centered at (%f, %f) with radii %f and %f\n",
      //                  poly.GetCheckCenter().x(),
      //                  poly.GetCheckCenter().y(),
      //                  poly.GetInscribedRadius(),
      //                  poly.GetCircumscribedRadius());

      // // don't draw circles for now

      // // hack! don't want to collide with path ids
      // u32 pathId = polyID + 2300;
      // innerCircle.AppendArc(0,
      //                       poly.GetCheckCenter().x(), poly.GetCheckCenter().y(),
      //                       poly.GetInscribedRadius(),
      //                       0.0f, 2*M_PI,
      //                       1.0, 1.0, 1.0);
      // DrawPath(pathId, innerCircle, color);

      // Planning::Path outerCircle;

      // // hack! don't want to collide with path ids
      // pathId = polyID + 2400;
      // outerCircle.AppendArc(0,
      //                       poly.GetCheckCenter().x(), poly.GetCheckCenter().y(),
      //                       poly.GetCircumscribedRadius(),
      //                       0.0f, 2*M_PI,
      //                       1.0, 1.0, 1.0);
      // DrawPath(pathId, outerCircle, color);


      DrawPoly(polyID, poly.GetSimplePolygon(), color);
    }
  
    
    
    
    // ================== Object drawing methods ====================
    
    void VizManager::DrawObject(const u32 objectID,
                                const u32 objectTypeID,
                                const Anki::Point3f &size_mm,
                                const Anki::Pose3d &pose,
                                const ColorRGBA& color,
                                const f32* params)
    {
      VizObject v;
      v.objectID = objectID;
      v.objectTypeID = objectTypeID;
      
      v.x_size_m = MM_TO_M(size_mm.x());
      v.y_size_m = MM_TO_M(size_mm.y());
      v.z_size_m = MM_TO_M(size_mm.z());
      
      v.x_trans_m = MM_TO_M(pose.GetTranslation().x());
      v.y_trans_m = MM_TO_M(pose.GetTranslation().y());
      v.z_trans_m = MM_TO_M(pose.GetTranslation().z());
      
      
      // TODO: rotation...
      v.rot_deg = RAD_TO_DEG( pose.GetRotationAngle().ToFloat() );
      v.rot_axis_x = pose.GetRotationAxis().x();
      v.rot_axis_y = pose.GetRotationAxis().y();
      v.rot_axis_z = pose.GetRotationAxis().z();
      
      v.color = u32(color);
      
      if(params != nullptr) {
        for(s32 i=0; i<4; ++i) {
          v.params[i] = params[i];
        }
      }
      
      SendMessage( GET_MESSAGE_ID(VizObject), &v );
    }
    
    
    void VizManager::EraseVizObject(const Handle_t objectID)
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
    
    void VizManager::EraseVizObjectType(const VizObjectType type)
    {
      VizEraseObject v;
      v.objectID = OBJECT_ID_RANGE;
      v.lower_bound_id = VizObjectBaseID[type];
      v.upper_bound_id = VizObjectBaseID[type+1]-1;
      
      SendMessage( GET_MESSAGE_ID(VizEraseObject), &v );
    }

    void VizManager::DrawPlannerObstacle(const bool isReplan,
                                         const u32 polyID,
                                         const FastPolygon& poly,
                                         const ColorRGBA& color)
    {
      // const u32 polyType = (isReplan ? VIZ_QUAD_PLANNER_OBSTACLE_REPLAN : VIZ_QUAD_PLANNER_OBSTACLE);
      
      DrawPoly(polyID, poly, color);
    }
    
    // ================== Path drawing methods ====================
    
    void VizManager::DrawPath(const u32 pathID,
                              const Planning::Path& p,
                              const ColorRGBA& color)
    {
      ErasePath(pathID);
      // printf("drawing path %u of length %hhu\n", pathID, p.GetNumSegments());
      
      for (int s=0; s < p.GetNumSegments(); ++s) {
        const Planning::PathSegmentDef& seg = p.GetSegmentConstRef(s).GetDef();
        switch(p.GetSegmentConstRef(s).GetType()) {
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
      
      SetPathColor(pathID, color);
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

      // printf("viz: erasing path %u\n", pathID);
      
      SendMessage( GET_MESSAGE_ID(VizErasePath), &v );
    }

    void VizManager::EraseAllPaths()
    {
      VizErasePath v;
      v.pathID = ALL_PATH_IDs;

      printf("viz: erasing all paths\n");
      
      SendMessage( GET_MESSAGE_ID(VizErasePath), &v );
    }
    
    void VizManager::SetPathColor(const u32 pathID, const ColorRGBA& color)
    {
      VizSetPathColor v;
      v.pathID = pathID;
      v.colorID = u32(color);
      
      SendMessage( GET_MESSAGE_ID(VizSetPathColor), &v );
    }
    
    
    // =============== Quad methods ==================
    
    void VizManager::EraseQuad(const u32 quadType, const u32 quadID)
    {
      VizEraseQuad v;
      v.quadType = quadType;
      v.quadID = quadID;
      
      SendMessage( GET_MESSAGE_ID(VizEraseQuad), &v );
    }
    
    void VizManager::EraseAllQuadsWithType(const u32 quadType)
    {
      EraseQuad(quadType, ALL_QUAD_IDs);
    }
    
    void VizManager::EraseAllQuads()
    {
      EraseQuad(ALL_QUAD_TYPEs, ALL_QUAD_IDs);
    }
    
    void VizManager::EraseAllPlannerObstacles(const bool isReplan)
    {
      if(isReplan) {
        EraseAllQuadsWithType(VIZ_QUAD_PLANNER_OBSTACLE_REPLAN);
      } else {
        EraseAllQuadsWithType(VIZ_QUAD_PLANNER_OBSTACLE);
      }
    }
    
    void VizManager::EraseAllMatMarkers()
    {
      EraseAllQuadsWithType(VIZ_QUAD_MAT_MARKER);
    }

    
    // =============== Text methods ==================

    void VizManager::SetText(const TextLabelType& labelType, const ColorRGBA& color, const char* format, ...)
    {
      VizSetLabel v;
      v.labelID = labelType;
      v.colorID = u32(color);
      
      va_list argptr;
      va_start(argptr, format);
      vsnprintf((char*)v.text, sizeof(v.text), format, argptr);
      va_end(argptr);
      
      SendMessage( GET_MESSAGE_ID(VizSetLabel), &v );
    }
    
    
    // ================== Color methods ====================
    /*
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
    */
    
    // ============== Misc. Debug methods =================
    void VizManager::SetDockingError(const f32 x_dist, const f32 y_dist, const f32 angle)
    {
      VizDockingErrorSignal v;
      v.x_dist = x_dist;
      v.y_dist = y_dist;
      v.angle = angle;
      
      SendMessage( GET_MESSAGE_ID(VizDockingErrorSignal), &v );
    }
    

    void VizManager::SendRobotState(const MessageRobotState &msg,
                                    const s32 &numAnimBytesFree,
                                    const u8 &videoFramefateHz)
    {
      VizRobotState m;
      m.pose_frame_id = msg.pose_frame_id;
      m.pose_x = msg.pose_x;
      m.pose_y = msg.pose_y;
      m.pose_z = msg.pose_z;
      m.pose_angle = msg.pose_angle;
      m.pose_pitch_angle = msg.pose_pitch_angle;
      m.lwheel_speed_mmps = msg.lwheel_speed_mmps;
      m.rwheel_speed_mmps = msg.rwheel_speed_mmps;
      m.headAngle = msg.headAngle;
      m.liftHeight = msg.liftHeight;
      m.lastPathID = msg.lastPathID;
      m.proxLeft = msg.proxLeft;
      m.proxForward = msg.proxForward;
      m.proxRight = msg.proxRight;
      m.battVolt10x = msg.battVolt10x;
      m.status = msg.status;
      
      m.numAnimBytesFree = numAnimBytesFree;
      m.videoFramerateHZ = videoFramefateHz;
      
      SendMessage( GET_MESSAGE_ID(VizRobotState), &m);
    } // SendRobotState()
    
    void VizManager::SendGreyImage(const RobotID_t robotID,
                                   const u8* data,
                                   const Vision::CameraResolution res,
                                   const TimeStamp_t timestamp)
    {
      if(!_sendImages) {
        return;
      }
      
      const u32 dataLength = Vision::CameraResInfo[res].width * Vision::CameraResInfo[res].height;
      SendImage(robotID, data, dataLength, res, timestamp, Vision::IE_RAW_GRAY);
    }
    
    
    void VizManager::SendColorImage(const RobotID_t robotID, const u8* data, const Vision::CameraResolution res, const TimeStamp_t timestamp)
    {
      if(!_sendImages) {
        return;
      }
      
      const u32 dataLength = Vision::CameraResInfo[res].width * Vision::CameraResInfo[res].height * 3;
      SendImage(robotID, data, dataLength, res, timestamp, Vision::IE_RAW_RGB);
    }
    
    
    void VizManager::SendImageChunk(const RobotID_t robotID, MessageImageChunk robotImageChunk)
    {
      if(!_sendImages) {
        return;
      }
      
      VizImageChunk v;
      static_assert(robotImageChunk.data.size() == MAX_VIZ_IMAGE_CHUNK_SIZE,
                    "MessageImageChunk and VizImageChunk must have the same chunk size.");
      
      v.chunkSize = robotImageChunk.chunkSize;
      v.chunkId = robotImageChunk.chunkId;
      v.chunkCount = robotImageChunk.imageChunkCount;
      v.imgId = robotImageChunk.imageId;
      v.resolution = robotImageChunk.resolution;
      v.encoding = robotImageChunk.imageEncoding;
      
      std::copy(robotImageChunk.data.begin(), robotImageChunk.data.end(), v.data);
      
      SendMessage( GET_MESSAGE_ID(VizImageChunk), &v );
    }
    
    void VizManager::SendImage(const RobotID_t robotID,
                               const u8* data,
                               const u32 dataLength,
                               const Vision::CameraResolution res,
                               const TimeStamp_t timestamp,
                               const Vision::ImageEncoding_t encoding)
    {
      if(!_sendImages) {
        return;
      }

      VizImageChunk v;
      v.resolution = res;
      v.imgId = ++(_imgID[robotID]);
      v.chunkId = 0;
      f32 chunkCount = ceilf((f32)dataLength / MAX_VIZ_IMAGE_CHUNK_SIZE);
      if(chunkCount > static_cast<f32>(u8_MAX)) {
        PRINT_NAMED_ERROR("VizManager.SendImage", "Too many chunks (>255) required to send image of %d bytes.\n", dataLength);
        return;
      }
      v.chunkCount = static_cast<u8>(chunkCount);
      v.chunkSize = MAX_VIZ_IMAGE_CHUNK_SIZE;
      v.encoding = encoding;
      
      s32 bytesToSend = dataLength;
      
      while (bytesToSend > 0) {
        if (bytesToSend < MAX_VIZ_IMAGE_CHUNK_SIZE) {
          v.chunkSize = bytesToSend;
          assert(v.chunkId == v.chunkCount-1);
        }
        bytesToSend -= v.chunkSize;
        
        
        memcpy(v.data, &data[v.chunkId * MAX_VIZ_IMAGE_CHUNK_SIZE], v.chunkSize);
        // printf("Sending CAM image %d chunk %d (size: %d), bytesLeftToSend %d of %d, first/lastByte=%d/%d\n",
        //       v.imgId, v.chunkId, v.chunkSize, bytesToSend, dataLength, v.data[0], v.data[v.chunkSize-1]);
        SendMessage( GET_MESSAGE_ID(VizImageChunk), &v );
        
        ++v.chunkId;
      }
/*
      if (_saveImageMode != SAVE_OFF) {
        
        // Make sure image capture folder exists
        if (!DirExists(AnkiUtil::kP_IMG_CAPTURE_DIR)) {
          if (!MakeDir(AnkiUtil::kP_IMG_CAPTURE_DIR)) {
            PRINT_NAMED_WARNING("VizManager.SendGreyImage.CreateDirFailed","\n");
          }
        }
        
        const char *ext = "";
        switch(encoding) {
          case Vision::IE_RAW_GRAY:
            ext = "pgm";
            break;
          case Vision::IE_RAW_RGB:
            ext = "ppm";
            break;
          case Vision::IE_JPEG_COLOR:
          case Vision::IE_JPEG_GRAY:
            ext = "jpg";
            break;
          default:
            ext = "raw";
        }
        // Create image file
        char imgCaptureFilename[64];
        snprintf(imgCaptureFilename, sizeof(imgCaptureFilename), "%s/robot%d_img_%d.%s",
                 AnkiUtil::kP_IMG_CAPTURE_DIR, robotID, timestamp, ext);
        
        switch(encoding)
        {
          case Vision::IE_RAW_RGB:
            Vision::WritePPM(imgCaptureFilename, data, Vision::CameraResInfo[res].width, Vision::CameraResInfo[res].height);
            break;
          case Vision::IE_RAW_GRAY:
            Vision::WritePGM(imgCaptureFilename, data, Vision::CameraResInfo[res].width, Vision::CameraResInfo[res].height);
            break;
          default:
            // Just dump already-encoded data to file:
            FILE * fp = fopen(imgCaptureFilename, "w");
            fwrite(data, dataLength, sizeof(u8), fp);
            fclose(fp);
        }
        
        PRINT_INFO("Saved image to %s\n", imgCaptureFilename);

        // Turn off save mode if we were in one-shot mode
        if (_saveImageMode == SAVE_ONE_SHOT) {
          _saveImageMode = SAVE_OFF;
        }
      }
 */
    } // SendImage()
    
    void VizManager::SendVisionMarker(const u16 topLeft_x, const u16 topLeft_y,
                                      const u16 topRight_x, const u16 topRight_y,
                                      const u16 bottomRight_x, const u16 bottomRight_y,
                                      const u16 bottomLeft_x, const u16 bottomLeft_y,
                                      bool verified)
    {
      VizVisionMarker v;
      v.topLeft_x = topLeft_x;
      v.topLeft_y = topLeft_y;
      v.topRight_x = topRight_x;
      v.topRight_y = topRight_y;
      v.bottomRight_x = bottomRight_x;
      v.bottomRight_y = bottomRight_y;
      v.bottomLeft_x = bottomLeft_x;
      v.bottomLeft_y = bottomLeft_y;
      v.verified = static_cast<u8>(verified);
      
      SendMessage(GET_MESSAGE_ID(VizVisionMarker), &v);
    }

    void VizManager::SendTrackerQuad(const u16 topLeft_x, const u16 topLeft_y,
                                     const u16 topRight_x, const u16 topRight_y,
                                     const u16 bottomRight_x, const u16 bottomRight_y,
                                     const u16 bottomLeft_x, const u16 bottomLeft_y)
    {
      VizTrackerQuad v;
      v.topLeft_x = topLeft_x;
      v.topLeft_y = topLeft_y;
      v.topRight_x = topRight_x;
      v.topRight_y = topRight_y;
      v.bottomRight_x = bottomRight_x;
      v.bottomRight_y = bottomRight_y;
      v.bottomLeft_x = bottomLeft_x;
      v.bottomLeft_y = bottomLeft_y;
      
      SendMessage(GET_MESSAGE_ID(VizTrackerQuad), &v);
    }
    
    
  } // namespace Cozmo
} // namespace Anki
