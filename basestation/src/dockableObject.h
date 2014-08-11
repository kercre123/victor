/**
 * File: dockableObject.h
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

#ifndef ANKI_COZMO_BASESTATION_DOCKABLE_OBJECT_H
#define ANKI_COZMO_BASESTATION_DOCKABLE_OBJECT_H

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
      
      // If the object is selected, draws it using the "selected" color.
      // Otherwise draws it in the object's defined color.
      virtual void Visualize() override;
      virtual void Visualize(const ColorRGBA& color) = 0; 
      
      // Calls object's (virtual) Visualize() function to draw the main object
      // and then draws pre-action poses as well.
      void VisualizeWithPreActionPoses();
      
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
    
    //////////

#if 0
    class DockableObject : public Vision::ObservableObject
    {
    public:
      
      DockableObject();
      
      // Get possible poses to start docking/tracking procedure. These will be
      // a point a given distance away from each vertical face that has the
      // specified code, in the direction orthogonal to that face.  The points
      // will be w.r.t. same parent as the object, with the Z coordinate at the
      // height of the center of the object. The poses will be paired with
      // references to the corresponding marker. Optionally, only poses/markers
      // with the specified code can be returned.
      using PoseMarkerPair_t = std::pair<Pose3d,const Vision::KnownMarker&>;
      virtual void GetPreDockPoses(const float distance_mm,
                                   std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                   const Vision::Marker::Code withCode = Vision::Marker::ANY_CODE) const = 0;
      
      // Keep track of whether this object (has been docked with and) is being
      // carried
      bool IsBeingCarried() const;
      void SetIsBeingCarried(const bool tf);

      // DockableObjects will draw their pre-dock poses
      virtual void Visualize() = 0;
      virtual void Visualize(VIZ_COLOR_ID color) = 0;
      virtual void Visualize(const VIZ_COLOR_ID color, const f32 preDockPoseDistance);
      virtual void EraseVisualization() override;
      virtual f32  GetDefaultPreDockDistance() const = 0;
      
    protected:
      
      bool _isBeingCarried;
      
      bool GetPreDockPose(const Point3f& canonicalPoint,
                          const float distance_mm,
                          Pose3d& preDockPose) const;
      
      std::vector<VizManager::Handle_t> _vizPreDockPoseHandles;
      
    }; // class DockableObject
    
    
    
    inline bool DockableObject::IsBeingCarried() const {
      return _isBeingCarried;
    }
    
    inline void DockableObject::SetIsBeingCarried(const bool tf) {
      _isBeingCarried = tf;
    }
#endif
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_DOCKABLE_OBJECT_H