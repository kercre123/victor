/**
 * File: ramp.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines a Ramp object, which is a type of DockableObject
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/ramp.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "util/logging/logging.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"


namespace Anki {
  
  namespace Cozmo {
     
    const std::vector<Point3f>& Ramp::GetCanonicalCorners() const {
    
      static const std::vector<Point3f> CanonicalCorners = {{
        // Bottom corners
        Point3f(PlatformLength + SlopeLength,  -0.5f*Width,  0.f),
        Point3f(SlopeLength,                   -0.5f*Width,  0.f),
        Point3f(SlopeLength,                    0.5f*Width,  0.f),
        Point3f(PlatformLength + SlopeLength,   0.5f*Width,  0.f),
        // Top corners:
        Point3f(SlopeLength+PlatformLength, -0.5f*Width,  Height),
        Point3f(SlopeLength,                -0.5f*Width,  Height),
        Point3f(SlopeLength,                 0.5f*Width,  Height),
        Point3f(SlopeLength+PlatformLength,  0.5f*Width,  Height),
      }};
      
      return CanonicalCorners;
      
    } // GetCanonicalCorners()
    
    
    Ramp::Ramp()
    : ActionableObject(ObjectFamily::Ramp, ObjectType::Ramp_Basic)
    , _size(SlopeLength+PlatformLength, Width, Height)
    , _preAscentPose(0, Z_AXIS_3D(),
                     {{-PreAscentDistance, 0.f, 0.f}},
                     &GetPose())
    , _preDescentPose(M_PI, Z_AXIS_3D(),
                      {{SlopeLength + PlatformLength + PreDescentDistance, 0, Height}},
                      &GetPose())
    , _vizHandle(VizManager::INVALID_HANDLE)
    
    {
      // TODO: Support multiple ramp types
      
      // Five markers:
      Pose3d frontPose(M_PI_2 - Ramp::Angle, Y_AXIS_3D(),
                       {{Ramp::FrontMarkerDistance*std::cos(Ramp::Angle),
                        0, Ramp::FrontMarkerDistance*std::sin(Ramp::Angle)}});
      frontPose *= Pose3d(-M_PI_2, Z_AXIS_3D(), {{0,0,0}});
      _frontMarker = &AddMarker(Vision::MARKER_RAMPFRONT, frontPose, Ramp::TopMarkerSize);
      
      if(_preAscentPose.GetWithRespectTo(_frontMarker->GetPose(), _preAscentPose) == false) {
        PRINT_NAMED_ERROR("Ramp.PreAscentPoseError", "Could not get preAscentPose w.r.t. front ramp marker.\n");
      }
      _preAscentPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PreAscentPose");
      AddPreActionPose(PreActionPose::ENTRY, _frontMarker, _preAscentPose);
      
      const f32 SideMarkerHeight = 16.f;
      
      const Pose3d backPose(M_PI_2, Z_AXIS_3D(), {{SlopeLength+PlatformLength, 0, SideMarkerHeight}});
      AddMarker(Vision::MARKER_RAMPBACK, backPose, Ramp::SideMarkerSize);
      
      const Vec3f PreDockPoseOffset(0.f, -DEFAULT_PREDOCK_POSE_DISTANCE_MM, -SideMarkerHeight);
      
      const Pose3d rightPose(0.f, Z_AXIS_3D(), {{120.f, -0.5f*Ramp::Width, SideMarkerHeight}});
      _rightMarker = &AddMarker(Vision::MARKER_RAMPRIGHT, rightPose, Ramp::SideMarkerSize);
      AddPreActionPose(PreActionPose::DOCKING, _rightMarker, PreDockPoseOffset);
      
      const Pose3d leftPose(M_PI, Z_AXIS_3D(), {{120.f,  0.5f*Ramp::Width, SideMarkerHeight}});
      _leftMarker = &AddMarker(Vision::MARKER_RAMPLEFT, leftPose, Ramp::SideMarkerSize);
      AddPreActionPose(PreActionPose::DOCKING, _leftMarker, PreDockPoseOffset);
      
      Pose3d topPose(-M_PI_2, Y_AXIS_3D(),
                     {{Ramp::PlatformLength + Ramp::SlopeLength - 0.025f, 0, Ramp::Height}});
      topPose *= Pose3d(M_PI_2, Z_AXIS_3D(), {{0,0,0}});
      _topMarker = &AddMarker(Vision::MARKER_INVERTED_RAMPFRONT, topPose, Ramp::TopMarkerSize);
      
      
      if(_preDescentPose.GetWithRespectTo(_topMarker->GetPose(), _preDescentPose) == false) {
        PRINT_NAMED_ERROR("Ramp.PreDescentPoseError", "Could not get preDescentPose w.r.t. top ramp marker.\n");
      }
      _preDescentPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PreDescentPose");
      AddPreActionPose(PreActionPose::ENTRY, _topMarker, _preDescentPose);
      
    } // Ramp() Constructor
    
    Ramp::~Ramp()
    {
      EraseVisualization();
    }
    
    
    Ramp::TraversalDirection Ramp::WillAscendOrDescend(const Pose3d& robotPose) const
    {
      Pose3d robotPoseWrtRamp;
      if(robotPose.GetWithRespectTo(GetPose(), robotPoseWrtRamp) == false) {
        PRINT_NAMED_WARNING("Ramp.IsAscendingOrDescending", "Could not determine robot pose w.r.t. ramp's pose.\n");
        return UNKNOWN;
      }
      
      // TODO: Better selection criteria for ascent vs. descent?
      if(robotPoseWrtRamp.GetTranslation().z() < 0.f) {
        return ASCENDING;
      } else {
        return DESCENDING;
      }
    } // IsAscendingOrDescending()
    
    
    Pose3d Ramp::GetPostAscentPose(const float wheelBase) const
    {
      Pose3d pose(0, Z_AXIS_3D(),
                  {{Ramp::SlopeLength + wheelBase, 0, Ramp::Height}},
                  &GetPose());
      
      //pose.PreComposeWith(GetPose());
      pose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PostAscentPose");
      
      return pose;
    } // GetPostAscentPose()
    
    Pose3d Ramp::GetPostDescentPose(const float wheelBase) const
    {
      Pose3d pose(M_PI, Z_AXIS_3D(),
                  {{-wheelBase, 0, 0.f}},
                  &GetPose());
      
      //pose.PreComposeWith(GetPose());
      pose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PostDescentPose");
      
      return pose;
    } // GetPostDescentPose()
    
#if 0
#pragma mark --- Virtual Method Implementations ---
#endif
    
    
    Ramp* Ramp::CloneType() const
    {
      return new Ramp();
    }
    
    void Ramp::Visualize(const ColorRGBA& color)
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = VizManager::getInstance()->DrawRamp(GetID().GetValue(), Ramp::PlatformLength,
                                                          Ramp::SlopeLength, Ramp::Width,
                                                          Ramp::Height, vizPose, color);
    } // Visualize()
    
    
    void Ramp::EraseVisualization()
    {
      // Erase the ramp
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
      
      // Erase the pre-action poses
      ActionableObject::EraseVisualization();
    }
    
    
   /*
    void Ramp::GetPreDockPoses(const float distance_mm,
                               std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                               const Vision::Marker::Code withCode) const
    {
      Pose3d preDockPose;
      
      if(withCode == Vision::Marker::ANY_CODE || withCode == _leftMarker->GetCode()) {
        const f32 distanceForThisFace = _leftMarker->GetPose().GetTranslation().Length() + distance_mm;
        if(GetPreDockPose(-Y_AXIS_3D(), distanceForThisFace, preDockPose) == true) {
          preDockPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "LeftPreDockPose");
          poseMarkerPairs.emplace_back(preDockPose, *_leftMarker);
        }
      }
      
      if(withCode == Vision::Marker::ANY_CODE || withCode == _rightMarker->GetCode()) {
        const f32 distanceForThisFace = _rightMarker->GetPose().GetTranslation().Length() + distance_mm;
        if(GetPreDockPose( Y_AXIS_3D(), distanceForThisFace, preDockPose) == true) {
          preDockPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "RightPreDockPose");
          poseMarkerPairs.emplace_back(preDockPose, *_rightMarker);
        }
      }
    } // for each canonical docking point
  
    
    f32 Ramp::GetDefaultPreDockDistance() const
    {
      return Ramp::PreDockDistance;
    }
    */
    
    // TODO: Make these dependent on ramp type/size?
    Point3f Ramp::GetSameDistanceTolerance() const {
      Point3f distTol((SlopeLength + PlatformLength)*.5f, Width*.5f, Height*.5f);
      return distTol;
    }
        
    bool Ramp::IsPreActionPoseValid(const PreActionPose& preActionPose,
                                    const Pose3d* reachableFromPose,
                                    const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const
    {
      bool isValid = ActionableObject::IsPreActionPoseValid(preActionPose, reachableFromPose, obstacles);
      if(isValid && reachableFromPose != nullptr && preActionPose.GetActionType() == PreActionPose::ENTRY) {
        // Valid according to default check, now continue with checking reachability:
        // Make sure reachableFrom pose is at about the same height of the ENTRY pose.
      
        Pose3d reachableFromWrtEntryPose;
        if(reachableFromPose->GetWithRespectTo(*preActionPose.GetPose().GetParent(), reachableFromWrtEntryPose) == false) {
          PRINT_NAMED_WARNING("Ramp.IsPreActionPoseValid.PoseOriginMisMatch",
                              "Could not get specified reachableFrom pose w.r.t. entry action's pose.\n");
          isValid = false;
        } else {
          const f32 zThreshold = GetHeight() * 0.3f;
          isValid = NEAR(reachableFromWrtEntryPose.GetTranslation().z(), preActionPose.GetPose().GetTranslation().z(), zThreshold);
        }
      }
      
      return isValid;
    }
    
  } // namespace Cozmo
} // namespace Anki


