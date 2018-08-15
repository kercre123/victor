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
  
  namespace Vector {
    
    // === Charger predock pose params ===
    // {angle, x, y}
    // angle: angle about z-axis (which runs vertically along marker)
    //     x: distance along marker normal
    //     y: distance along marker horizontal
    const Pose2d kChargerPreDockPoseOffset = {0, 0, 130.f};
    
    const std::vector<Point3f>& Charger::GetCanonicalCorners() const {
    
      static const std::vector<Point3f> CanonicalCorners = {{
        // Bottom corners
        Point3f(kLength, -0.5f*kWidth,  0.f),
        Point3f(0,      -0.5f*kWidth,  0.f),
        Point3f(0,       0.5f*kWidth,  0.f),
        Point3f(kLength,  0.5f*kWidth,  0.f),
        // Top corners:
        Point3f(kLength, -0.5f*kWidth,  kHeight),
        Point3f(0,       -0.5f*kWidth,  kHeight),
        Point3f(0,        0.5f*kWidth,  kHeight),
        Point3f(kLength,  0.5f*kWidth,  kHeight),
      }};
      
      return CanonicalCorners;
      
    } // GetCanonicalCorners()
    
    
    Charger::Charger(ObjectType type)
    : ObservableObject(ObjectFamily::Charger, type), ActionableObject()
    , _size(kLength, kWidth, kHeight)
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      Pose3d frontPose(-M_PI_2_F, Z_AXIS_3D(),
                       Point3f{kSlopeLength+kPlatformLength, 0, kMarkerZPosition});
      
      _marker = &AddMarker(Vision::MARKER_CHARGER_HOME, frontPose, Point2f(kMarkerWidth, kMarkerHeight));
      
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
                  Point3f{kRobotToChargerDistWhenDocked, 0, 0},
                  GetPose());
      
      pose.SetName("Charger" + std::to_string(GetID().GetValue()) + "DockedPose");
      
      return pose;
    }
    
    Pose3d Charger::GetRobotPostRollOffPose() const
    {
      Pose3d pose(M_PI_F, Z_AXIS_3D(),
                  Point3f{-kRobotToChargerDistPostRollOff, 0, 0},
                  GetPose());
      
      pose.SetName("Charger" + std::to_string(GetID().GetValue()) + "PostRollOffPose");
      
      return pose;
    }
    
    Pose3d Charger::GetDockPoseRelativeToRobot(const Robot& robot)
    {
      return Pose3d(M_PI_F, Z_AXIS_3D(),
                    Point3f{kRobotToChargerDistWhenDocked, 0, 0},
                    robot.GetPose(),
                    "ChargerDockPose");
    }
    
    Quad2f Charger::GetDockingAreaQuad() const
    {
      // Define the docking area w.r.t. charger. This defines the area in
      // front of the charger that must be clear of obstacles if the robot
      // is to successfully dock with the charger.
      const float xExtent_mm = 120.f;
      const float yExtent_mm = kWidth;
      std::vector<Point3f> dockingAreaPts = {{
        {0.f,         -yExtent_mm/2.f, 0.f},
        {-xExtent_mm, -yExtent_mm/2.f, 0.f},
        {0.f,         +yExtent_mm/2.f, 0.f},
        {-xExtent_mm, +yExtent_mm/2.f, 0.f}
      }};
      
      const auto& chargerPose = GetPose();
      const RotationMatrix3d& R = chargerPose.GetRotationMatrix();
      std::vector<Point2f> points;
      for (auto& pt : dockingAreaPts) {
        // Rotate to charger pose
        pt = R*pt;
        
        // Project onto XY plane, i.e. just drop the Z coordinate
        points.emplace_back(pt.x(), pt.y());
      }
      
      Quad2f boundingQuad = GetBoundingQuad(points);
      
      // Re-center
      Point2f center(chargerPose.GetTranslation().x(), chargerPose.GetTranslation().y());
      boundingQuad += center;
      
      return boundingQuad;
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
      _vizHandle = _vizManager->DrawCharger(GetID().GetValue(),
                                            Charger::kPlatformLength + Charger::kWallWidth,
                                            Charger::kSlopeLength,
                                            Charger::kWidth,
                                            Charger::kHeight,
                                            vizPose,
                                            color);
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
      Point3f distTol(kLength*.5f, kWidth*.5f, kHeight*.5f);
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
    
  } // namespace Vector
} // namespace Anki


