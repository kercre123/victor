/**
 * File: vizManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/5/2014
 *
 * Description: Implements the VizManager object. See
 *              corresponding header for more detail.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/viz/vizObjectBaseId.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "anki/cozmo/game/comms/gameMessagePort.h"
#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/vision/basestation/imageIO.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "clad/vizInterface/messageViz.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <fstream>

#if ANKICORETECH_USE_OPENCV
#include <opencv2/highgui.hpp>
#endif

namespace Anki {
  namespace Cozmo {
    
    CONSOLE_VAR(bool, kSendAnythingToViz, "VizDebug", true);
    CONSOLE_VAR(bool, kSendBehaviorScoresToViz, "VizDebug", true);
    
    const VizManager::Handle_t VizManager::INVALID_HANDLE = std::numeric_limits<u32>::max();
    
    Result VizManager::Connect(const char *udp_host_address, const unsigned short port, const char* unity_host_address, const unsigned short unity_port)
    {
      if(_isInitialized) {
        Disconnect();
      }

      #if !VIZ_ON_DEVICE
      if (!_vizClient.Connect(udp_host_address, port)) {
        PRINT_NAMED_INFO("VizManager.Connect", "Failed to init VizManager client (%s:%d)", udp_host_address, port);
        //_isInitialized = false;
      }

      if(!_unityVizClient.Connect(unity_host_address, unity_port)) {
        PRINT_NAMED_INFO("VizManager.Connect", "Failed to init VizManager unity client (%s:%d)", unity_host_address, unity_port);
      }
      #endif
     
      PRINT_STREAM_INFO("VizManager.Connect", "VizManager connected.");
      _isInitialized = true;
      
      return _isInitialized ? RESULT_OK : RESULT_FAIL;
    }
    
    Result VizManager::Disconnect()
    {
      if(_isInitialized) {
        bool vizDisconnected = _vizClient.Disconnect();
        bool unityDisconnected = true;
        #if !VIZ_ON_DEVICE
        unityDisconnected = _unityVizClient.Disconnect();
        #endif
        
        if (vizDisconnected || unityDisconnected) {
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
      if (!ANKI_DEV_CHEATS || !_isInitialized || !kSendAnythingToViz)
      {
        return;
      }
      
      ANKI_CPU_PROFILE("VizManager::SendMessage");

      const size_t MAX_MESSAGE_SIZE{(size_t)VizConstants::MaxMessageSize};
      uint8_t buffer[MAX_MESSAGE_SIZE];

      const size_t numWritten = (uint32_t)message.Pack(buffer, MAX_MESSAGE_SIZE);
      
      {
        #if !VIZ_ON_DEVICE
        ANKI_CPU_PROFILE("VizClient.Send");
        if (_vizClient.Send((const char*)buffer, (int)numWritten) <= 0) {
          PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %s of size %zd failed", VizInterface::MessageVizTagToString(message.GetTag()), numWritten);
        }
        #endif
      }
      
      {
        ANKI_CPU_PROFILE("UnityVizClient.Send");
        #if VIZ_ON_DEVICE
        if (_unityVizPort != nullptr) {
          _unityVizPort->PushToGameMessage(buffer, numWritten);
        }
        #else
        if (_unityVizClient.Send((const char*)buffer, (int)numWritten) <= 0) {
          if ( _unityVizClient.IsConnected() ) { // prevents webots from crying when no Unity app is launched
            PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %s of size %zd to Unity failed", VizInterface::MessageVizTagToString(message.GetTag()), numWritten);
          }
        }
        #endif
      }
        
      // Log viz messages from here.
      if(ANKI_DEV_CHEATS && nullptr != DevLoggingSystem::GetInstance())
      {
        DevLoggingSystem::GetInstance()->LogMessage(message);
      }
    
    }

    
    void VizManager::ShowObjects(bool show)
    {
      ANKI_CPU_PROFILE("VizManager::ShowObjects");
      SendMessage(VizInterface::MessageViz(VizInterface::ShowObjects(show)));
    }
    
    
    // ===== Robot drawing function =======
    
    void VizManager::DrawRobot(const u32 robotID, const Pose3d& pose, const f32 headAngle, const f32 liftAngle) {
      ANKI_CPU_PROFILE("VizManager::DrawRobot");
      SendMessage(VizInterface::MessageViz(
        VizInterface::SetRobot(robotID,
          (float)MM_TO_M(pose.GetTranslation().x()),
          (float)MM_TO_M(pose.GetTranslation().y()),
          (float)MM_TO_M(pose.GetTranslation().z()),
          pose.GetRotationAngle().ToFloat(),
          pose.GetRotationAxis().x(), pose.GetRotationAxis().y(), pose.GetRotationAxis().z(),
          headAngle, liftAngle
        )
      ));
    }
    
    
    // ===== Convenience object draw functions for specific object types ====
    
    VizManager::Handle_t VizManager::DrawRobot(const u32 robotID, const Pose3d& pose, const ColorRGBA& color)
    {
      if(robotID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_ROBOT]) {
        PRINT_NAMED_WARNING("VizManager.DrawRobot.IDtooLarge",
                            "Specified robot ID=%d larger than maxID=%d",
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
        PRINT_NAMED_WARNING("VizManager.DrawCuboid.IDtooLarge",
                            "Specified block ID=%d larger than maxID=%d",
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
        PRINT_NAMED_WARNING("VizManager.DrawPreDockPose.IDtooLarge",
                            "Specified PreDockPose ID=%d larger than maxID=%d",
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
        PRINT_NAMED_WARNING("VizManager.DrawRamp.IDtooLarge",
                            "Specified ramp ID=%d larger than maxID=%d",
                            rampID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_RAMP]);
        return INVALID_HANDLE;
      }
      
      // Ramps use one extra parameter which is the ratio of slopeLength to
      // platformLength, which is stored as the x size.  So slopeLength
      // can easily be computed from x size internally (in whatever dimensions
      // the visuzalization uses).
      f32 params[4] = {slopeLength/platformLength, 0, 0, 0};
      
      const u32 vizID = VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_RAMP] + rampID;
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_RAMP, {platformLength, width, height}, pose, color, params);
      
      return vizID;
    }
    
    VizManager::Handle_t VizManager::DrawCharger(const u32 chargerID, const f32 platformLength, const f32 slopeLength,
      const f32 width, const f32 height, const Pose3d& pose, const ColorRGBA& color)
    {
      if (chargerID >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_CHARGER]) {
        PRINT_NAMED_WARNING("VizManager.DrawCharger.IDtooLarge",
                            "Specified charger ID=%d larger than maxID=%d",
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
                 {platformLength, width, height}, pose, color, params);
      
      return vizID;
    }

    VizManager::Handle_t VizManager::DrawHumanHead(const s32 headID, const Point3f& size, const Pose3d& pose, const ColorRGBA& color)
    {
      if (std::abs(headID) >= _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD]) {
        PRINT_NAMED_WARNING("VizManager.DrawHumanHead.IDtooLarge",
                            "Specified head ID=%d larger than maxID=%d",
                            headID, _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD]);
        return INVALID_HANDLE;

      }
      
      const u32 vizID = (headID >= 0 ?
                         VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD] + headID :
                         _VizObjectMaxID[(int)VizObjectType::VIZ_OBJECT_HUMAN_HEAD] + headID);
                         
      DrawObject(vizID, VizObjectType::VIZ_OBJECT_HUMAN_HEAD, size, pose, color);
      return vizID;
    }
    
    void VizManager::DrawCameraOval(const Point2f &center,
                                    float xRadius, float yRadius,
                                    const Anki::ColorRGBA &color)
    {
      ANKI_CPU_PROFILE("VizManager::DrawCameraOval");
      
      SendMessage(VizInterface::MessageViz(
        VizInterface::CameraOval((uint32_t)color, center.x(), center.y(), xRadius, yRadius)));
    }
    
    void VizManager::DrawCameraLine(const Point2f& start, const Point2f& end, const ColorRGBA& color)
    {
      ANKI_CPU_PROFILE("VizManager::DrawCameraLine");
      
      SendMessage(VizInterface::MessageViz(
        VizInterface::CameraLine((uint32_t)color, start.x(), start.y(), end.x(), end.y())));
    }
    
    void VizManager::DrawCameraText(const Point2f& position, const std::string& text, const ColorRGBA& color)
    {
      ANKI_CPU_PROFILE("VizManager::DrawCameraText");
      
      SendMessage(VizInterface::MessageViz(
        VizInterface::CameraText((uint32_t)color, std::round(position.x()), std::round(position.y()), {text})));
    }
    
    void VizManager::DrawCameraFace(const Vision::TrackedFace& face, const ColorRGBA& color) {
      // Draw eyes
      Point2f leftEye, rightEye;
      if(face.GetEyeCenters(leftEye, rightEye)) {
        DrawCameraOval(leftEye,  1, 1, color);
        DrawCameraOval(rightEye, 1, 1, color);
      }
      
      // Draw features
      for(s32 iFeature=0; iFeature<(s32)Vision::TrackedFace::NumFeatures; ++iFeature)
      {
        Vision::TrackedFace::FeatureName featureName = (Vision::TrackedFace::FeatureName)iFeature;
        const Vision::TrackedFace::Feature& feature = face.GetFeature(featureName);
        
        for(size_t crntPoint=0, nextPoint=1; nextPoint < feature.size(); ++crntPoint, ++nextPoint) {
          DrawCameraLine(feature[crntPoint], feature[nextPoint], color);
        }
      }
      
      // Draw name & most likely expression
      std::string name;
      if(face.GetName().empty()) {
        if(face.GetID() > 0) {
          name = "KnownFace[";
        } else {
          name = "UnknownFace[";
        }
        name += std::to_string(face.GetID()) + "]-";
      } else {
        name = face.GetName() + "-";
      }
      name += EnumToString(face.GetMaxExpression());
      
      // Add score debugging info:
      auto debugScoreInfo = face.GetRecognitionDebugInfo();
      for(auto & info : debugScoreInfo) {
        name += "\n*";
        if(info.name.empty()) {
          name += "KnownFace";
        } else {
          name += info.name;
        }
        name += "[" + std::to_string(info.matchedID) + "]=" + std::to_string(info.score);
      }
      
      DrawCameraText(Point2f(face.GetRect().GetX(), face.GetRect().GetYmax()), name, color);
      
      // Draw bounding rectangle (?)
      Quad2f quad;
      face.GetRect().GetQuad(quad);
      DrawCameraQuad(quad, color);
    }
    
    void VizManager::EraseRobot(const u32 robotID)
    {
      const int viztype = (int)VizObjectType::VIZ_OBJECT_ROBOT;
      ASSERT_NAMED(robotID < _VizObjectMaxID[viztype], "VizManager.EraseRobot.InvalidID");
      EraseVizObject(VizObjectBaseID[viztype] + robotID);
    }
    
    void VizManager::EraseCuboid(const u32 blockID)
    {
      const int viztype = (int)VizObjectType::VIZ_OBJECT_CUBOID;
      ASSERT_NAMED(blockID < _VizObjectMaxID[viztype], "VizManager.EraseCuboid.InvalidID");
      EraseVizObject(VizObjectBaseID[viztype] + blockID);
    }

    void VizManager::EraseAllCuboids()
    {
      EraseVizObjectType(VizObjectType::VIZ_OBJECT_CUBOID);
    }
    
    void VizManager::ErasePreDockPose(const u32 preDockPoseID)
    {
      const int viztype = (int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE;
      ASSERT_NAMED(preDockPoseID < _VizObjectMaxID[viztype], "VizManager.ErasePreDockPose.InvalidID");
      EraseVizObject(VizObjectBaseID[viztype] + preDockPoseID);
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
      ANKI_CPU_PROFILE("VizManager::DrawObject");
      
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
      v.rot_deg = pose.GetRotationAngle().getDegrees();
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
      ANKI_CPU_PROFILE("VizManager::EraseVizObject");
      SendMessage(VizInterface::MessageViz(VizInterface::EraseObject(objectID, 0, 0)));
    }
    

    void VizManager::EraseAllVizObjects()
    {
      ANKI_CPU_PROFILE("VizManager::EraseAllVizObjects");
      
      SendMessage(VizInterface::MessageViz(VizInterface::EraseObject((uint32_t)VizConstants::ALL_OBJECT_IDs, 0, 0)));
    }
    
    void VizManager::EraseVizObjectType(const VizObjectType type)
    {
      ANKI_CPU_PROFILE("VizManager::EraseVizObjectType");
      
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
      ANKI_CPU_PROFILE("VizManager::AppendPathSegmentLine");
      
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
      ANKI_CPU_PROFILE("VizManager::AppendPathSegmentArc");
      
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
      ANKI_CPU_PROFILE("VizManager::ErasePath");
      SendMessage(VizInterface::MessageViz(VizInterface::ErasePath(pathID)));
    }

    void VizManager::EraseAllPaths()
    {
      ANKI_CPU_PROFILE("VizManager::EraseAllPaths");
      SendMessage(VizInterface::MessageViz(VizInterface::ErasePath((uint32_t)VizConstants::ALL_PATH_IDs)));
    }
    
    void VizManager::SetPathColor(const u32 pathID, const ColorRGBA& color)
    {
      ANKI_CPU_PROFILE("VizManager::SetPathColor");
      SendMessage(VizInterface::MessageViz(VizInterface::SetPathColor(pathID, (uint32_t)color)));
    }
    
    
    // =============== Quad methods ==================
    
    void VizManager::EraseQuad(const uint32_t quadType, const u32 quadID)
    {
      ANKI_CPU_PROFILE("VizManager::EraseQuad");
      SendMessage(VizInterface::MessageViz(VizInterface::EraseQuad(quadType, quadID)));
    }
    
    void VizManager::EraseAllQuadsWithType(const uint32_t quadType)
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
    
    // ==== Draw functions by identifier =====
    
    void VizManager::EraseSegments(const std::string& identifier)
    {
      ANKI_CPU_PROFILE("VizManager::EraseSegments");
      SendMessage(VizInterface::MessageViz(VizInterface::EraseSegmentPrimitives{identifier}));
    }
    

    void VizManager::DrawQuadVector(const std::string& identifier, const SimpleQuadVector& quads)
    {
      ANKI_CPU_PROFILE("VizManager::DrawQuadVector");
      SendMessage(VizInterface::MessageViz(VizInterface::SimpleQuadVectorMessageBegin{identifier}));
      
      // split quad vector into several messages
      if ( !quads.empty() )
      {
        // calculate some initial sizes
        const size_t kReservedBytes = 2; // for things like array size
        const size_t maxBufferSize = Anki::Util::numeric_cast<size_t>((std::underlying_type<VizConstants>::type)VizConstants::MaxMessageSize);
        const size_t maxBufferForQuads = maxBufferSize - (identifier.length()+1) - kReservedBytes;
        size_t quadsPerMessage = maxBufferForQuads / sizeof(SimpleQuadVector::value_type);
        size_t remainingQuads = quads.size();
        
        ASSERT_NAMED(quadsPerMessage>0, "VizManager.DrawQuadVector.InvalidQuadsPerMessage");
        
        // sadly we can't create one message and send it several times, because MessageViz doesn't support it (it needs
        // to embed the tag) for receiving end processing, and we can't initialize messages with a range of vectors, so
        // I have to create copies :(
        SimpleQuadVector partQuads;
        partQuads.reserve( quadsPerMessage );
        
        // while we have quads to send
        while ( remainingQuads > 0 )
        {
          // how many are we sending in this message?
          quadsPerMessage = Anki::Util::Min(quadsPerMessage, remainingQuads);
          
          // clear the destination vector and insert as many as we are sending, from where we left off
          partQuads.clear();
          partQuads.insert( partQuads.end(), quads.end() - remainingQuads, quads.end() - remainingQuads + quadsPerMessage );
          
          remainingQuads -= quadsPerMessage;
          
          // send message
          ANKI_CPU_PROFILE("VizManager::SimpleQuadVectorMessage");
          SendMessage(VizInterface::MessageViz(VizInterface::SimpleQuadVectorMessage{identifier, partQuads}));
        }
      }
      
      SendMessage(VizInterface::MessageViz(VizInterface::SimpleQuadVectorMessageEnd{identifier}));
    }
    
    void VizManager::EraseQuadVector(const std::string& identifier)
    {
      ANKI_CPU_PROFILE("VizManager::EraseQuadVector");
      
      SendMessage(VizInterface::MessageViz(VizInterface::SimpleQuadVectorMessageBegin{identifier}));
      SendMessage(VizInterface::MessageViz(VizInterface::SimpleQuadVectorMessageEnd{identifier}));
    }
    
    // =============== Circle methods ==================
    
    void VizManager::EraseCircle(u32 polyID)
    {
      ErasePoly(polyID);
    }

    
    // =============== Text methods ==================

    void VizManager::SetText(const TextLabelType& labelType, const ColorRGBA& color, const char* format, ...)
    {
      ANKI_CPU_PROFILE("VizManager::SetText");
      
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
    void VizManager::SetDockingError(const f32 x_dist, const f32 y_dist, const f32 z_dist, const f32 angle)
    {
      ANKI_CPU_PROFILE("VizManager::SetDockingError");
      SendMessage(VizInterface::MessageViz(VizInterface::DockingErrorSignal(x_dist, y_dist, z_dist, angle)));
    }
    
    void VizManager::SendCameraInfo(const u16 exposure_ms, const f32 gain)
    {
      ANKI_CPU_PROFILE("VizManager::SendCameraInfo");
      SendMessage(VizInterface::MessageViz(VizInterface::CameraInfo(gain, exposure_ms)));
    }

    void VizManager::SendRobotState(const RobotState &msg,
                                    const s32 numAnimBytesFree,
                                    const s32 numAnimAudioFramesFree,
                                    const u8 videoFrameRateHz,
                                    const u8 imageProcFrameRateHz,
                                    const u8 enabledAnimTracks,
                                    const u8 animTag)
    {
      ANKI_CPU_PROFILE("VizManager::SendRobotState");
      SendMessage(VizInterface::MessageViz(VizInterface::RobotStateMessage(msg, numAnimBytesFree, numAnimAudioFramesFree, videoFrameRateHz, imageProcFrameRateHz, enabledAnimTracks, animTag)));
    }
    
    void VizManager::SendRobotMood(VizInterface::RobotMood&& robotMood)
    {
      ANKI_CPU_PROFILE("VizManager::SendRobotMood");
      SendMessage(VizInterface::MessageViz(std::move(robotMood)));
    }

    void VizManager::SendRobotBehaviorSelectData(VizInterface::RobotBehaviorSelectData&& robotBehaviorSelectData)
    {
      if (kSendBehaviorScoresToViz)
      {
        ANKI_CPU_PROFILE("VizManager::SendRobotBehaviorSelectData");
        SendMessage(VizInterface::MessageViz(std::move(robotBehaviorSelectData)));
      }
    }

    void VizManager::SendNewBehaviorSelected(VizInterface::NewBehaviorSelected&& newBehaviorSelected)
    {
      ANKI_CPU_PROFILE("VizManager::SendNewBehaviorSelected");
      SendMessage(VizInterface::MessageViz(std::move(newBehaviorSelected)));
    }

    void VizManager::SendStartRobotUpdate()
    {
      ANKI_CPU_PROFILE("VizManager::SendStartRobotUpdate");
      SendMessage(VizInterface::MessageViz(VizInterface::StartRobotUpdate()));
    }

    void VizManager::SendEndRobotUpdate()
    {
      ANKI_CPU_PROFILE("VizManager::SendEndRobotUpdate");
      SendMessage(VizInterface::MessageViz(VizInterface::EndRobotUpdate()));
    }
    
    void VizManager::SendSaveImages(ImageSendMode mode, std::string path)
    {
      ANKI_CPU_PROFILE("VizManager::SendSaveImages");
      SendMessage(VizInterface::MessageViz(VizInterface::SaveImages(mode, path)));
    }

    void VizManager::SendSaveState(bool enabled, std::string path)
    {
      ANKI_CPU_PROFILE("VizManager::SendSaveState");
      SendMessage(VizInterface::MessageViz(VizInterface::SaveState(enabled, path)));
    }
    
    void VizManager::SendObjectConnectionState(u32 activeID, ObjectType type, bool connected)
    {
      ANKI_CPU_PROFILE("VizManager::SendObjectConnectionState");
      SendMessage(VizInterface::MessageViz(VizInterface::ObjectConnectionState(activeID, type, connected)));
    }
    
    void VizManager::SendObjectMovingState(u32 activeID, bool moving)
    {
      ANKI_CPU_PROFILE("VizManager::SendObjectMovingState");
      SendMessage(VizInterface::MessageViz(VizInterface::ObjectMovingState(activeID, moving)));
    }
    
    void VizManager::SendObjectUpAxisState(u32 activeID, UpAxis upAxis)
    {
      ANKI_CPU_PROFILE("VizManager::SendObjectUpAxisState");
      SendMessage(VizInterface::MessageViz(VizInterface::ObjectUpAxisState(activeID, upAxis)));
    }
    
    void VizManager::SendObjectAccelState(u32 activeID, const ActiveAccel& accel)
    {
      ANKI_CPU_PROFILE("VizManager::SendObjectAccelState");
      SendMessage(VizInterface::MessageViz(VizInterface::ObjectAccelState(activeID, accel)));
    }

  
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
      ANKI_CPU_PROFILE("VizManager::SendImageChunk");
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
      if(chunkCount > static_cast<f32>(std::numeric_limits<u8>::max())) {
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

    void VizManager::SendTrackerQuad(const u16 topLeft_x, const u16 topLeft_y,
                                     const u16 topRight_x, const u16 topRight_y,
                                     const u16 bottomRight_x, const u16 bottomRight_y,
                                     const u16 bottomLeft_x, const u16 bottomLeft_y)
    {
      ANKI_CPU_PROFILE("VizManager::SendTrackerQuad");
      
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
    
    void VizManager::SetOrigin(const SetVizOrigin& msg)
    {
      ANKI_CPU_PROFILE("VizManager::SetOrigin");
      
      SetVizOrigin v(msg);
      SendMessage(VizInterface::MessageViz(std::move(v)));
    }
    
    
    void VizManager::SubscribeToEngineEvents(IExternalInterface& externalInterface)
    {
      auto helper = AnkiEventUtil<VizManager, decltype(_eventHandlers)>(externalInterface, *this, _eventHandlers);
      
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableDisplay>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::ErasePoseMarker>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EraseQuad>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetVizOrigin>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveImages>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveRobotState>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisualizeQuad>();

    }
    
    template<>
    void VizManager::HandleMessage(const ExternalInterface::EnableDisplay& msg)
    {
      ShowObjects(msg.enable);
    }
    
    template<>
    void VizManager::HandleMessage(const ExternalInterface::ErasePoseMarker& msg)
    {
      EraseAllQuadsWithType((uint32_t)VizQuadType::VIZ_QUAD_POSE_MARKER);
    }
    
    template<>
    void VizManager::HandleMessage(const ExternalInterface::VisualizeQuad& msg)
    {
      const Quad3f quad({msg.xUpperLeft,  msg.yUpperLeft,  msg.zUpperLeft},
                        {msg.xUpperRight, msg.yUpperRight, msg.zUpperRight},
                        {msg.xLowerLeft,  msg.yLowerLeft,  msg.zLowerLeft},
                        {msg.xLowerRight, msg.yLowerRight, msg.zLowerRight});
      
      DrawGenericQuad(msg.quadID, quad, msg.color);
    }
    
    template<>
    void VizManager::HandleMessage(const SetVizOrigin& msg)
    {
      SetOrigin(msg);
    }
    
    template<>
    void VizManager::HandleMessage(const ExternalInterface::EraseQuad& msg)
    {
      EraseQuad((uint32_t)VizQuadType::VIZ_QUAD_GENERIC_3D, msg.quadID);
    }
    
    template<>
    void VizManager::HandleMessage(const ExternalInterface::SaveImages& msg)
    {
      SendSaveImages(msg.mode, msg.path);
    }
    
    template<>
    void VizManager::HandleMessage(const ExternalInterface::SaveRobotState& msg)
    {
      SendSaveState(msg.enabled, msg.path);
    }
    
    
  } // namespace Cozmo
} // namespace Anki
