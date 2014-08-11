/**
 * File: actionableObject.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines an "Actionable" Object, which is a subclass of an
 *              ObservableObject that can also be interacted with or acted upon.
 *              It extends the (coretech) ObservableObject to have a notion of
 *              docking and entry points, for example, useful for Cozmo's.
 *              These are represented by different types of "pre-action" poses.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "actionableObject.h"

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
    , _marker(marker)
    , _poseWrtMarkerParent(M_PI_2, Z_AXIS_3D, Y_AXIS_3D * -distance, &marker->GetPose()) // init w.r.t. marker
    {      
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
        VisualizePreActionPoses();
      } else {
        Visualize(GetColor());
      }
    }
    
    void ActionableObject::VisualizePreActionPoses()
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
    
    
  } // namespace Cozmo
} // namespace Anki