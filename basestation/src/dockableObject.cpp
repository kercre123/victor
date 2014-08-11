/**
 * File: dockableObject.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines a "Dockable" Object, which is a subclass of an
 *              ObservableObject that can also be docked with.
 *              It extends the (coretech) ObservableObject to have a notion of
 *              docking for Cozmo's purposes, by offering, for example, the
 *              ability to request pre-dock poses.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "dockableObject.h"

namespace Anki {
  namespace Cozmo {
    
#if 0
#pragma mark --- PreActionPose ---
#endif
    
    const ColorRGBA& PreActionPose::GetVisualizeColor(ActionType type)
    {
      static const std::map<ActionType, ColorRGBA> ColorLUT = {
        {DOCKING, ColorRGBA(0.f,0.f,1.f,0.5f)},
        {ENTRY,   ColorRGBA(1.f,0.f,0.f,0.5f)}
      };
      
      return ColorLUT.at(type);
    }
    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 f32 distance)
    : _type(type)
    //, _canonicalPoint(Y_AXIS_3D * -distance)
    , _marker(marker)
    , _poseWrtMarkerParent(M_PI_2, Z_AXIS_3D, Y_AXIS_3D * -distance, &marker->GetPose()) // init w.r.t. marker
    {
      //_canonicalPoint = marker->GetPose() * _canonicalPoint;
      
      // Now make pose w.r.t. marker parent
      if(_poseWrtMarkerParent.GetWithRespectTo(*_marker->GetPose().GetParent(), _poseWrtMarkerParent) == false) {
        PRINT_NAMED_ERROR("PreActionPose.GetPoseWrtMarkerParentFailed",
                          "Could not get the pre-action pose w.r.t. the marker's parent.\n");
      }
      _poseWrtMarkerParent.SetName("PreActionPose");
      
    } // PreActionPose Constructor
    
    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 const Pose3d& poseWrtMarker)
    : _type(type)
    , _marker(marker)
    {
      if(poseWrtMarker.GetParent() != &marker->GetPose()) {
        PRINT_NAMED_ERROR("PreActionPose.PoseWrtMarkerParentInvalid",
                          "Given pose w.r.t. marker should have the marker as its parent pose.\n");
      }
      if(poseWrtMarker.GetWithRespectTo(*_marker->GetPose().GetParent(), _poseWrtMarkerParent) == false) {
        PRINT_NAMED_ERROR("PreActionPose.GetPoseWrtMarkerParentFailed",
                          "Could not get the pre-action pose w.r.t. the marker's parent.\n");
      }
      _poseWrtMarkerParent.SetName("PreActionPose");
    } // PreActionPose Constructor
    
    
    bool PreActionPose::GetCurrentPose(const Anki::Pose3d &markerParentPose, Anki::Pose3d &currentPose) const
    {
      bool validPoseFound = false;
      
      if(_marker->GetPose().GetParent() == &markerParentPose)
      {
        // Put the returned pose in the same frame as the marker's parent
        currentPose = markerParentPose * _poseWrtMarkerParent;
        currentPose.SetParent(markerParentPose.GetParent());
        
        // Check if currentPose is vertically oriented, and thus valid.
        // Get vector, v, from center of block to this point.
        Point3f v(currentPose.GetTranslation() - markerParentPose.GetTranslation());
        v.MakeUnitLength();
        const f32 DotProdTolerance = std::cos(DEG_TO_RAD(30));
        
        validPoseFound = NEAR(DotProduct(Z_AXIS_3D, v), 0.f, DotProdTolerance);
        
      } // if given pose is parent of the marker
      else {
        PRINT_NAMED_WARNING("PreActionPose.GetCurrentPose.InvalidParentPose",
                            "Given marker parent pose is not the parent of this PreActionPose's marker.\n");
      }
      
      return validPoseFound;
    } // GetCurrentPose()
    
#if 0
#pragma mark --- ActionableObject ---
#endif
    
    ActionableObject::ActionableObject()
    : _isBeingCarried(false)
    {
      
    }
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type, const Vision::KnownMarker *marker, f32 distance)
    {
      _preActionPoses.emplace_back(type, marker, distance);
    } // AddPreActionPose()
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type,
                                            const Vision::KnownMarker* marker,
                                            const Pose3d& poseWrtMarker)
    {
      _preActionPoses.emplace_back(type, marker, poseWrtMarker);
    }
    
    void ActionableObject::GetCurrentPreActionPoses(std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                                    const std::set<PreActionPose::ActionType>& withAction,
                                                    const std::set<Vision::Marker::Code>& withCode)
    {
      const Pose3d& atPose = GetPose();
      
      for(auto & preActionPose : _preActionPoses)
      {
        if((withCode.empty()   || withCode.count(preActionPose.GetMarker()->GetCode()) > 0) &&
           (withAction.empty() || withAction.count(preActionPose.GetActionType()) > 0))
        {
          Pose3d currentPose;
          const bool isValid = preActionPose.GetCurrentPose(atPose, currentPose);
          
          if(isValid) {
            poseMarkerPairs.emplace_back(currentPose, *preActionPose.GetMarker());
          }
        } // if preActionPose has correct code/action
      } // for each preActionPose
      
    } // GetCurrentPreActionPoses()
    
    void ActionableObject::Visualize()
    {
      if(IsSelected()) {
        static const ColorRGBA SELECTED_COLOR(0.f,1.f,0.f,1.f);
        Visualize(SELECTED_COLOR);
        VisualizeWithPreActionPoses();
      } else {
        Visualize(GetColor());
      }
    }
    
    // TODO: Rename to VisualizePreActionPoses()
    void ActionableObject::VisualizeWithPreActionPoses()
    {
      // Draw the main object:
      //Visualize();
      
      // Draw the pre-action poses, using a different color for each type of action
      u32 poseID = 0;
      std::vector<ActionableObject::PoseMarkerPair_t> poses;
      
      GetCurrentPreActionPoses(poses, {PreActionPose::DOCKING});
      for(auto & pose : poses) {
        _vizPreActionPoseHandles.emplace_back(VizManager::getInstance()->DrawPreDockPose(poseID,
                                                                                         pose.first.GetWithRespectToOrigin(),
                                                                                         PreActionPose::GetVisualizeColor(PreActionPose::DOCKING)));
        ++poseID;
      }
      
      poses.clear();
      GetCurrentPreActionPoses(poses, {PreActionPose::ENTRY});
      for(auto & pose : poses) {
        _vizPreActionPoseHandles.emplace_back(VizManager::getInstance()->DrawPreDockPose(poseID,
                                                                                         pose.first.GetWithRespectToOrigin(),
                                                                                         PreActionPose::GetVisualizeColor(PreActionPose::ENTRY)));
        ++poseID;
      }
    } // VisualizeWithPreActionPoses()
    
    
    void ActionableObject::EraseVisualization()
    {
      for(auto & preActionPoseHandle : _vizPreActionPoseHandles) {
        if(preActionPoseHandle != VizManager::INVALID_HANDLE) {
          VizManager::getInstance()->EraseVizObject(preActionPoseHandle);
        }
      }
      _vizPreActionPoseHandles.clear();
    }
    
#if 0
#pragma mark --- DockableObject ---
#endif
    
#if 0
    DockableObject::DockableObject()
    : _isBeingCarried(false)
    {
      
    }
    
    bool DockableObject::GetPreDockPose(const Point3f& canonicalPoint,
                                        const float distance_mm,
                                        Pose3d& preDockPose) const
    {
      bool dockingPointFound = false;
      
      // Compute this point's position at this distance according to this
      // block's current pose
      Point3f dockingPt(canonicalPoint);  // start with canonical point
      dockingPt *= distance_mm;           // scale to specified distance
      dockingPt =  GetPose() * dockingPt; // transform to block's pose
      
      //
      // Check if it's vertically oriented
      //
      const float DOT_TOLERANCE   = .35f;
      
      // Get vector, v, from center of block to this point
      Point3f v(dockingPt);
      v -= GetPose().GetTranslation();
      
      // Dot product of this vector with the z axis should be near zero
      // TODO: make dot product tolerance in terms of an angle?
      if( NEAR(DotProduct(Z_AXIS_3D, v), 0.f,  distance_mm * DOT_TOLERANCE) ) {
        
        /*
         // Rotation of block around v should be a multiple of 90 degrees
         const float ANGLE_TOLERANCE = DEG_TO_RAD(35);
         const float angX = ABS(blockPose.GetRotationAngle<'X'>().ToFloat());
         const float angY = ABS(blockPose.GetRotationAngle<'Y'>().ToFloat());
         
         const bool angX_mult90 = (NEAR(angX, 0.f,             ANGLE_TOLERANCE) ||
         NEAR(angX, DEG_TO_RAD(90),  ANGLE_TOLERANCE) ||
         NEAR(angX, DEG_TO_RAD(180), ANGLE_TOLERANCE) ||
         NEAR(angX, DEG_TO_RAD(360), ANGLE_TOLERANCE));
         const bool angY_mult90 = (NEAR(angY, 0.f,             ANGLE_TOLERANCE) ||
         NEAR(angY, DEG_TO_RAD(90),  ANGLE_TOLERANCE) ||
         NEAR(angY, DEG_TO_RAD(180), ANGLE_TOLERANCE) ||
         NEAR(angY, DEG_TO_RAD(360), ANGLE_TOLERANCE));
         
         if(angX_mult90 && angY_mult90)
         {
         dockingPt.z() = 0.f;  // Project to floor plane
         preDockPose.SetTranslation(dockingPt);
         preDockPose.SetRotation(atan2f(-v.y(), -v.x()), Z_AXIS_3D);
         dockingPointFound = true;
         }
         */
        
        // TODO: The commented out logic above appears not to be accounting
        // for rotation ambiguity of blocks. Just accept this is a valid dock pose for now.
        dockingPt.z() = 0.f;  // Project to floor plane
        preDockPose.SetTranslation(dockingPt);
        preDockPose.SetRotation(atan2f(-v.y(), -v.x()), Z_AXIS_3D);
        preDockPose.SetParent(GetPose().GetParent());
        dockingPointFound = true;
        
      }
      
      return dockingPointFound;
    }  // GetPreDockPose()
    
    
    void DockableObject::Visualize(const VIZ_COLOR_ID color, const f32 preDockPoseDistance)
    {
      // Draw the main object:
      Visualize(color);
      
      // Draw the pre-dock poses
      if(preDockPoseDistance > 0.f) {
        u32 poseID = 0;
        std::vector<DockableObject::PoseMarkerPair_t> poses;
        GetPreDockPoses(preDockPoseDistance, poses);
        for(auto pose : poses) {
          _vizPreDockPoseHandles.emplace_back(VizManager::getInstance()->DrawPreDockPose(poseID,
                                                                                         pose.first.GetWithRespectToOrigin(),
                                                                                         VIZ_COLOR_PREDOCKPOSE));
          ++poseID;
        }
      }
    }
    
    void DockableObject::EraseVisualization()
    {
      for(auto & preDockPoseHandle : _vizPreDockPoseHandles) {
        if(preDockPoseHandle != VizManager::INVALID_HANDLE) {
          VizManager::getInstance()->EraseVizObject(preDockPoseHandle);
        }
      }
      _vizPreDockPoseHandles.clear();
    }
#endif
    
  } // namespace Cozmo
} // namespace Anki