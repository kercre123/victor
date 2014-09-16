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
        {DOCKING,      ColorRGBA(0.0f,0.0f,1.0f,0.5f)},
        {PLACEMENT,    ColorRGBA(0.0f,0.8f,0.2f,0.5f)},
        {ENTRY,        ColorRGBA(1.f,0.f,0.f,0.5f)}
      };
      
      static const ColorRGBA Default(1.0f,0.0f,0.0f,0.5f);
     
      auto iter = ColorLUT.find(type);
      if(iter == ColorLUT.end()) {
        PRINT_NAMED_WARNING("PreActionPose.GetVisualizationColor.ColorNotDefined",
                            "Color not defined for ActionType=%d. Returning default color.\n", type);
        return Default;
      } else {
        return iter->second;
      }
    }
    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 const f32 distance,
                                 const Radians& headAngle)
    : PreActionPose(type, marker, Y_AXIS_3D * -distance, headAngle)
    {
      
    } // PreActionPose Constructor

    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 const Vec3f& offset,
                                 const Radians& headAngle)
    : _type(type)
    , _marker(marker)
    , _poseWrtMarkerParent(M_PI_2, Z_AXIS_3D, offset, &marker->GetPose()) // init w.r.t. marker
    , _headAngle(headAngle)
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
                                 const Pose3d& poseWrtMarker,
                                 const Radians& headAngle)
    : _type(type)
    , _marker(marker)
    , _headAngle(headAngle)
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
    
    
    PreActionPose::PreActionPose(const PreActionPose& canonicalPose,
                                 const Pose3d& markerParentPose)
    : _type(canonicalPose.GetActionType())
    , _marker(canonicalPose.GetMarker())
    , _poseWrtMarkerParent(markerParentPose*canonicalPose._poseWrtMarkerParent)
    , _headAngle(canonicalPose.GetHeadAngle())
    {
      _poseWrtMarkerParent.SetParent(markerParentPose.GetParent());
      _poseWrtMarkerParent.SetName("PreActionPose");
    }
    
    bool PreActionPose::IsOrientedForAction(const Pose3d& markerParentPose) const
    {
      /*
      bool isCorrectlyOriented = true;
      
      switch(GetActionType())
      {
        case DOCKING:
        {
       */
          // Check if currentPose is vertically oriented, and thus valid.
          // Get unit vector, v, from parent object's origin to the current pose.
          Point3f v(_poseWrtMarkerParent.GetTranslation() - markerParentPose.GetTranslation());
          v.MakeUnitLength();
          const f32 DotProdTolerance = 1.f - std::cos(DEG_TO_RAD(30));
          
          // Make sure that vector is roughly orthogonal to Z axis
          const bool isCorrectlyOriented = NEAR(DotProduct(Z_AXIS_3D, v), 0.f, DotProdTolerance);
       /*   break;
        }
        case ENTRY:
        {
          // Entry is orientation agnostic (?)
          isCorrectlyOriented = true;
          break;
        }
        default:
          PRINT_NAMED_ERROR("PreActionPose.IsMarkerOrientedForAction.UnexpectedActionCase",
                            "Case for action %d not implemented.\n", GetActionType());
          assert(false);
      }
       */
      return isCorrectlyOriented;
    }
    
#if 0
    Result PreActionPose::GetCurrentPose(const Pose3d &markerParentPose, Pose3d &currentPose) const
    {
      Result result = RESULT_OK;
      
      if(_marker->GetPose().GetParent() == &markerParentPose)
      {
        // Put the returned pose in the same frame as the marker's parent
        currentPose = markerParentPose * _poseWrtMarkerParent;
        currentPose.SetParent(markerParentPose.GetParent());
        
      } // if given pose is parent of the marker
      else {
        PRINT_NAMED_WARNING("PreActionPose.GetCurrentPose.InvalidParentPose",
                            "Given marker parent pose is not the parent of this PreActionPose's marker.\n");
        result = RESULT_FAIL;
      }
      
      return result;
    } // GetCurrentPose()
