/**
 * File: charger.cpp
 *
 * Author: Kevin Yoon
 * Date:   6/5/2015
 *
 * Description: Defines a charger base object, which is a type of DockableObject
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/charger.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "util/logging/logging.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"


namespace Anki {
  
  namespace Cozmo {
    
    const std::vector<Point3f>& Charger::GetCanonicalCorners() const {
    
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
    
    
    Charger::Charger()
    : ActionableObject(ObjectFamily::Charger, ObjectType::Charger_Basic)
    , _size(SlopeLength+PlatformLength, Width, Height)
    , _preAscentPose(0, Z_AXIS_3D(),
                     {{-PreAscentDistance, 0.f, 0.f}},
                     &GetPose())
    , _preDescentPose(M_PI, Z_AXIS_3D(),
                      {{SlopeLength + PlatformLength + PreDescentDistance, 0, Height}},
                      &GetPose())
    , _vizHandle(VizManager::INVALID_HANDLE)
    
    {
      // TODO: Support multiple Charger types
      
      Pose3d frontPose(M_PI_2 - Charger::Angle, Y_AXIS_3D(),
                       {{Charger::FrontMarkerDistance*std::cos(Charger::Angle),
                        0, Charger::FrontMarkerDistance*std::sin(Charger::Angle)}});
      frontPose *= Pose3d(-M_PI_2, Z_AXIS_3D(), {{0,0,0}});
      _frontMarker = &AddMarker(Vision::MARKER_RAMPFRONT, frontPose, Charger::TopMarkerSize);
      
      if(_preAscentPose.GetWithRespectTo(_frontMarker->GetPose(), _preAscentPose) == false) {
        PRINT_NAMED_ERROR("Charger.PreAscentPoseError", "Could not get preAscentPose w.r.t. front Charger marker.\n");
      }
      _preAscentPose.SetName("Charger" + std::to_string(GetID().GetValue()) + "PreAscentPose");
      AddPreActionPose(PreActionPose::ENTRY, _frontMarker, _preAscentPose);
      
/*
      Pose3d topPose(-M_PI_2, Y_AXIS_3D(),
                     {{Charger::PlatformLength + Charger::SlopeLength - 0.025f, 0, Charger::Height}});
      topPose *= Pose3d(M_PI_2, Z_AXIS_3D(), {{0,0,0}});
      _topMarker = &AddMarker(Vision::MARKER_INVERTED_RAMPFRONT, topPose, Charger::TopMarkerSize);
      
      
      if(_preDescentPose.GetWithRespectTo(_topMarker->GetPose(), _preDescentPose) == false) {
        PRINT_NAMED_ERROR("Charger.PreDescentPoseError", "Could not get preDescentPose w.r.t. top Charger marker.\n");
      }
      _preDescentPose.SetName("Charger" + std::to_string(GetID().GetValue()) + "PreDescentPose");
      AddPreActionPose(PreActionPose::ENTRY, _topMarker, _preDescentPose);
  */
    } // Charger() Constructor
    
    Charger::~Charger()
    {
      EraseVisualization();
    }
    
    
    Charger::TraversalDirection Charger::WillAscendOrDescend(const Pose3d& robotPose) const
    {
      Pose3d robotPoseWrtCharger;
      if(robotPose.GetWithRespectTo(GetPose(), robotPoseWrtCharger) == false) {
        PRINT_NAMED_WARNING("Charger.IsAscendingOrDescending", "Could not determine robot pose w.r.t. Charger's pose.\n");
        return UNKNOWN;
      }
      
      // TODO: Better selection criteria for ascent vs. descent?
      if(robotPoseWrtCharger.GetTranslation().z() < 0.f) {
        return ASCENDING;
      } else {
        return DESCENDING;
      }
    } // IsAscendingOrDescending()
    
    
    Pose3d Charger::GetPostAscentPose(const float wheelBase) const
    {
      Pose3d pose(0, Z_AXIS_3D(),
                  {{Charger::SlopeLength + wheelBase, 0, Charger::Height}},
                  &GetPose());
      
      //pose.PreComposeWith(GetPose());
      pose.SetName("Charger" + std::to_string(GetID().GetValue()) + "PostAscentPose");
      
      return pose;
    } // GetPostAscentPose()
    
    Pose3d Charger::GetPostDescentPose(const float wheelBase) const
    {
      Pose3d pose(M_PI, Z_AXIS_3D(),
                  {{-wheelBase, 0, 0.f}},
                  &GetPose());
      
      //pose.PreComposeWith(GetPose());
      pose.SetName("Charger" + std::to_string(GetID().GetValue()) + "PostDescentPose");
      
      return pose;
    } // GetPostDescentPose()
    
#if 0
#pragma mark --- Virtual Method Implementations ---
#endif
    
    
    Charger* Charger::CloneType() const
    {
      return new Charger();
    }
    
    void Charger::Visualize(const ColorRGBA& color)
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = VizManager::getInstance()->DrawCharger(GetID().GetValue(), Charger::PlatformLength,
                                                          Charger::SlopeLength, Charger::Width,
                                                          Charger::Height, vizPose, color);
    } // Visualize()
    
    
    void Charger::EraseVisualization()
    {
      // Erase the Charger
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
      
      // Erase the pre-action poses
      ActionableObject::EraseVisualization();
    }

    
    // TODO: Make these dependent on Charger type/size?
    Point3f Charger::GetSameDistanceTolerance() const {
      Point3f distTol((SlopeLength + PlatformLength)*.5f, Width*.5f, Height*.5f);
      return distTol;
    }
        
    bool Charger::IsPreActionPoseValid(const PreActionPose& preActionPose,
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


