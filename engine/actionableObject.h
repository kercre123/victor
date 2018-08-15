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

#include "engine/cozmoObservableObject.h"

#include "engine/preActionPose.h"
#include "engine/viz/vizManager.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

namespace Anki {
  namespace Vector {
    
    // TODO: Move to separate file
    class ActiveLED
    {
    public:
      ActiveLED();
      
    private:
      ColorRGBA _currentColor;
      
    };
    
    class ActionableObject : public virtual ObservableObject // NOTE: Vector::ObservableObject, not Vision::
    {
    public:
      ActionableObject();
      
      // Return only those pre-action poses that are "valid" (See protected
      // IsPreActionPoseValid() method below.)
      // Optionally, you may filter based on ActionType and Marker Code as well.
      // Returns true if we had to generate preActionPoses, false if cached poses were used
      // return value is currently only used for unit tests
      bool GetCurrentPreActionPoses(std::vector<PreActionPose>& preActionPoses,
                                    const Pose3d& robotPose,
                                    const std::set<PreActionPose::ActionType>& withAction = std::set<PreActionPose::ActionType>(),
                                    const std::set<Vision::Marker::Code>& withCode = std::set<Vision::Marker::Code>(),
                                    const std::vector<std::pair<Quad2f,ObjectID> >& obstacles = std::vector<std::pair<Quad2f,ObjectID> >(),
                                    const Pose3d* reachableFromPose = nullptr,
                                    const f32 offset_mm = 0,
                                    bool visualize = false) const;
      
      // Draws just the pre-action poses. The reachableFrom pose (e.g. the
      // current pose of the robot) is passed along to GetCurrenPreActionsPoses()
      // (see above).
      void VisualizePreActionPoses(const std::vector<std::pair<Quad2f,ObjectID> >& obstacles = std::vector<std::pair<Quad2f,ObjectID> >(),
                                   const Pose3d* reachableFrom = nullptr) const;
      
      // Just erases pre-action poses (if any were drawn). Subclasses should
      // call this from their virtual EraseVisualization() implementations.
      virtual void EraseVisualization() const override;
      
      // For defining Active Objects (which are powered and have, e.g., LEDs they can flash)
      std::list<ActiveLED> const& GetLEDs() const { return _activeLEDs; }
      
    protected:
 
      // Only "valid" poses are returned by GetCurrenPreActionPoses
      // By default, allows any rotation around Z, but none around X/Y, meaning
      // the pose must be vertically-oriented to be "valid". ReachableFromPose
      // is not used by default. Derived classes can implement their own
      // specific checks, but note that reachableFromPose could be nullptr
      // (meaning it was unspecified).     
      virtual bool IsPreActionPoseValid(const PreActionPose& preActionPose,
                                        const Pose3d* reachableFromPose,
                                        const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const;
      
      // Generates all possible preAction poses of the given type
      virtual void GeneratePreActionPoses(const PreActionPose::ActionType type,
                                          std::vector<PreActionPose>& preActionPoses) const = 0;
      
      virtual void SetPose(const Pose3d& newPose, f32 fromDistance, PoseState newPoseState) override;

      // TODO: Define a method for adding LEDs to active objects
      //void AddActiveLED(const Pose3d& poseWrtObject);
      
    private:      
      mutable std::vector<VizManager::Handle_t> _vizPreActionPoseHandles;
      
      // Set of pathIDs for visualizing the preActionLines
      mutable std::set<u32> _vizPreActionLineIDs;
      
      std::list<ActiveLED> _activeLEDs;
      
      mutable std::array<std::vector<PreActionPose>, PreActionPose::ActionType::NONE> _cachedPreActionPoses;
      
    }; // class ActionableObject
    
  } // namespace Vector
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ACTIONABLE_OBJECT_H