#endif
    
    
#if 0
#pragma mark --- ActionableObject ---
#endif
    
    ActionableObject::ActionableObject()
    : _isBeingCarried(false)
    , _isSelected(false)
    {
      
    }
    
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type, const Vision::KnownMarker *marker,
                                            const f32 distance, const Radians& headAngle)
    {
      _preActionPoses.emplace_back(type, marker, distance, headAngle);
    } // AddPreActionPose()
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type, const Vision::KnownMarker *marker,
                                            const Vec3f& offset, const Radians& headAngle)
    {
      _preActionPoses.emplace_back(type, marker, offset, headAngle);
    } // AddPreActionPose()
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type,
                                            const Vision::KnownMarker* marker,
                                            const Pose3d& poseWrtMarker,
                                            const Radians& headAngle)
    {
      _preActionPoses.emplace_back(type, marker, poseWrtMarker, headAngle);
    }
    
    
    bool ActionableObject::IsPreActionPoseValid(const PreActionPose& preActionPose,
                                                const Pose3d* reachableFromPose) const
    {
      const Pose3d checkPose = preActionPose.GetPose().GetWithRespectToOrigin();
      
      bool isValid = true;
      
      if(reachableFromPose != nullptr) {
        // Pose should be at ground height or it's not reachable
        const f32 heightThresh = 15.f;
        isValid = NEAR(checkPose.GetTranslation().z(), reachableFromPose->GetWithRespectToOrigin().GetTranslation().z(), heightThresh);
      }
      
      if(isValid) {
        // Allow any rotation around Z, but none around X/Y, in order to keep
        // vertically-oriented poses
        const f32 vertAlignThresh = 1.f - std::cos(DEG_TO_RAD(30)); // TODO: tighten?
        isValid = NEAR(checkPose.GetRotationMatrix()(2,2), 1.f, vertAlignThresh);
      }
      
      return isValid;
      
    } // IsPreActionPoseValid()
    
    
    void ActionableObject::GetCurrentPreActionPoses(std::vector<PreActionPose>& preActionPoses,
                                                    const std::set<PreActionPose::ActionType>& withAction,
                                                    const std::set<Vision::Marker::Code>& withCode,
                                                    const Pose3d* reachableFromPose)
    {
      const Pose3d& relToObjectPose = GetPose();
      
      for(auto & preActionPose : _preActionPoses)
      {
        if((withCode.empty()   || withCode.count(preActionPose.GetMarker()->GetCode()) > 0) &&
           (withAction.empty() || withAction.count(preActionPose.GetActionType()) > 0))
        {
          PreActionPose currentPose(preActionPose, relToObjectPose);
          
          if(IsPreActionPoseValid(currentPose, reachableFromPose)) {
            preActionPoses.emplace_back(currentPose);
          }
        } // if preActionPose has correct code/action
      } // for each preActionPose
      
    } // GetCurrentPreActionPoses()
    
    
    void ActionableObject::Visualize()
    {
      if(IsSelected()) {
        static const ColorRGBA SELECTED_COLOR(0.f,1.f,0.f,1.f);
        Visualize(SELECTED_COLOR);
        //VisualizePreActionPoses();
      } else {
        Visualize(GetColor());
      }
    }
    
    void ActionableObject::VisualizePreActionPoses(const Pose3d* reachableFrom)
    {
      // Draw the main object:
      //Visualize();
      
      // Draw the pre-action poses, using a different color for each type of action
      u32 poseID = 0;
      std::vector<PreActionPose> poses;
      
      for(PreActionPose::ActionType actionType : {PreActionPose::DOCKING, PreActionPose::ENTRY})
      {
        GetCurrentPreActionPoses(poses, {actionType}, std::set<Vision::Marker::Code>(), reachableFrom);
        for(auto & pose : poses) {
          _vizPreActionPoseHandles.emplace_back(VizManager::getInstance()->DrawPreDockPose(poseID,
                                                                                           pose.GetPose().GetWithRespectToOrigin(),
                                                                                           PreActionPose::GetVisualizeColor(actionType)));
          ++poseID;
        }
        
        poses.clear();
      } // for each actionType
      
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