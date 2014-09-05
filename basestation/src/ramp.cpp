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

#include "ramp.h"

#include "anki/common/basestation/utils/logging/logging.h"

#include "anki/cozmo/robot/cozmoConfig.h"


namespace Anki {
  
  namespace Cozmo {
    
    const ObjectType Ramp::Type("BASIC_RAMP");
    
    const f32 Ramp::Angle = atan(Height/SlopeLength);
    
    const std::array<Point3f, Ramp::NUM_CORNERS> Ramp::CanonicalCorners = {{
      // Bottom corners
      Point3f(-0.5f*PlatformLength,              -0.5f*Width,  -0.5f*Height),
      Point3f(SlopeLength + 0.5f*PlatformLength, -0.5f*Width,  -0.5f*Height),
      Point3f(SlopeLength + 0.5f*PlatformLength,  0.5f*Width,  -0.5f*Height),
      Point3f(-0.5f*PlatformLength,               0.5f*Width,  -0.5f*Height),
      // Top corners:
      Point3f(-0.5f*PlatformLength, -0.5f*Width,  0.5f*Height),
      Point3f( 0.5f*PlatformLength, -0.5f*Width,  0.5f*Height),
      Point3f( 0.5f*PlatformLength,  0.5f*Width,  0.5f*Height),
      Point3f(-0.5f*PlatformLength,  0.5f*Width,  0.5f*Height),
    }};
    
    
    Ramp::Ramp()
    : _preAscentPose(M_PI, Z_AXIS_3D,
                     {{Ramp::PlatformLength*.5f + Ramp::SlopeLength + Ramp::PreAscentDistance, 0, -.5f*Ramp::Height}},
                     &GetPose())
    , _preDescentPose(0, Z_AXIS_3D,
                      {{-(.5f*Ramp::PlatformLength+Ramp::PreDescentDistance), 0, 0.5f*Ramp::Height}},
                      &GetPose())
    , _vizHandle(VizManager::INVALID_HANDLE)
    
    {
      // TODO: Support multiple ramp types
      
      // Five markers:
      const f32 angle = -atan(Ramp::SlopeLength/Ramp::Height);
      Pose3d frontPose(angle, Y_AXIS_3D,
                       {{Ramp::SlopeLength+Ramp::PlatformLength*.5f-Ramp::FrontMarkerDistance,
                         0, -Ramp::Height*.5f + Ramp::Height*Ramp::FrontMarkerDistance/Ramp::SlopeLength}});
      frontPose *= Pose3d(M_PI_2, Z_AXIS_3D, {{0,0,0}});
      _frontMarker = &AddMarker(Vision::MARKER_RAMPFRONT, frontPose, Ramp::MarkerSize);
      
      if(_preAscentPose.GetWithRespectTo(_frontMarker->GetPose(), _preAscentPose) == false) {
        PRINT_NAMED_ERROR("Ramp.PreAscentPoseError", "Could not get preAscentPose w.r.t. front ramp marker.\n");
      }
      _preAscentPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PreAscentPose");
      AddPreActionPose(PreActionPose::ENTRY, _frontMarker, _preAscentPose, DEG_TO_RAD(-10));
      
      const Pose3d backPose(-M_PI_2, Z_AXIS_3D, {{-0.5f*PlatformLength, 0, 0}});
      AddMarker(Vision::MARKER_RAMPBACK, backPose, Ramp::MarkerSize);
      
      const Vec3f PreDockPoseOffset(0.f, -100.f, -0.5f*Ramp::Height);
      
      const Pose3d leftPose(0.f, Z_AXIS_3D, {{10.f, -0.5f*Ramp::Width, 0.f}});
      _leftMarker = &AddMarker(Vision::MARKER_RAMPLEFT, leftPose, Ramp::MarkerSize);
      AddPreActionPose(PreActionPose::DOCKING, _leftMarker, PreDockPoseOffset, DEG_TO_RAD(-15));
      
      const Pose3d rightPose(M_PI, Z_AXIS_3D, {{10.f,  0.5f*Ramp::Width, 0.f}});
      _rightMarker = &AddMarker(Vision::MARKER_RAMPRIGHT, rightPose, Ramp::MarkerSize);
      AddPreActionPose(PreActionPose::DOCKING, _rightMarker, PreDockPoseOffset, DEG_TO_RAD(-15));
      
      const Pose3d topPose(2.0944, {{-0.5774f, 0.5774f, -0.5774f}},
                           {{Ramp::PlatformLength*.5f - Ramp::MarkerSize*.5f, 0, Ramp::Height*.5f}});
      _topMarker = &AddMarker(Vision::MARKER_INVERTED_RAMPFRONT, topPose, Ramp::MarkerSize);
      
      
      if(_preDescentPose.GetWithRespectTo(_topMarker->GetPose(), _preDescentPose) == false) {
        PRINT_NAMED_ERROR("Ramp.PreDescentPoseError", "Could not get preDescentPose w.r.t. top ramp marker.\n");
      }
      _preDescentPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PreDescentPose");
      AddPreActionPose(PreActionPose::ENTRY, _topMarker, _preDescentPose, MIN_HEAD_ANGLE);
      
    } // Ramp() Constructor
    
