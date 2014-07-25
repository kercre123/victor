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

namespace Anki {
  
  namespace Cozmo {
    
    const Ramp::Type Ramp::Type::BASIC_RAMP;
    
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
    : DockableObject(Type::BASIC_RAMP) // TODO: Support multiple ramp types
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      // Four markers:
      const Pose3d frontPose(1.8508, {{-0.4653f, -0.4653f, 0.7530f}},
                             {{0.5f*(Ramp::PlatformLength+Ramp::SlopeLength), 0, 0}});
      AddMarker(Vision::MARKER_RAMPFRONT, frontPose, Ramp::MarkerSize);
      
      const Pose3d backPose(-M_PI_2, Z_AXIS_3D, {{-0.5f*PlatformLength, 0, 0}});
      AddMarker(Vision::MARKER_RAMPBACK, backPose, Ramp::MarkerSize);
      
      const Pose3d leftPose(0.f, Z_AXIS_3D, {{10.f, -0.5f*Ramp::Width, 0.f}});
      _leftMarker = &AddMarker(Vision::MARKER_RAMPLEFT, leftPose, Ramp::MarkerSize);
      
      const Pose3d rightPose(M_PI, Z_AXIS_3D, {{10.f,  0.5f*Ramp::Width, 0.f}});
      _rightMarker = &AddMarker(Vision::MARKER_RAMPRIGHT, rightPose, Ramp::MarkerSize);
      
    } // Ramp() Constructor
    
    Ramp::Ramp(const Ramp& otherRamp)
    : Ramp()
    {
      SetPose(otherRamp.GetPose());
    } // Ramp() Copy Constructor
    
    Ramp::~Ramp()
    {
      EraseVisualization();
    }
    
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
    
    Ramp* Ramp::Clone() const
    {
      // Call the copy constructor
      return new Ramp(*this);
    }

    void Ramp::Visualize()
    {
      Visualize(VIZ_COLOR_DARKGRAY);
    }
    
    void Ramp::Visualize(VIZ_COLOR_ID color)
    {
      Pose3d vizPose;
      if(pose_.GetWithRespectTo(pose_.FindOrigin(), vizPose) == false) {
        // This really should not happen, by definition...
        PRINT_NAMED_ERROR("Ramp.Visualize.OriginProblem", "Could not get ramp's pose w.r.t. its own origin (?!?)\n");
        return;
      }
      
      _vizHandle = VizManager::getInstance()->DrawRamp(GetID().GetValue(), Ramp::PlatformLength,
                                                       Ramp::SlopeLength, Ramp::Width,
                                                       Ramp::Height, vizPose, color);

    } // Visualize()
    
    void Ramp::EraseVisualization()
    {
      // Draw the ramp
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
      
      // Erase the pre-dock poses
      DockableObject::EraseVisualization();
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
    
   
    void Ramp::GetPreDockPoses(const float distance_mm,
                               std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                               const Vision::Marker::Code withCode) const
    {
      Pose3d preDockPose;
      
      if(withCode == Vision::Marker::ANY_CODE || withCode == _leftMarker->GetCode()) {
        const f32 distanceForThisFace = _leftMarker->GetPose().GetTranslation().Length() + distance_mm;
        if(GetPreDockPose(-Y_AXIS_3D, distanceForThisFace, preDockPose) == true) {
          poseMarkerPairs.emplace_back(preDockPose, *_leftMarker);
        }
      }
      
      
      if(withCode == Vision::Marker::ANY_CODE || withCode == _rightMarker->GetCode()) {
        const f32 distanceForThisFace = _rightMarker->GetPose().GetTranslation().Length() + distance_mm;
        if(GetPreDockPose( Y_AXIS_3D, distanceForThisFace, preDockPose) == true) {
          poseMarkerPairs.emplace_back(preDockPose, *_rightMarker);
        }
      }
    } // for each canonical docking point
  
    ObjectType Ramp::GetTypeByName(const std::string& name)
    {
      // TODO: Support other types/names
      if(name == "BASIC_RAMP") {
        return Ramp::Type::BASIC_RAMP;
      } else {
        assert(false);
      }
    }
    
    
  } // namespace Cozmo
} // namespace Anki


