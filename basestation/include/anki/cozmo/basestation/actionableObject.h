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

#include "anki/cozmo/basestation/cozmoObservableObject.h"

#include "anki/cozmo/basestation/preActionPose.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

namespace Anki {
  namespace Cozmo {
    
    // TODO: Move to separate file
    class ActiveLED
    {
    public:
      ActiveLED();
      
    private:
      ColorRGBA _currentColor;
      
    };
    
    class ActionableObject : public ObservableObject // NOTE: Cozmo::ObservableObject, not Vision::
    {
    public:
      ActionableObject(ObjectFamily family, ObjectType type);
      
      // Return true if actions poses of any type exist for this object
      bool HasPreActionPoses() const;
      
      // Return only those pre-action poses that are "valid" (See protected
      // IsPreActionPoseValid() method below.)
      // Optionally, you may filter based on ActionType and Marker Code as well.
      void GetCurrentPreActionPoses(std::vector<PreActionPose>& preActionPoses,
                                    const std::set<PreActionPose::ActionType>& withAction = std::set<PreActionPose::ActionType>(),
                                    const std::set<Vision::Marker::Code>& withCode = std::set<Vision::Marker::Code>(),
                                    const std::vector<std::pair<Quad2f,ObjectID> >& obstacles = std::vector<std::pair<Quad2f,ObjectID> >(),
                                    const Pose3d* reachableFromPose = nullptr);
      
      // If the object is selected, draws it using the "selected" color.
      // Otherwise draws it in the object's defined color.
      virtual void Visualize() override;
      virtual void Visualize(const ColorRGBA& color) override = 0;
      
      // Draws just the pre-action poses. The reachableFrom pose (e.g. the
      // current pose of the robot) is passed along to GetCurrenPreActionsPoses()
      // (see above).
      void VisualizePreActionPoses(const std::vector<std::pair<Quad2f,ObjectID> >& obstacles = std::vector<std::pair<Quad2f,ObjectID> >(),
                                   const Pose3d* reachableFrom = nullptr);
      
      // Just erases pre-action poses (if any were drawn). Subclasses should
      // call this from their virtual EraseVisualization() implementations.
      virtual void EraseVisualization() override;
      
      // Keep track of whether this object (has been docked with and) is being
      // carried. (Should this be here? Should we have an IsCarryable() virtual
      // method?)
      bool IsBeingCarried() const;
      void SetBeingCarried(const bool tf);
      
      // TODO: Possibly make this more descriptive to give finer-tuned control over states and visualization options.
      bool IsSelected() const;
      void SetSelected(const bool tf);
      
      // For defining Active Objects (which are powered and have, e.g., LEDs they can flash)
      std::list<ActiveLED> const& GetLEDs() const { return _activeLEDs; }
      
    protected:

      // Wrappers for each of the PreActionPose constructors:
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker* marker,
                            const f32 distance);
      
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker *marker,
                            const Vec3f& offset);
      
      void AddPreActionPose(PreActionPose::ActionType type,
                            const Vision::KnownMarker* marker,
                            const Pose3d& poseWrtMarker);
 
      // Only "valid" poses are returned by GetCurrenPreActionPoses
      // By default, allows any rotation around Z, but none around X/Y, meaning
      // the pose must be vertically-oriented to be "valid". ReachableFromPose
      // is not used by default. Derived classes can implement their own
      // specific checks, but note that reachableFromPose could be nullptr
      // (meaning it was unspecified).
      virtual bool IsPreActionPoseValid(const PreActionPose& preActionPose,
                                        const Pose3d* reachableFromPose,
                                        const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const;
      
      // TODO: Define a method for adding LEDs to active objects
      //void AddActiveLED(const Pose3d& poseWrtObject);
      
    private:
      
      std::vector<PreActionPose> _preActionPoses;
      
      std::vector<VizManager::Handle_t> _vizPreActionPoseHandles;
      
      bool _isBeingCarried;
      bool _isSelected;
      
      std::list<ActiveLED> _activeLEDs;
      
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
    
    inline void ActionableObject::SetBeingCarried(const bool tf) {
      _isBeingCarried = tf;
      if(_isBeingCarried) {
        // Don't visualize pre-action poses for objects while they are being carried
        ActionableObject::EraseVisualization();
      }
    }
    
    inline bool ActionableObject::IsSelected() const {
      return _isSelected;
    }
    
    inline void ActionableObject::SetSelected(const bool tf) {
      _isSelected = tf;
      if(_isSelected == false) {
        // Don't draw pre-action poses if not selected
        ActionableObject::EraseVisualization();
      }
    }
    
  
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ACTIONABLE_OBJECT_H