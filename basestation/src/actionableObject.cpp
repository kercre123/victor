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