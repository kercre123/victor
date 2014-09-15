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
        DOCKING,       // e.g. with objects to pick up with lifter down
        PLACEMENT,     // e.g. for putting a carried object down
        ENTRY,         // e.g. for entering a bridge or ascending/descending a ramp
      };
      
      // Simple case: pose is along the normal to the marker, at the given distance
      // (Aligned with center of marker)
      PreActionPose(ActionType type,
                    const Vision::KnownMarker* marker,
                    const f32 distance,
                    const Radians& headAngle);
      
      // Pose is aligned with normal (facing the marker), but offset by the given
      // vector. Note that a shift along the negative Y axis is equivalent to
      // the simple case above. (The marker is in the X-Z plane.
      PreActionPose(ActionType type,
                    const Vision::KnownMarker* marker,
                    const Vec3f& offset,
                    const Radians& headAngle);
      
      // Specify arbitrary position relative to marker
      // poseWrtMarker's parent should be the marker's pose.
      PreActionPose(ActionType type,
                    const Vision::KnownMarker* marker,
                    const Pose3d&  poseWrtMarker,
                    const Radians& headAngle);
      
      // For creating a pre-action pose in its current position, given the
      // canonical pre-action pose and the currennt pose of its marker's
      // parent. Probably not generally useful, but used by ActionableObject.
      PreActionPose(const PreActionPose& canonicalPose,
                    const Pose3d& markerParentPose);
      
      // Get the type of action associated with this PreActionPose
      ActionType GetActionType() const;
      
      // Get marker associated with thise PreActionPose
      const Vision::KnownMarker* GetMarker() const;
      
      // Get the current PreActionPose, given the current pose of the
      // its marker's parent.
      //Result GetCurrentPose(const Pose3d& markerParentPose,
      //                    Pose3d& currentPose) const;
      
      // Get the head angle associated with this pre-action pair
      const Radians& GetHeadAngle() const;
      
      // Returns true if the marker is correctly oriented for its action type.
      // For example, DOCKING poses must have the marker vertical to be docked with.
      bool IsOrientedForAction(const Pose3d& markerParentPose) const;
      
      // Get the Code of the Marker this PreActionPose is "attached" to.
      //const Vision::Marker::Code& GetMarkerCode() const;
      
      // Get PreActionPose w.r.t. the parent of marker it is "attached" to. It is
      // the caller's responsibility to make it w.r.t. the world origin
      // (or other pose) if desired.
      const Pose3d& GetPose() const; // w.r.t. marker's parent!
      
      static const ColorRGBA& GetVisualizeColor(ActionType type);
      
    protected:
      
      ActionType   _type;
      
      const Vision::KnownMarker* _marker;
      
      Pose3d _poseWrtMarkerParent;
      
      Radians _headAngle;
      
    }; // class PreActionPose
    
    
    inline PreActionPose::ActionType PreActionPose::GetActionType() const {
      return _type;
    }
    
    inline const Vision::KnownMarker* PreActionPose::GetMarker() const {
      return _marker;
    }
    
    inline const Radians& PreActionPose::GetHeadAngle() const {
      return _headAngle;
    }
    
    inline const Pose3d& PreActionPose::GetPose() const {
      return _poseWrtMarkerParent;
    }
    
    
    class ActionableObject : public Vision::ObservableObject
    {
    public:
      ActionableObject();
      
      // Return true if actions poses of any type exist for this object
      bool HasPreActionPoses() const;
      
      // Return only those pre-action poses that are "valid" (See protected
      // IsPreActionPoseValid() method below.)
      // Optionally, you may filter based on ActionType and Marker Code as well.
      void GetCurrentPreActionPoses(std::vector<PreActionPose>& preActionPoses,
                                    const std::set<PreActionPose::ActionType>& withAction = std::set<PreActionPose::ActionType>(),
                                    const std::set<Vision::Marker::Code>& withCode = std::set<Vision::Marker::Code>(),
                                    const Pose3d* reachableFromPose = nullptr);
      
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
                            const f32 distance,
                            const Radians& headAngle);
      
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker *marker,
                            const Vec3f& offset,
                            const Radians& headAngle);
      
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker* marker,
                            const Pose3d& poseWrtMarker,
                            const Radians& headAngle);
 
      // Only "valid" poses are returned by GetCurrenPreActionPoses
      // By default, allows any rotation around Z, but none around X/Y, meaning
      // the pose must be vertically-oriented to be "valid". ReachableFromPose
      // is not used by default. Derived classes can implement their own
      // specific checks, but note that reachableFromPose could be nullptr
      // (meaning it was unspecified).
      virtual bool IsPreActionPoseValid(const PreActionPose& preActionPose,
                                        const Pose3d* reachableFromPose) const;
      
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