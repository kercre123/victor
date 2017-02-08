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

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {
  
// Forward decl.
class Robot;
class ObservableObject;
class Charger;
  
class ObjectPoseConfirmer
{
public:
  
  ObjectPoseConfirmer(Robot& robot);
  
  // Search for an unconfirmed or confirmed object and pose match. Returns true if the object is already confirmed at
  // the observed pose (pose of the objSeen), false if the object is not confirmed there, either because it's not
  // confirmed, or because it's confirmed far from that pose.
  // objectToCopyIDFrom: some instance that we have found for this object, and from which we can copy the objectID. No
  // other functionality is guaranteed from the object (for example whether it's confirmed, or even has a pose)
  // TODO Andrew evaluate match of Unique vs Passive: COZMO-6171
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
  // Iff the _observedObject_ is confirmed, then objectToUpdate gets the newPose and
  // the same PoseState as the observedObject. Otherwise, objectToUpdate remains the same.
  Result AddObjectRelativeObservation(ObservableObject* objectToUpdate, const Pose3d& newPose,
                                      const ObservableObject* observedObject);
  
  // New object's pose will immediately be updated, but will inherit old object's
  // number of observations and pose state.
  Result CopyWithNewPose(ObservableObject* newObject, const Pose3d& newPose,
                         const ObservableObject* oldObject);
  
  // Simply adds the given object in its current pose to the PoseConfirmer's records,
  // without changing its pose or changing its observation count.
  Result AddInExistingPose(const ObservableObject* object);
  
  // Increment the number of times the object has gone unobserved.
  // Once object is unobserved enough times, its pose state will be set to Unknown
  // and it will be cleared from BlockWorld.
  // Fails if this object wasn't previously added.
  Result MarkObjectUnobserved(ObservableObject* object);

  // TODO change to SetX, where X is a posestate
  void SetPoseState(ObservableObject* object, PoseState newState) const;
  
  // Remove all last pose and num observation information for all object IDs,
  // e.g. when the robot delocalizes
  void Clear();
  
  // Get last time that the pose of the object was updated
  TimeStamp_t GetLastPoseUpdatedTime(const ObjectID& id) const;
  
protected:
  
  struct PoseConfirmation
  {
    Pose3d referencePose;
    s32    numTimesObserved   = 0; // at this pose, using vision
    s32    numTimesUnobserved = 0;
    TimeStamp_t    lastPoseUpdatedTime = 0;
    
    // returns true if the reference pose is confirmed (by comparing observation number)
    bool IsReferencePoseConfirmed() const;
    
    // pointer to the object while it's not confirmed. Once it is confirmed in a pose, its ownership is passed
    // onto the blockworld, and this pointer is set to nullptr.
    std::shared_ptr<ObservableObject> unconfirmedObject;
    
    PoseConfirmation() { }
    
    // note this entry will gain ownership of the pointer passed in.
    // this constructor is used when we grab an unconfirmed object
    PoseConfirmation(const std::shared_ptr<ObservableObject>& observation,
                     s32 initNumTimesObserved,
                     TimeStamp_t initLastPoseUpdatedTime);

    // note this entry will gain ownership of the pointer passed in.
    // this constructor is used when we confirm an object in the first observation
    PoseConfirmation(const Pose3d& initPose,
                     s32 initNumTimesObserved,
                     TimeStamp_t initLastPoseUpdatedTime);
    
  };
  
  using PoseConfirmations = std::map<ObjectID, PoseConfirmation>;
  
  void UpdatePoseInInstance(ObservableObject* object,
                            const ObservableObject* observation,
                            const ObservableObject* confirmedMatch,
                            const Pose3d& newPose,
                            bool robotWasMoving,
                            f32 obsDistance_mm);
  
  void SetPoseHelper(ObservableObject* object, const Pose3d& newPose, f32 distance,
                     PoseState newPoseState, const char* debugStr) const;
  
  void FindObjectMatchForObservation(const std::shared_ptr<ObservableObject>& objSeen,
                                     const ObservableObject*& objectToCopyIDFrom) const;
  
  Robot& _robot;
  PoseConfirmations _poseConfirmations;
  
  
}; // class ObjectPoseConfirmer
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_ObjectPoseConfirmer_H__
