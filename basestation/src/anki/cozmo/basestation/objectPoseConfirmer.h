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
  
  // Saw object with camera from given distance at given time in history
  Result AddVisualObservation(ObservableObject* object, const Pose3d& newPose, bool robotWasMoving, f32 obsDistance_mm);
  
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
  
  // Immediately marks the object's pose state as Unknown -- but does NOT notify BlockWorld.
  // TODO: We probably want to call this in a lot of places instead of BlockWorld::ClearObject()
  //   but for now, just let ClearObject call this directly.
  Result MarkObjectUnknown(ObservableObject* object) const;
  
  // Remove all last pose and num observation information for all object IDs,
  // e.g. when the robot delocalizes
  void Clear();
  
  // Get last time that the pose of the object was updated
  TimeStamp_t GetLastPoseUpdatedTime(const ObjectID& id) const;
  
protected:
  
  struct PoseConfirmation
  {
    Pose3d lastPose;
    s32    numTimesObserved   = 0; // at this pose, using vision
    s32    numTimesUnobserved = 0;
    TimeStamp_t    lastPoseUpdatedTime = 0;
    
    PoseConfirmation() { }
    PoseConfirmation(const Pose3d& initPose, s32 initNumTimesObserved, TimeStamp_t initLastPoseUpdatedTime);
  };
  
  using PoseConfirmations = std::map<ObjectID, PoseConfirmation>;
  
  void SetPoseHelper(ObservableObject* object, const Pose3d& newPose, f32 distance,
                     PoseState newPoseState, const char* debugStr) const;
  
  Robot& _robot;
  PoseConfirmations _poseConfirmations;
  
  
}; // class ObjectPoseConfirmer
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_ObjectPoseConfirmer_H__
