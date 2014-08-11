/**
 * File: actionableObject.h
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
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASESTATION_ACTIONABLE_OBJECT_H
#define ANKI_COZMO_BASESTATION_ACTIONABLE_OBJECT_H

#include "anki/vision/basestation/observableObject.h"

#include "vizManager.h"

namespace Anki {
  namespace Cozmo {
    
    class PreActionPose
    {
    public:
      enum ActionType {
        DOCKING, // e.g. with objects to pick up
        ENTRY,   // e.g. for entering a bridge or ascending/descending a ramp
      };
      
      // Simple case: pose is along the normal to the marker, at the given distance
      PreActionPose(ActionType type,
                    const Vision::KnownMarker* marker,
                    f32 distance);
      
      // Specify arbitrary position relative to marker
      // poseWrtMarker's parent should be the marker's pose.
      PreActionPose(ActionType type,
                    const Vision::KnownMarker* marker,
                    const Pose3d& poseWrtMarker);
      
      // Get the type of action associated with this PreActionPose
      ActionType GetActionType() const;
      
      // Get marker associated with thise PreActionPose
      const Vision::KnownMarker* GetMarker() const;
      
      // Get the current PreActionPose, given the current pose of the
      // its marker's parent. Returns true if a valid pose is found (meaning
      // the pose is aligned with the ground plane), false otherwise.
      bool GetCurrentPose(const Pose3d& markerParentPose,
                          Pose3d& currentPose) const;
      
      
      // Get the Code of the Marker this PreActionPose is "attached" to.
      //const Vision::Marker::Code& GetMarkerCode() const;
      
      // Get PreActionPose w.r.t. the marker it is "attached" to. It is
      // the caller's responsibility to make it w.r.t. the world origin
      // (or other pose) if desired.
      //const Pose3d& GetPose() const; // w.r.t. marker!
      
      static const ColorRGBA& GetVisualizeColor(ActionType type);
      
    protected:
      
      ActionType _type;
      
      const Vision::KnownMarker* _marker;
      
      Pose3d _poseWrtMarkerParent;
      
    }; // class DockingPose
    
    
    inline PreActionPose::ActionType PreActionPose::GetActionType() const {
      return _type;
    }
    
    inline const Vision::KnownMarker* PreActionPose::GetMarker() const {
      return _marker;
    }
    
    
    
    class ActionableObject : public Vision::ObservableObject
    {
    public:
      ActionableObject();
      
      // Return true if actions poses of any type exist for this object
      bool HasPreActionPoses() const;
      
      // Return only those pre-action poses that are roughly aligned with the
      // ground plane, given their parent marker's current orientation.
      // Optionally, you may filter based on ActionType and Marker Code as well.
      using PoseMarkerPair_t = std::pair<Pose3d,const Vision::KnownMarker&>;
      void GetCurrentPreActionPoses(std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                    const std::set<PreActionPose::ActionType>& withAction = std::set<PreActionPose::ActionType>(),
                                    const std::set<Vision::Marker::Code>& withCode = std::set<Vision::Marker::Code>());
      
      // If the object is selected, draws it using the "selected" color, and
      // draws the pre-action poses as well (using method below).
      // Otherwise draws it in the object's defined color, with no pre-action poses.
      virtual void Visualize() override;
      virtual void Visualize(const ColorRGBA& color) = 0; 
      
      // Draws just the pre-action poses. Called automatically by Visualize()
      // when the object is marked as selected. (See methods above.)
      void VisualizePreActionPoses();
      
      // Just erases pre-action poses (if any were drawn). Subclasses should
      // call this from their virtual EraseVisualization() implementations.
      virtual void EraseVisualization() override;
      
      // Keep track of whether this object (has been docked with and) is being
      // carried. (Should this be here? Should we have an IsCarryable() virtual
      // method?)
      bool IsBeingCarried() const;
      void SetIsBeingCarried(const bool tf);
      
      // TODO: Possibly make this more descriptive to give finer-tuned control over states and visualization options.
      bool IsSelected() const;
      void SetSelected(const bool tf);
      
    protected:
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker* marker,
                            f32 distance);
      
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker* marker,
                            const Pose3d& poseWrtMarker);
      
    private:
      std::vector<PreActionPose> _preActionPoses;
      
      std::vector<VizManager::Handle_t> _vizPreActionPoseHandles;
      
      bool _isBeingCarried;
      bool _isSelected;
      
    }; // class ActionableObject
    
    
#if 0
#pragma mark --- Inline Implementations ---
#endif
    
    inline bool ActionableObject::HasPreActionPoses() const {
      return !_preActionPoses.empty();
    }
    
    inline bool ActionableObject::IsBeingCarried() const {
      return _isBeingCarried;
    }
    
    inline void ActionableObject::SetIsBeingCarried(const bool tf) {
      _isBeingCarried = tf;
    }
    
    inline bool ActionableObject::IsSelected() const {
      return _isSelected;
    }
    
    inline void ActionableObject::SetSelected(const bool tf) {
      _isSelected = tf;
    }
    
  
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ACTIONABLE_OBJECT_H