    Ramp::~Ramp()
    {
      EraseVisualization();
    }
    
    
    Ramp::TraversalDirection Ramp::IsAscendingOrDescending(const Pose3d& robotPose) const
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
      Pose3d pose(M_PI, Z_AXIS_3D,
                  {{Ramp::PlatformLength*.5f - wheelBase, 0, Ramp::Height*.5f}},
                  &GetPose());
      
      //pose.PreComposeWith(GetPose());
      pose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PostAscentPose");
      
      return pose;
    } // GetPostAscentPose()
    
    Pose3d Ramp::GetPostDescentPose(const float wheelBase) const
    {
      Pose3d pose(0, Z_AXIS_3D,
                  {{Ramp::PlatformLength*.5f + Ramp::SlopeLength + wheelBase, 0, -Ramp::Height*.5f}},
                  &GetPose());
      
      //pose.PreComposeWith(GetPose());
      pose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "PostDescentPose");
      
      return pose;
    } // GetPostDescentPose()
    
#if 0
#pragma mark --- Virtual Method Implementations ---
#endif
    
    void Ramp::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
    {
      corners.resize(NUM_CORNERS);
      for(s32 i=0; i<NUM_CORNERS; ++i) {
        // Start with canonical corner
        corners[i] = Ramp::CanonicalCorners[i];

        // Move to given pose
        corners[i] = atPose * corners[i];
      }
    } // GetCorners()
    
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
    
    
    Quad2f Ramp::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      const RotationMatrix3d& R = atPose.GetRotationMatrix();
      
      std::vector<Point2f> points;
      points.reserve(Ramp::NUM_CORNERS);
      for(auto corner : Ramp::CanonicalCorners) {
        
        // Move canonical point to correct (padded) size
        corner.x() += (signbit(corner.x()) ? -padding_mm : padding_mm);
        corner.y() += (signbit(corner.y()) ? -padding_mm : padding_mm);
        corner.z() += (signbit(corner.z()) ? -padding_mm : padding_mm);
        
        // Rotate to given pose
        corner = R*corner;
        
        // Project onto XY plane, i.e. just drop the Z coordinate
        points.emplace_back(corner.x(), corner.y());
      }
      
      Quad2f boundingQuad = GetBoundingQuad(points);
      
      // Re-center
      Point2f center(atPose.GetTranslation().x(), atPose.GetTranslation().y());
      boundingQuad += center;
      
      return boundingQuad;
    } // GetBoundingQuadXY()
    
   /*
    void Ramp::GetPreDockPoses(const float distance_mm,
                               std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                               const Vision::Marker::Code withCode) const
    {
      Pose3d preDockPose;
      
      if(withCode == Vision::Marker::ANY_CODE || withCode == _leftMarker->GetCode()) {
        const f32 distanceForThisFace = _leftMarker->GetPose().GetTranslation().Length() + distance_mm;
        if(GetPreDockPose(-Y_AXIS_3D, distanceForThisFace, preDockPose) == true) {
          preDockPose.SetName("Ramp" + std::to_string(GetID().GetValue()) + "LeftPreDockPose");
          poseMarkerPairs.emplace_back(preDockPose, *_leftMarker);
        }
      }
      
      if(withCode == Vision::Marker::ANY_CODE || withCode == _rightMarker->GetCode()) {
        const f32 distanceForThisFace = _rightMarker->GetPose().GetTranslation().Length() + distance_mm;
        if(GetPreDockPose( Y_AXIS_3D, distanceForThisFace, preDockPose) == true) {
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
      return Point3f((SlopeLength + PlatformLength)*.5f, Width*.5f, Height*.5f);
    }
    
    Radians Ramp::GetSameAngleTolerance() const {
      return DEG_TO_RAD(45);
    }

    
    ObjectType Ramp::GetTypeByName(const std::string& name)
    {
      // TODO: Support other types/names
      if(name == Ramp::Type.GetName()) {
        return Ramp::Type;
      } else {
        assert(false);
      }
    }
    
    
    bool Ramp::IsPreActionPoseValid(const PreActionPose& preActionPose,
                                    const Pose3d* reachableFromPose) const
    {
      bool isValid = ActionableObject::IsPreActionPoseValid(preActionPose, reachableFromPose);
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


