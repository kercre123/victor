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

#include "anki/cozmo/basestation/viz/vizManager.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/vision/basestation/imageIO.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "clad/vizInterface/messageViz.h"
#include <fstream>
#if ANKICORETECH_USE_OPENCV
#include <opencv2/highgui/highgui.hpp>
#endif


namespace Anki {
  namespace Cozmo {
    
    // Base IDs for each VizObject type
    const u32 VizObjectBaseID[(int)VizObjectType::NUM_VIZ_OBJECT_TYPES+1] = {
      0,    // VIZ_OJECT_ROBOT
      1000, // VIZ_OBJECT_CUBOID
      2000, // VIZ_OBJECT_RAMP
      3000, // VIZ_OBJECT_CHARGER
      4000, // VIZ_OJECT_PREDOCKPOSE
      5000, // VIZ_OBJECT_HUMAN_HEAD
      6000, // VIZ_OBJECT_CAMERA_FACE
      u32_MAX - 100 // Last valid object ID allowed
    };
    
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
      for (u32 i=0; i<(int)VizObjectType::NUM_VIZ_OBJECT_TYPES; ++i) {
        _VizObjectMaxID[i] = VizObjectBaseID[i+1] - VizObjectBaseID[i];
      }
    }

    void VizManager::SendMessage(const VizInterface::MessageViz& message)
    {
      if (!_isInitialized)
        return;

      const size_t MAX_MESSAGE_SIZE{2848};
      uint8_t buffer[MAX_MESSAGE_SIZE]{0};

      const size_t numWritten = (uint32_t)message.Pack(buffer, MAX_MESSAGE_SIZE);
      if (_vizClient.Send(buffer, (int)numWritten) <= 0) {
        PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %s of size %zd failed\n", VizInterface::MessageVizTagToString(message.GetTag()), numWritten);
      }
    }

    
    void VizManager::ShowObjects(bool show)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::ShowObjects(show)));
    }
    
    
    // ===== Robot drawing function =======
    
    void VizManager::DrawRobot(const u32 robotID, const Pose3d& pose, const f32 headAngle, const f32 liftAngle) {
      SendMessage(VizInterface::MessageViz(
        VizInterface::SetRobot(robotID,
          (float)MM_TO_M(pose.GetTranslation().x()),
          (float)MM_TO_M(pose.GetTranslation().y()),
          (float)MM_TO_M(pose.GetTranslation().z()),
          pose.GetRotationAngle().ToFloat(),
          pose.GetRotationAxis().x(), pose.GetRotationAxis().y(), pose.GetRotationAxis().z(),
          headAngle, liftAngle
        )
      );
    }
    
    
    // ===== Convenience object draw functions for specific object types ====
    
    VizManager::Handle_t VizManager::DrawRobot(const u32 robotID, const Pose3d& pose, const ColorRGBA& color)
    {
      if(robotID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_ROBOT]) {
        PRINT_NAMED_ERROR("VizManager.DrawRobot.IDtooLarge", "Specified robot ID=%d larger than maxID=%d",
          robotID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_ROBOT]);
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_ROBOT] + robotID;
      Anki::Point3f dims; // junk
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_ROBOT, dims, pose, color);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawCuboid(const u32 blockID,
                                                const Point3f &size,
                                                const Pose3d &pose,
                                                const ColorRGBA& color)
    {
      if(blockID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CUBOID]) {
        PRINT_NAMED_ERROR("VizManager.DrawCuboid.IDtooLarge", "Specified block ID=%d larger than maxID=%d",
          blockID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CUBOID]);
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_CUBOID] + blockID;
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_CUBOID, size, pose, color);
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawPreDockPose(const u32 preDockPoseID, const Pose3d& pose, const ColorRGBA& color)
    {
      if(preDockPoseID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE]) {
        PRINT_NAMED_ERROR("VizManager.DrawPreDockPose.IDtooLarge", "Specified robot ID=%d larger than maxID=%d",
          preDockPoseID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE]);
        return INVALID_HANDLE;
      }
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE] + preDockPoseID;
      Anki::Point3f dims; // junk
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_PREDOCKPOSE, dims, pose, color);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawRamp(const u32 rampID, const f32 platformLength, const f32 slopeLength,
      const f32 width, const f32 height, const Pose3d& pose, const ColorRGBA& color)
    {
      if (rampID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_RAMP]) {
        PRINT_NAMED_ERROR("VizManager.DrawRamp.IDtooLarge", "Specified ramp ID=%d larger than maxID=%d",
          rampID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_RAMP]);
        return INVALID_HANDLE;
      }
      
      // Ramps use one extra parameter which is the ratio of slopeLength to
      // platformLength, which is stored as the x size.  So slopeLength
      // can easily be computed from x size internally (in whatever dimensions
      // the visuzalization uses).
      f32 params[4] = {slopeLength/platformLength, 0, 0, 0};
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_RAMP] + rampID;
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_RAMP, {{platformLength, width, height}}, pose, color, params);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawCharger(const u32 chargerID, const f32 platformLength, const f32 slopeLength,
      const f32 width, const f32 height, const Pose3d& pose, const ColorRGBA& color)
    {
      if (chargerID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CHARGER]) {
        PRINT_NAMED_ERROR("VizManager.DrawCharger.IDtooLarge", "Specified charger ID=%d larger than maxID=%d",
          chargerID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CHARGER]);
        return INVALID_HANDLE;
      }
      
      // Ramps use one extra parameter which is the ratio of slopeLength to
      // platformLength, which is stored as the x size.  So slopeLength
      // can easily be computed from x size internally (in whatever dimensions
      // the visuzalization uses).
      f32 params[4] = {slopeLength/platformLength, 0, 0, 0};
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_CHARGER] + chargerID;
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_CHARGER,
                 {{platformLength, width, height}}, pose, color, params);
      
      return vizID;
    }

    VizManager::Handle_t VizManager::DrawHumanHead(const u32 headID, const Point3f& size, const Pose3d& pose, const ColorRGBA& color)
    {
      if (headID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD]) {
        PRINT_NAMED_ERROR("VizManager.DrawHumanHead.IDtooLarge", "Specified head ID=%d larger than maxID=%d",
          headID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD]);
        return INVALID_HANDLE;

      }
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD] + headID;
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_HUMAN_HEAD, size, pose, color);
      return vizID;
    }
    
    void VizManager::DrawCameraOval(const Point2f &center,
                                    float xRadius, float yRadius,
                                    const Anki::ColorRGBA &color)
    {
      SendMessage(VizInterface::MessageViz(
        VizInterface::CameraOval((uint32_t)color, center.x(), center.y(), xRadius, yRadius)));
    }
    
    void VizManager::DrawCameraLine(const Point2f& start, const Point2f& end, const ColorRGBA& color)
    {
      SendMessage(VizInterface::MessageViz(
        VizInterface::CameraLine((uint32_t)color, start.x(), start.y(), end.x(), end.y())));
    }
    
    void VizManager::DrawCameraText(const Point2f& position, const std::string& text, const ColorRGBA& color)
    {
      SendMessage(VizInterface::MessageViz(
        VizInterface::CameraText((uint32_t)color, std::round(position.x()), std::round(position.y()), {text})));
    }
    
    void VizManager::DrawCameraFace(const Vision::TrackedFace& face, const ColorRGBA& color) {
      // Draw eyes
      DrawCameraOval(face.GetLeftEyeCenter(), 1, 1, color);
      DrawCameraOval(face.GetRightEyeCenter(), 1, 1, color);
      
      // Draw features
      for(s32 iFeature=0; iFeature<(s32)Vision::TrackedFace::NumFeatures; ++iFeature)
      {
        Vision::TrackedFace::FeatureName featureName = (Vision::TrackedFace::FeatureName)iFeature;
        const Vision::TrackedFace::Feature& feature = face.GetFeature(featureName);
        
        for(size_t crntPoint=0, nextPoint=1; nextPoint < feature.size(); ++crntPoint, ++nextPoint) {
          DrawCameraLine(feature[crntPoint], feature[nextPoint], color);
        }
      }
      
      // Draw name
      std::string name;
      if(face.GetID() < 0) {
        name = "Unknown";
      } else if(face.GetName().empty()) {
        name = "Face" + std::to_string(face.GetID());
      } else {
        name = face.GetName();
      }
      DrawCameraText(Point2f(face.GetRect().GetX(), face.GetRect().GetYmax()), name, color);
      
      // Draw bounding rectangle (?)
      Quad2f quad;
      face.GetRect().GetQuad(quad);
      DrawCameraQuad(quad, color);
    }
    
    void VizManager::EraseRobot(const u32 robotID)
    {
      CORETECH_ASSERT(robotID < _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_ROBOT]);
      EraseVizObject(VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_ROBOT] + robotID);
    }
    
    void VizManager::EraseCuboid(const u32 blockID)
    {
      CORETECH_ASSERT(blockID < _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CUBOID]);
      EraseVizObject(_VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CUBOID] + blockID);
    }

    void VizManager::EraseAllCuboids()
    {
      EraseVizObjectType(VizObjectType::VIZ_OBJECT_CUBOID);
    }
    
    void VizManager::ErasePreDockPose(const u32 preDockPoseID)
    {
      CORETECH_ASSERT(preDockPoseID < _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE]);
      EraseVizObject(VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE] + preDockPoseID);
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
  
    
    void VizManager::ErasePoly(u32 __polyID)
    {
      u32 pathId = __polyID + _polyIDOffset;
      // TODO: For now polys are drawn using the path drawing logic, but when it gets implmeneted properly this should be updated
      ErasePath(pathId);
    }
    
    
    // ================== Object drawing methods ====================
    
    void VizManager::DrawObject(const u32 objectID,
      VizObjectType objectTypeID,
      const Anki::Point3f &size_mm,
      const Anki::Pose3d &pose,
      const ColorRGBA& color,
      const f32* params)
    {
      VizInterface::Object v;
      v.objectID = objectID;
      v.objectTypeID = objectTypeID;
      
      v.x_size_m = (float)MM_TO_M(size_mm.x());
      v.y_size_m = (float)MM_TO_M(size_mm.y());
      v.z_size_m = (float)MM_TO_M(size_mm.z());
      
      v.x_trans_m = (float)MM_TO_M(pose.GetTranslation().x());
      v.y_trans_m = (float)MM_TO_M(pose.GetTranslation().y());
      v.z_trans_m = (float)MM_TO_M(pose.GetTranslation().z());
      
      
      // TODO: rotation...
      v.rot_deg = (float)RAD_TO_DEG( pose.GetRotationAngle().ToFloat() );
      v.rot_axis_x = pose.GetRotationAxis().x();
      v.rot_axis_y = pose.GetRotationAxis().y();
      v.rot_axis_z = pose.GetRotationAxis().z();
      
      v.color = u32(color);
      
      if(params != nullptr) {
        for(s32 i=0; i<4; ++i) {
          v.objParameters[i] = params[i];
        }
      }
      
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }
    
    
    void VizManager::EraseVizObject(const Handle_t objectID)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::EraseObject(objectID, 0, 0)));
    }
    

    void VizManager::EraseAllVizObjects()
    {
      SendMessage(VizInterface::MessageViz(VizInterface::EraseObject((uint32_t)VizConstants::ALL_OBJECT_IDs, 0, 0)));
    }
    
    void VizManager::EraseVizObjectType(const VizObjectType type)
    {
      SendMessage(VizInterface::MessageViz(
        VizInterface::EraseObject((uint32_t)VizConstants::OBJECT_ID_RANGE,
          VizObjectBaseID[(int)type], VizObjectBaseID[(int)type+1]-1)));
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
      VizInterface::AppendPathSegmentLine v;
      v.pathID = pathID;
      v.x_start_m = MM_TO_M(x_start_mm);
      v.y_start_m = MM_TO_M(y_start_mm);
      v.z_start_m = 0;
      v.x_end_m = MM_TO_M(x_end_mm);
      v.y_end_m = MM_TO_M(y_end_mm);
      v.z_end_m = 0;
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }
    
    void VizManager::AppendPathSegmentArc(const u32 pathID,
      const f32 x_center_mm, const f32 y_center_mm,
      const f32 radius_mm, const f32 startRad, const f32 sweepRad)
    {
      VizInterface::AppendPathSegmentArc v;
      v.pathID = pathID;
      v.x_center_m = MM_TO_M(x_center_mm);
      v.y_center_m = MM_TO_M(y_center_mm);
      v.radius_m = MM_TO_M(radius_mm);
      v. start_rad = startRad;
      v. sweep_rad = sweepRad;
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }
    

    void VizManager::ErasePath(const u32 pathID)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::ErasePath(pathID)));
    }

    void VizManager::EraseAllPaths()
    {
      SendMessage(VizInterface::MessageViz(VizInterface::ErasePath((uint32_t)VizConstants::ALL_PATH_IDs)));
    }
    
    void VizManager::SetPathColor(const u32 pathID, const ColorRGBA& color)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::SetPathColor(pathID, (uint32_t)color)));
    }
    
    
    // =============== Quad methods ==================
    
    void VizManager::EraseQuad(const u32 quadType, const u32 quadID)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::EraseQuad(quadType, quadID)));
    }
    
    void VizManager::EraseAllQuadsWithType(const u32 quadType)
    {
      EraseQuad(quadType, (uint32_t)VizConstants::ALL_QUAD_IDs);
    }
    
    void VizManager::EraseAllQuads()
    {
      EraseQuad((uint32_t)VizConstants::ALL_QUAD_TYPEs, (uint32_t)VizConstants::ALL_QUAD_IDs);
    }
    
    void VizManager::EraseAllPlannerObstacles(const bool isReplan)
    {
      if(isReplan) {
        EraseAllQuadsWithType((uint32_t)VizQuadType::VIZ_QUAD_PLANNER_OBSTACLE_REPLAN);
      } else {
        EraseAllQuadsWithType((uint32_t)VizQuadType::VIZ_QUAD_PLANNER_OBSTACLE);
      }
    }
    
    void VizManager::EraseAllMatMarkers()
    {
      EraseAllQuadsWithType((uint32_t)VizQuadType::VIZ_QUAD_MAT_MARKER);
    }
    
    // =============== Circle methods ==================
    
    void VizManager::EraseCircle(u32 polyID)
    {
      ErasePoly(polyID);
    }

    
    // =============== Text methods ==================

    void VizManager::SetText(const TextLabelType& labelType, const ColorRGBA& color, const char* format, ...)
    {
      char buffer[2048]{0};
      va_list argptr;
      va_start(argptr, format);
      vsnprintf(buffer, 2048, format, argptr);
      va_end(argptr);
      SendMessage(VizInterface::MessageViz(VizInterface::SetLabel(labelType, (uint32_t)color, {std::string(buffer)})));
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
      SendMessage(VizInterface::MessageViz(VizInterface::DockingErrorSignal(x_dist, y_dist, angle)));
    }
    

    void VizManager::SendRobotState(const RobotState &msg,
                                    const s32 &numAnimBytesFree,
                                    const u8 &videoFramefateHz)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::RobotStateMessage(msg)));
    } // SendRobotState()
    
    /*
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
    */

    
    void VizManager::SendImageChunk(const RobotID_t robotID, const ImageChunk& robotImageChunk)
    {
      if(!_sendImages) {
        return;
      }
      SendMessage(VizInterface::MessageViz(ImageChunk(robotImageChunk)));
    }
    
