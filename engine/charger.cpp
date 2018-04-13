/**
 * File: charger.cpp
 *
 * Author: Kevin Yoon
 * Date:   6/5/2015
 *
 * Description: Defines a charger base object, which is a type of ActionableObject
 *
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "engine/charger.h"

#include "engine/objectPoseConfirmer.h"
#include "engine/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "coretech/common/engine/math/quad_impl.h"

#include "util/logging/logging.h"

namespace Anki {
  
  namespace Cozmo {
    
    // === Charger predock pose params ===
    // {angle, x, y}
    // angle: angle about z-axis (which runs vertically along marker)
    //     x: distance along marker horizontal
    //     y: distance along marker normal
    const Pose2d kChargerPreDockPoseOffset = {0, 0, 130.f};
    
    const std::vector<Point3f>& Charger::GetCanonicalCorners() const {
    
      static const std::vector<Point3f> CanonicalCorners = {{
        // Bottom corners
        Point3f(Length, -0.5f*Width,  0.f),
        Point3f(0,      -0.5f*Width,  0.f),
        Point3f(0,       0.5f*Width,  0.f),
        Point3f(Length,  0.5f*Width,  0.f),
        // Top corners:
        Point3f(Length, -0.5f*Width,  Height),
        Point3f(0,      -0.5f*Width,  Height),
        Point3f(0,       0.5f*Width,  Height),
        Point3f(Length,  0.5f*Width,  Height),
      }};
      
      return CanonicalCorners;
      
    } // GetCanonicalCorners()
    
    
    Charger::Charger(ObjectType type)
    : ObservableObject(ObjectFamily::Charger, type), ActionableObject()
    , _size(Length, Width, Height)
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      Pose3d frontPose(-M_PI_2_F, Z_AXIS_3D(),
                       Point3f{SlopeLength+PlatformLength, 0, MarkerZPosition});
      
#ifdef SIMULATOR
      // Simulation uses new marker with battery
      const auto markerType = Vision::MARKER_CHARGER_HOME;
#else
      // Real robot uses old marker with eyes until VIC-945
      const auto markerType = Vision::MARKER_CHARGER_HOME_EYES;
#endif
      
      _marker = &AddMarker(markerType, frontPose, Point2f(MarkerWidth, MarkerHeight));
      
    } // Charger() Constructor

    
    Charger::~Charger()
    {
      EraseVisualization();
    }

    void Charger::GeneratePreActionPoses(const PreActionPose::ActionType type,
                                         std::vector<PreActionPose>& preActionPoses) const
    {
      preActionPoses.clear();
      
      switch(type)
      {
        case PreActionPose::ActionType::DOCKING:
        case PreActionPose::ActionType::PLACE_RELATIVE:
        {
          const float halfHeight = 0.5f * GetHeight();
          
          Pose3d poseWrtMarker(M_PI_2_F + kChargerPreDockPoseOffset.GetAngle().ToFloat(),
                               Z_AXIS_3D(),
                               {kChargerPreDockPoseOffset.GetX() , -kChargerPreDockPoseOffset.GetY(), -halfHeight},
                               _marker->GetPose());
          
          poseWrtMarker.SetName("Charger" + std::to_string(GetID().GetValue()) + "PreActionPose");
          
          preActionPoses.emplace_back(type,
                                      _marker,
                                      poseWrtMarker,
                                      0);
          break;
        }
        case PreActionPose::ActionType::ENTRY:
        case PreActionPose::ActionType::FLIPPING:
        case PreActionPose::ActionType::PLACE_ON_GROUND:
        case PreActionPose::ActionType::ROLLING:
        case PreActionPose::ActionType::NONE:
        {
          break;
        }
      }
    }
    
    Pose3d Charger::GetRobotDockedPose() const
    {
      Pose3d pose(M_PI, Z_AXIS_3D(),
                  Point3f{RobotToChargerDistWhenDocked, 0, 0},
                  GetPose());
      
      pose.SetName("Charger" + std::to_string(GetID().GetValue()) + "DockedPose");
      
      return pose;
    }
    
    Pose3d Charger::GetDockPoseRelativeToRobot(const Robot& robot)
    {
      return Pose3d(M_PI_F, Z_AXIS_3D(),
                    Point3f{RobotToChargerDistWhenDocked, 0, 0},
                    robot.GetPose(),
                    "ChargerDockPose");
    }
    
#if 0
#pragma mark --- Virtual Method Implementations ---
#endif
    
    
    Charger* Charger::CloneType() const
    {
      return new Charger();
    }
    
    void Charger::Visualize(const ColorRGBA& color) const
    {
      Pose3d vizPose = GetPose().GetWithRespectToRoot();
      _vizHandle = _vizManager->DrawCharger(GetID().GetValue(), Charger::PlatformLength + Charger::WallWidth,
                                                          Charger::SlopeLength, Charger::Width,
                                                          Charger::Height, vizPose, color);
    } // Visualize()
    
    
    void Charger::EraseVisualization() const
    {
      // Erase the Charger
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        _vizManager->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
      
      // Erase the pre-action poses
      ActionableObject::EraseVisualization();
    }

    
    // TODO: Make these dependent on Charger type/size?
    Point3f Charger::GetSameDistanceTolerance() const {
      Point3f distTol(Length*.5f, Width*.5f, Height*.5f);
      return distTol;
    }
        
    bool Charger::IsPreActionPoseValid(const PreActionPose& preActionPose,
                                    const Pose3d* reachableFromPose,
                                    const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const
    {
      bool isValid = ActionableObject::IsPreActionPoseValid(preActionPose, reachableFromPose, obstacles);
      
      // TODO: While charger pose estimation is as jumpy as it currently is, skip height check
      /*
      if(isValid && reachableFromPose != nullptr && preActionPose.GetActionType() == PreActionPose::ENTRY) {
        // Valid according to default check, now continue with checking reachability:
        // Make sure reachableFrom pose is at about the same height of the ENTRY pose.
        
        Pose3d reachableFromWrtEntryPose;
        if(reachableFromPose->GetWithRespectTo(*preActionPose.GetPose().GetParent(), reachableFromWrtEntryPose) == false) {
          PRINT_NAMED_WARNING("Charger.IsPreActionPoseValid.PoseOriginMisMatch",
                              "Could not get specified reachableFrom pose w.r.t. entry action's pose.\n");
          isValid = false;
        } else {
          const f32 zThreshold = 10.f;
          isValid = std::fabsf(reachableFromWrtEntryPose.GetTranslation().z()) < zThreshold;
        }
      }
       */
      
      return isValid;
    }
    
  } // namespace Cozmo
} // namespace Anki


