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

#include "engine/viz/vizManager.h"
#include "engine/viz/vizObjectBaseId.h"
#include "engine/debug/devLoggingSystem.h"
#include "engine/cozmoAPI/comms/gameMessagePort.h"
#include "coretech/common/engine/exceptions.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/engine/imageIO.h"
#include "coretech/vision/engine/faceTracker.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/ankiEventUtil.h"
#include "clad/vizInterface/messageViz.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/boundedWhile.h"
#include "util/helpers/templateHelpers.h"
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
      if (_isConnected) {
        Disconnect();
      }

      if (!_vizClient.Connect(udp_host_address, port)) {
        PRINT_NAMED_WARNING("VizManager.Connect.Failed", "Failed to init VizManager client (%s:%d)", udp_host_address, port);
        return RESULT_FAIL;
      }

      #if VIZ_TO_UNITY
      if (!_unityVizClient.Connect(unity_host_address, unity_port)) {
        PRINT_NAMED_WARNING("VizManager.Connect.FailedUnity", "Failed to init VizManager unity client (%s:%d)", unity_host_address, unity_port);
        return RESULT_FAIL;
      }
      #endif
     
      PRINT_NAMED_INFO("VizManager.Connect.Success", "");
      _isConnected = true;
      
      return RESULT_OK;
    }
    
    Result VizManager::Disconnect()
    {
      if(_isConnected) {
        bool vizDisconnected = _vizClient.Disconnect();
        bool unityDisconnected = true;
        #if VIZ_TO_UNITY
        unityDisconnected = _unityVizClient.Disconnect();
        #endif
        
        if (vizDisconnected || unityDisconnected) {
          _isConnected = false;
          PRINT_NAMED_INFO("VizManager.Disconnect.Success", "");
          return RESULT_OK;
        }
        return RESULT_FAIL;
      } else {
        return RESULT_OK;
      }
    }
    
    VizManager::VizManager()
    : _isConnected(false)
    , _messageCountViz(0)
    , _sendImages(false)
    {
      // Compute the max IDs permitted by VizObject type
      for (u32 i=0; i<(int)VizObjectType::NUM_VIZ_OBJECT_TYPES; ++i) {
        _VizObjectMaxID[i] = VizObjectBaseID[i+1] - VizObjectBaseID[i];
      }
    }

    void VizManager::SendMessage(const VizInterface::MessageViz& message)
    {
      if (!ANKI_DEV_CHEATS || !_isConnected || !kSendAnythingToViz)
      {
        return;
      }
      
      ANKI_CPU_PROFILE("VizManager::SendMessage");

      ++_messageCountViz;

      const size_t MAX_MESSAGE_SIZE{(size_t)VizConstants::MaxMessageSize};
      uint8_t buffer[MAX_MESSAGE_SIZE];

      const size_t numPacked = message.Pack(buffer, MAX_MESSAGE_SIZE);
      
      {
        
        ANKI_CPU_PROFILE("VizClient.Send");
        if (_vizClient.Send((const char*)buffer, numPacked) <= 0) {
          PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %s of size %zu failed", VizInterface::MessageVizTagToString(message.GetTag()), numPacked);
        }
      }

      #if VIZ_TO_UNITY
      {
        ANKI_CPU_PROFILE("UnityVizClient.SendToUnity")
        if (_unityVizClient.Send((const char*)buffer, numPacked) <= 0) {
          if ( _unityVizClient.IsConnected() ) { // prevents webots from crying when no Unity app is launched
            PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %s of size %zu to Unity failed", VizInterface::MessageVizTagToString(message.GetTag()), numPacked);
          }
        }
      }
      #endif
        
      // Log viz messages from here.
      if (ANKI_DEV_CHEATS && nullptr != DevLoggingSystem::GetInstance())
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
        VizInterface::SetRobot(
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
    
    void VizManager::DisplayCameraImage(const TimeStamp_t timestamp)
    {
      SendMessage(VizInterface::MessageViz(VizInterface::DisplayImage(timestamp)));
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
    
    void VizManager::DrawCameraPoly(const Poly2f& poly, const ColorRGBA& color, const bool isClosed)
    {
      ANKI_CPU_PROFILE("VizManager::DrawCameraPoly");
      
      auto crnt = poly.begin();
      auto next = (crnt + 1);
      auto end  = poly.end();
      auto upperBound = poly.size() + 1;
      BOUNDED_WHILE(upperBound, next != end)
      {
        DrawCameraLine(*crnt, *next, color);
        ++crnt;
        ++next;
      }
      
      if(isClosed)
      {
        DrawCameraLine(*crnt, *(poly.begin()), color);
      }
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
      
      // Draw name
      std::string name;
      if(face.GetName().empty()) {
        if(face.GetID() > 0) {
          name = "KnownFace[";
        } else {
          name = "UnknownFace[";
        }
        name += std::to_string(face.GetID()) + "]";
      } else {
        name = face.GetName();
      }
      
      // For display bars (smile, blink, expression)
      const f32 barAlpha = 1.f;
      const f32 kBarFraction = 0.1f;
      
      // Add expression and score, if not Unknown
      Vision::FacialExpression expression = face.GetMaxExpression();
      if(Vision::FacialExpression::Unknown != expression)
      {
        auto const& expressionValues = face.GetExpressionValues();
        const size_t kBufLength = 64;
        char buffer[kBufLength];
        snprintf(buffer, kBufLength, ", Ex:%s[%hhu]", EnumToString(expression),
                 expressionValues[Util::EnumToUnderlying(expression)]);
        name += buffer;
        
        // Draw expression score histogram (NOTE: sum of all OKAO expression scores is 100)
        const f32 kTotalScoreSum = 100.f;
        const ColorRGBA barColor(0.f,1.f,1.f,barAlpha);
        const s32 barWidth = std::round((1.f - (2.f*kBarFraction)) *
                                        (f32)face.GetRect().GetWidth() / (f32)expressionValues.size());
        f32 xLeft = face.GetRect().GetBottomLeft().x() + kBarFraction * face.GetRect().GetWidth();
        for(s32 iExp=0; iExp<expressionValues.size(); ++iExp)
        {
          const auto value = expressionValues[iExp];
          const s32 barHeight = std::round((f32)value / kTotalScoreSum *
                                           face.GetRect().GetHeight() * (0.5f-kBarFraction));
          if(barHeight > 0)
          {
            const f32 yTop = face.GetRect().GetBottomLeft().y()-barHeight-kBarFraction*face.GetRect().GetHeight();
            const Rectangle<s32> bar(xLeft, yTop, barWidth, barHeight);
            DrawCameraRect(bar, barColor, true);
            const std::string expStr = EnumToString((Vision::FacialExpression)iExp);
            DrawCameraText(Point2f{xLeft,yTop}, expStr.substr(0,3), NamedColors::DARKGREEN);
          }
          xLeft += barWidth;
        }
        
      }
      
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
      
      // Draw smile amount bar along bottom of bounding quad for face. Thickness (height) of bar
      // corresponds to confidence.
      const Vision::SmileAmount& smile = face.GetSmileAmount();
      if(smile.wasChecked) {
        
        const f32 barHeight = std::max(1.f, smile.confidence*kBarFraction*(quad.GetMaxY()-quad.GetMinY()));
        const f32 barWidth = std::max(1.f, smile.amount * (quad.GetMaxX() - quad.GetMinX()));
        
        Rectangle<s32> smileBar(quad.GetBottomLeft().x(),
                                quad.GetBottomLeft().y() - barHeight,
                                barWidth, barHeight);
        
        DrawCameraRect(smileBar, ColorRGBA(0.f,0.f,1.f,barAlpha), true);
      }
      
      // Draw L/R blink amount bars along sides of bounding quad for face
      // Note: bars are bigger when eyes are more _open_
      const Vision::BlinkAmount& blink = face.GetBlinkAmount();
      if(blink.wasChecked) {
        const f32 barWidth  = kBarFraction*face.GetRect().GetWidth();
        
        // Left
        {
          const f32 barHeight = (1.f-blink.blinkAmountLeft) * (quad.GetMaxY()-quad.GetMinY());
          Rectangle<s32> blinkBar(quad.GetTopLeft().x(), quad.GetTopLeft().y(), barWidth, barHeight);
          DrawCameraRect(blinkBar, ColorRGBA(0.f,.5f,0.f,barAlpha), true);
        }

        // Right
        {
          const f32 barHeight = (1.f-blink.blinkAmountRight) * (quad.GetMaxY()-quad.GetMinY());
          Rectangle<s32> blinkBar(quad.GetTopRight().x()-barWidth, quad.GetTopRight().y(), barWidth, barHeight);
          DrawCameraRect(blinkBar, ColorRGBA(1.f,0.f,0.f,barAlpha), true);
        }
      }
      
      // Draw gaze indicator as line from face center in the direction of the gaze
      const Vision::Gaze& gaze = face.GetGaze();
      if(gaze.wasChecked) {
        const Point2f centerPt( face.GetRect().GetMidPoint() );
        Point2f lineEnd(centerPt);
        lineEnd.x() += face.GetRect().GetWidth() * 0.5f * std::sinf(DEG_TO_RAD(gaze.leftRight_deg));
        
        // Note that we subtract for y because positive y is down in the image
        lineEnd.y() -= face.GetRect().GetHeight() * 0.5f * std::sinf(DEG_TO_RAD(gaze.upDown_deg));
        
        DrawCameraLine(centerPt, lineEnd, NamedColors::RED);
        DrawCameraOval(centerPt, 1.f, 1.f, NamedColors::RED);
      }
      
    }
    
    void VizManager::EraseRobot(const u32 robotID)
    {
      const int viztype = (int)VizObjectType::VIZ_OBJECT_ROBOT;
      DEV_ASSERT(robotID < _VizObjectMaxID[viztype], "VizManager.EraseRobot.InvalidID");
      EraseVizObject(VizObjectBaseID[viztype] + robotID);
    }
    
    void VizManager::EraseCuboid(const u32 blockID)
    {
      const int viztype = (int)VizObjectType::VIZ_OBJECT_CUBOID;
      DEV_ASSERT(blockID < _VizObjectMaxID[viztype], "VizManager.EraseCuboid.InvalidID");
      EraseVizObject(VizObjectBaseID[viztype] + blockID);
    }

    void VizManager::EraseAllCuboids()
    {
      EraseVizObjectType(VizObjectType::VIZ_OBJECT_CUBOID);
    }
    
    void VizManager::ErasePreDockPose(const u32 preDockPoseID)
    {
      const int viztype = (int)VizObjectType::VIZ_OBJECT_PREDOCKPOSE;
      DEV_ASSERT(preDockPoseID < _VizObjectMaxID[viztype], "VizManager.ErasePreDockPose.InvalidID");
      EraseVizObject(VizObjectBaseID[viztype] + preDockPoseID);
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
      EraseAllPaths();
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
        
        DEV_ASSERT(quadsPerMessage>0, "VizManager.DrawQuadVector.InvalidQuadsPerMessage");
        
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
      
      char buffer[255]{0};
      va_list argptr;
      va_start(argptr, format);
      vsnprintf(buffer, 255, format, argptr);
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
    
    void VizManager::SendCameraParams(const CameraParams& params)
    {
      ANKI_CPU_PROFILE("VizManager::SendCameraParams");
      SendMessage(VizInterface::MessageViz(VizInterface::CameraParams(std::move(params))));
    }

    void VizManager::SendRobotState(const RobotState &msg,
                                    const u8  videoFrameRateHz,
                                    const u8  imageProcFrameRateHz,
                                    const u32 numProcAnimFaceKeyframes,
                                    const u8  lockedTracks,
                                    const u8  tracksInUse,                                    
                                    const f32 imuTemperature_degC,
                                    std::array<uint16_t, 4> cliffThresholds,
                                    const float batteryVolts
                                    )
    {
      ANKI_CPU_PROFILE("VizManager::SendRobotState");
      SendMessage(VizInterface::MessageViz(VizInterface::RobotStateMessage(msg, imuTemperature_degC, numProcAnimFaceKeyframes, cliffThresholds, videoFrameRateHz, imageProcFrameRateHz, lockedTracks, tracksInUse, batteryVolts)));
    }

    void VizManager::SendCurrentAnimation(const std::string& animName, u8 animTag)
    {
      ANKI_CPU_PROFILE("VizManager::SendCurrentAnimation");
      SendMessage(VizInterface::MessageViz(VizInterface::CurrentAnimation(animTag, animName)));  
    }
    
    void VizManager::SendRobotMood(VizInterface::RobotMood&& robotMood)
    {
      ANKI_CPU_PROFILE("VizManager::SendRobotMood");
      SendMessage(VizInterface::MessageViz(std::move(robotMood)));
    }

    void VizManager::SendBehaviorStackDebug(VizInterface::BehaviorStackDebug&& behaviorStackDebug)
    {
      ANKI_CPU_PROFILE("VizManager::SendBehaviorStackDebug");
      SendMessage(VizInterface::MessageViz(std::move(behaviorStackDebug)));
    }

    void VizManager::SendVisionModeDebug(VizInterface::VisionModeDebug&& visionModeDebug)
    {
      ANKI_CPU_PROFILE("VizManager::SendVisionModeDebug");
      SendMessage(VizInterface::MessageViz(std::move(visionModeDebug)));
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
      
    void VizManager::SendNewReactionTriggered(VizInterface::NewReactionTriggered&& newReactionTriggered)
    {
      ANKI_CPU_PROFILE("VizManager::SendNewReactionTriggered");
      SendMessage(VizInterface::MessageViz(std::move(newReactionTriggered)));
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
    
    void VizManager::SendVizMessage(VizInterface::MessageViz&& event)
    {
      ANKI_CPU_PROFILE("VizManager::SendVizMessage");
      SendMessage(event);
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
    
    void VizManager::SendObjectAccelState(u32 objectID, const ActiveAccel& accel)
    {
      ANKI_CPU_PROFILE("VizManager::SendObjectAccelState");
      SendMessage(VizInterface::MessageViz(VizInterface::ObjectAccelState(objectID, accel)));
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
