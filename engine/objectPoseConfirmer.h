/**
 * File: objectPoseConfirmer.h
 *
 * Author:  Andrew Stein
 * Created: 08/10/2016
 *
 * Description: Object for setting the poses and pose states of observable objects
 *              based on type of observation and accumulated evidence. Note that this
 *              class is a friend of Cozmo::ObservableObject and can therefore call
 *              its SetPose method.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_ObjectPoseConfirmer_H__
#define __Anki_Cozmo_Basestation_ObjectPoseConfirmer_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"
#include "clad/types/objectFamilies.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"

namespace Anki {
namespace Cozmo {
  
// Forward decl.
class Robot;
class ObservableObject;
class Charger;
  
class ObjectPoseConfirmer : public IDependencyManagedComponent<RobotComponentID>
{
public:
  ObjectPoseConfirmer();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  // Search for an unconfirmed or confirmed object and pose match. Returns true if the object is already confirmed at
  // the observed pose (pose of the objSeen), false if the object is not confirmed there, either because it's not
  // confirmed, or because it's confirmed at a different pose or origin, or just connected but not located.
  // objectToCopyIDFrom: some instance that we have found for this object, and from which we can copy the objectID. No
  // other functionality is guaranteed from the object (for example whether it's confirmed, or even has a pose)
  bool IsObjectConfirmedAtObservedPose(const std::shared_ptr<ObservableObject>& objSeen,
                                       const ObservableObject*& objectToCopyIDFrom) const;
  
  // Saw object with camera from given distance at given time in history. Pose and ID are expected to be properly
  // set in the object passed in.
  // returns true if the object is confirmed or becomes confirmed, false if this observation is for an object
  // not confirmed, and it does not cause a confirmation
  bool AddVisualObservation(const std::shared_ptr<ObservableObject>& observation,
                            ObservableObject* confirmedMatch,
                            bool robotWasMoving,
                            f32 obsDistance_mm);
  
  // Immediately update object with a give pose relative to the robot, with the given PoseState.
  // Pose will be set w.r.t. origin (not "attached to" robot).
  Result AddRobotRelativeObservation(ObservableObject* object, const Pose3d& poseRelToRobot, PoseState poseState);
  
  // We have an "observation" of where the object should be, by virtue of the lift's pose.
  // Given pose's parent must be the robot's lift pose, or will return RESULT_FAIL.
  // Pose will not be confirmed (until we see it, or until such time that the robot can tell
  // it's carrying something)
  Result AddLiftRelativeObservation(ObservableObject* object, const Pose3d& newPoseWrtLift);
  
  // Saw one object and that observation influenced another object (e.g. stacked blocks).
  // Checks the blockWorld, and if the object doesn't exist, it could add it to the
  // blockWorld at this point. This is not supported because we don't have a use case, but it's
  // how AddRobotRelativeObservation works, for example to add a charger. Inherits observation time from the
  // observedObject, but does not inherit its poseState.
  Result AddObjectRelativeObservation(ObservableObject* objectToUpdate, const Pose3d& newPose,
                                      const ObservableObject* observedObject);

  // New object's pose will immediately be updated, but will inherit old object's
  // number of observations and pose state.
  Result CopyWithNewPose(ObservableObject* newObject, const Pose3d& newPose,
                         const ObservableObject* oldObject);
  
  // Simply adds the given object in its current pose to the PoseConfirmer's records,
  // without changing its pose or changing its observation count.
  Result AddInExistingPose(const ObservableObject* object);
  
  // Ghost objects are expected to not be in the BlockWorld and to not be added. They also do not notify of changes.
  // This should not be a PoseConfirmation event, however to keep a consistent API to access SetPose, we need to
  // provide it here. TODO: Design proper API, for example having BlockWorldPoseConfirmer vs BlockWorldGhostConfirmer,
  // which share code, but make this distinction clear.
  Result SetGhostObjectPose(ObservableObject* ghostObject, const Pose3d& newPose, PoseState newPoseState);
  
  // Notify listeners of the pose and poseState change happening for the given object. It should arguably not be in the
  // poseConfirmer, but for now it's a good place to put together these calls
  // This method internally calls BroadcastObjectPoseStateChanged if the poseState changes
  // Note that if the object has been destroyed from the BlockWorld, a copy of the object is passed as reference, in
  // which the pose is Invalid and not accessible.
  void BroadcastObjectPoseChanged(const ObservableObject& object, const Pose3d* oldPose, PoseState oldPoseState);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Pose State
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // These methods specifically deal with changes to PoseStates. It would be nice if they encapsulate what they
  // mean, rather than the PoseState that they set. For example, MarkObjectMoved would set as Dirty or Unreliable.
  // For now, there can be a 1 to 1 match while we come up with the concepts.

  // Increment the number of times the object has gone unobserved.
  // Once object is unobserved enough times, its pose state will be set to Unknown
  // and it will be deleted from BlockWorld.
  // Fails if this object wasn't previously added.
  // Important: if the object is deleted from the world, the passed in pointer is nulled out to prevent
  // using freed memory
  Result MarkObjectUnobserved(ObservableObject*& object);
  
  // Purposely mark the object as Unknown in the current origin, and notify listeners of the change.
  // In the current implementation this means the located instance will be deleted
  // @propagateStack: if set to true, objects on top of this one will also be marked. Note that this can be ok
  // from outside BlockWorld update ticks, but if two requests to change objects are made from different
  // systems, no conflict resolution is made, and they are processed in the way they are requested.
  // Note: See BlockWorld::UpdatePoseOfStackedObjects for conflict resolution inside BlockWorld
  // Note: It does not need to count how many times we set Unknown, 1 is enough to make the change. This is in
  // contrast with MarkObjectUnobserved, and they should be standarized so that MarkObjectX is the confirming change,
  // rather than an unconfirmed mark.
  // Note: This method takes a reference to a pointer, because the object will be deleted (Unknown objects
  // are not stored in BlockWorld anymore). After deletion, the pointer will be nulled out.
  void MarkObjectUnknown(ObservableObject*& object, bool propagateStack);

  // Purposely mark the object as Dirty and notify listeners of the change.
  // @propagateStack: if set to true, objects on top of this one will also be marked. Note that this can be ok
  // from outside BlockWorld update ticks, but if two requests to change objects are made from different
  // systems, no conflict resolution is made, and they are processed in the way they are requested.
  // Note: See BlockWorld::UpdatePoseOfStackedObjects for conflict resolution inside BlockWorld
  // Note: It does not need to count how many times we set Dirty, 1 is enough to make the change. This is in
  // contrast with MarkObjectUnobserved, and they should be standarized so that MarkObjectX is the confirming change,
  // rather than an unconfirmed mark.
  void MarkObjectDirty(ObservableObject* object, bool propagateStack);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Remove all last pose and num observation information for all object IDs,
  // e.g. when the robot delocalizes
  void Clear();
  
  // notification from BlockWorld that the given object has been deleted from the current origin, and hence it should
  // not be confirmed anymore in the PoseConfirmer
  void DeletedObjectInCurrentOrigin(const ObjectID& ID);
  
  // Get last time that the pose of the object was updated. Returns 0 if ID not found.
  TimeStamp_t GetLastPoseUpdatedTime(const ObjectID& ID) const;
  
  // Get last time that an object was matched visually, even if not yet confirmed/updated. Returns 0 if ID not found.
  TimeStamp_t GetLastVisuallyMatchedTime(const ObjectID& ID) const;
  
protected:
  
  struct PoseConfirmation
  {
    Pose3d referencePose;
    s32    numTimesObserved   = 0; // at this pose, using vision
    s32    numTimesUnobserved = 0;
    TimeStamp_t    lastPoseUpdatedTime = 0;
    TimeStamp_t    lastVisuallyMatchedTime = 0;
    
    // pointer to the object while it's not confirmed. Once it is confirmed in a pose, its ownership is passed
    // onto the blockworld, and this pointer is set to nullptr.
    std::shared_ptr<ObservableObject> unconfirmedObject;
    
    PoseConfirmation() { }
    
    // note this entry will gain ownership of the pointer passed in.
    // this constructor is used when we grab an unconfirmed object
    // initializes lastVisuallyMatchedTime with the observation's LastObservedTime
    PoseConfirmation(const std::shared_ptr<ObservableObject>& observation,
                     s32 initNumTimesObserved,
                     TimeStamp_t initLastPoseUpdatedTime);

    // update pose, resets numTimesObserved back to 1, leaves visuallyMatchedTime unchanged
    void UpdatePose(const Pose3d& newPose, TimeStamp_t poseUpdatedTime);

    // returns true if the reference pose is confirmed (by comparing observation number)
    bool IsReferencePoseConfirmed() const;
  };
  
  using PoseConfirmations = std::map<ObjectID, PoseConfirmation>;
  
  // Helper to share code between updating BlockWorld instances and unconfirmed instances
  void UpdatePoseInInstance(ObservableObject* object,
                            const ObservableObject* observation,
                            const ObservableObject* confirmedMatch,
                            const Pose3d& newPose,
                            bool robotWasMoving,
                            f32 obsDistance_mm);
  
  // Note: if the pose is set to PoseState::Invalid, the object can be destroyed from the BlockWorld. In that case,
  // the object pointer passed in is set to nullptr to prevent using the freed memory
  void SetPoseHelper(ObservableObject*& object, const Pose3d& newPose, f32 distance,
                     PoseState newPoseState, const char* debugStr);
  
  // Note this method doesn't accept Invalid poseStates, since it won't destroy the object
  void SetPoseStateHelper(ObservableObject* object, PoseState newState);
  
  // Notify listeners of the poseState only change happening for the given object. It should arguably not be in the
  // poseConfirmer, but for now it's a good place to put together these calls
  // Note this method is used when the intended change affects only the poseState, but not the Pose
  // Note that if the object has been destroyed from the BlockWorld, a copy of the object is passed as reference, in
  // which the pose is Invalid and not accessible.
  void BroadcastObjectPoseStateChanged(const ObservableObject& object, PoseState oldPoseState);
  
  void FindObjectMatchForObservation(const std::shared_ptr<ObservableObject>& objSeen,
                                     const ObservableObject*& objectToCopyIDFrom) const;
  
  Robot* _robot = nullptr;
  PoseConfirmations _poseConfirmations;
  
private:
  
  void DelocalizeRobotFromObject(const ObjectID& object) const;
  
  
}; // class ObjectPoseConfirmer
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_ObjectPoseConfirmer_H__