/*
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

      ImageChunk v;
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
        SendMessage(VizInterface::MessageViz(std::move(v)));


        ++v.chunkId;
      }
*/
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
 *//*

    } // SendImage()
*/

    void VizManager::SendVisionMarker(const u16 topLeft_x, const u16 topLeft_y,
                                      const u16 topRight_x, const u16 topRight_y,
                                      const u16 bottomRight_x, const u16 bottomRight_y,
                                      const u16 bottomLeft_x, const u16 bottomLeft_y,
                                      bool verified)
    {
      VizInterface::VisionMarker v;
      v.topLeft_x = topLeft_x;
      v.topLeft_y = topLeft_y;
      v.topRight_x = topRight_x;
      v.topRight_y = topRight_y;
      v.bottomRight_x = bottomRight_x;
      v.bottomRight_y = bottomRight_y;
      v.bottomLeft_x = bottomLeft_x;
      v.bottomLeft_y = bottomLeft_y;
      v.verified = static_cast<u8>(verified);
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }

    void VizManager::SendTrackerQuad(const u16 topLeft_x, const u16 topLeft_y,
                                     const u16 topRight_x, const u16 topRight_y,
                                     const u16 bottomRight_x, const u16 bottomRight_y,
                                     const u16 bottomLeft_x, const u16 bottomLeft_y)
    {
      VizInterface::TrackerQuad v;
      v.topLeft_x = topLeft_x;
      v.topLeft_y = topLeft_y;
      v.topRight_x = topRight_x;
      v.topRight_y = topRight_y;
      v.bottomRight_x = bottomRight_x;
      v.bottomRight_y = bottomRight_y;
      v.bottomLeft_x = bottomLeft_x;
      v.bottomLeft_y = bottomLeft_y;
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }
    
    
  } // namespace Cozmo
} // namespace Anki
