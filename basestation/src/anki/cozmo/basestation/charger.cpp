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
#include "anki/cozmo/basestation/charger.h"

#include "anki/cozmo/basestation/objectPoseConfirmer.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "util/logging/logging.h"

namespace Anki {
  
  namespace Cozmo {
    
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
    : ObservableObject(ObjectFamily::Charger, type)
    , _size(Length, Width, Height)
    , _vizHandle(VizManager::INVALID_HANDLE)
    
    {
      // TODO: Support multiple Charger types
      
      Pose3d frontPose(-M_PI_2, Z_AXIS_3D(),
                       Point3f{SlopeLength+PlatformLength, 0, MarkerZPosition});
      _marker = &AddMarker(Vision::MARKER_CHARGER, frontPose, Point2f(MarkerWidth, MarkerHeight));

      // PreActionPose, initialized to be with respect to charger
      Pose3d preActionPose(0, Z_AXIS_3D(),
                           Point3f{-PreAscentDistance, 0.f, 0.f},
                           &GetPose());
      
      if(preActionPose.GetWithRespectTo(_marker->GetPose(), preActionPose) == false) {
        PRINT_NAMED_ERROR("Charger.PreActionPoseError", "Could not get preActionPose w.r.t. front Charger marker");
      }
      preActionPose.SetName("Charger" + std::to_string(GetID().GetValue()) + "PreActionPose");
      AddPreActionPose(PreActionPose::ENTRY, _marker, preActionPose);
      
    } // Charger() Constructor
    
    Charger::Charger(ActiveID activeID, FactoryID factoryID, ActiveObjectType activeObjectType)
    : Charger(GetTypeFromActiveObjectType(activeObjectType))
    {
      ASSERT_NAMED(GetTypeFromActiveObjectType(activeObjectType) == ObjectType::Charger_Basic, "Charger.InvalidFactoryID");
      
      _activeID = activeID;
      _factoryID = factoryID;
    }

    
    Charger::~Charger()
    {
      EraseVisualization();
    }

    
    Pose3d Charger::GetDockedPose() const
    {
      Pose3d pose(M_PI, Z_AXIS_3D(),
                  Point3f{RobotToChargerDistWhenDocked, 0, 0},
                  &GetPose());
      
      pose.SetName("Charger" + std::to_string(GetID().GetValue()) + "DockedPose");
      
      return pose;
    }
    
    void Charger::SetPoseRelativeToRobot(Robot& robot) // const Pose3d& robotPose, BlockWorld& blockWorld)
    {
      Pose3d relPose(-M_PI, Z_AXIS_3D(),
                     Point3f{RobotToChargerDistWhenDocked, 0, 0},
                     &robot.GetPose(),
                     "Charger" + std::to_string(GetID().GetValue()) + "DockedPose");
      
      robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(this, relPose, PoseState::Known);
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
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
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


