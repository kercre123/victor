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

#include "objectPoseConfirmer.h"

#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "util/console/consoleInterface.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

#define VERBOSE_DEBUG_PRINT 0
  
namespace {

// Minimum number of times we need to observe an object at a pose to switch from
// unknown to known or dirty
CONSOLE_VAR_RANGED(s32, kMinTimesToObserveObject, "PoseConfirmation", 2, 1,10);

// Minimum number of times not to observe an object marked dirty before marking its
// pose as unknkown.
CONSOLE_VAR_RANGED(s32, kMinTimesToNotObserveDirtyObject, "PoseConfirmation", 2, 1,10);

// Only localize to / identify active objects within this distance
CONSOLE_VAR_RANGED(f32, kMaxLocalizationDistance_mm, "PoseConfirmation", 250.f, 50.f, 1000.f);

// TODO: (Al) Remove once bryon fixes the timestamps
// Disable the object is still moving check for visual observation entries due to incorrect
// timestamps in object moved messages
CONSOLE_VAR(bool, kDisableStillMovingCheck, "PoseConfirmation", true);

}
  
ObjectPoseConfirmer::ObjectPoseConfirmer(Robot& robot)
: _robot(robot)
{
  
}
  
inline void ObjectPoseConfirmer::SetPoseHelper(ObservableObject* object, const Pose3d& newPose,
                                               f32 distance, PoseState newPoseState, const char* debugStr) const
{
  if(VERBOSE_DEBUG_PRINT)
  {
    PRINT_CH_DEBUG("PoseConfirmer", debugStr,
                   "ObjectID:%d at (%.1f,%.1f,%.1f) as %s",
                   object->GetID().GetValue(),
                   newPose.GetTranslation().x(),
                   newPose.GetTranslation().y(),
                   newPose.GetTranslation().z(),
                   EnumToString(newPoseState));
  }
  
  // Notify about the change we are about to make from old to new:
  _robot.GetBlockWorld().OnObjectPoseWillChange(object->GetID(), object->GetFamily(), newPose, newPoseState);
  _robot.GetLightsComponent().OnObjectPoseStateWillChange(object->GetID(), object->GetPoseState(), newPoseState);
  
  // if state changed from unknown to known or dirty
  if (object->IsPoseStateUnknown() && newPoseState != PoseState::Unknown){
    Util::sEventF("robot.object_seen", {{DDATA, TO_DDATA_STR(distance)}}, "%s", EnumToString(object->GetType()));
  }
  
  if(newPoseState == PoseState::Unknown)
  {
    MarkObjectUnknown(object);
    
    // TODO: ClearObject also calls MarkObjectUnknown(). Necessary to call twice? (COZMO-7128)
    _robot.GetBlockWorld().ClearObject(object);
  }
  
  object->SetPose(newPose, distance, newPoseState);
  
  _robot.GetBlockWorld().NotifyBlockConfigurationManagerObjectPoseChanged(object->GetID());
}

Result ObjectPoseConfirmer::MarkObjectUnknown(ObservableObject* object) const
{
  if( !object->IsPoseStateUnknown() )
  {
    SetPoseState(object, PoseState::Unknown);

    // Notify listeners if object is going fron !Unknown to Unknown
    using namespace ExternalInterface;
    _robot.Broadcast(MessageEngineToGame(RobotMarkedObjectPoseUnknown(_robot.GetID(), object->GetID().GetValue())));
  }
  
  return RESULT_OK;
}

void ObjectPoseConfirmer::SetPoseState(ObservableObject* object, PoseState newState) const
{
  _robot.GetLightsComponent().OnObjectPoseStateWillChange(object->GetID(), object->GetPoseState(), newState);
  
  object->SetPoseState(newState);
}


ObjectPoseConfirmer::PoseConfirmation::PoseConfirmation(const Pose3d& initPose, s32 initNumTimesObserved, TimeStamp_t initLastPoseUpdatedTime)
: lastPose(initPose)
, numTimesObserved(initNumTimesObserved)
, lastPoseUpdatedTime(initLastPoseUpdatedTime)
{
  
}

  
Result ObjectPoseConfirmer::AddVisualObservation(ObservableObject* object, const Pose3d& newPose,
                                                 bool robotWasMoving, f32 obsDistance_mm)
{
  ASSERT_NAMED(nullptr != object, "ObjectPoseConfirmer.AddVisualObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.AddVisualObservation.UnSetObjectID");

  auto iter = _poseConfirmations.find(objectID);
  if(iter == _poseConfirmations.end())
  {
    PRINT_CH_DEBUG("PoseConfirmer", "AddVisualObservation.NewEntry",
                   "ObjectID:%d at (%.1f,%.1f,%.1f), currently %s",
                   objectID.GetValue(),
                   newPose.GetTranslation().x(),
                   newPose.GetTranslation().y(),
                   newPose.GetTranslation().z(),
                   EnumToString(object->GetPoseState()));
    
    // Initialize new entry with one observation (this one)
    _poseConfirmations[objectID] = PoseConfirmation(newPose, 1, 0);
  
  }
  else
  {
    // Existing entry. Check if the new observation is (roughly) in the same pose as the last one.
    const Point3f distThreshold  = object->GetSameDistanceTolerance();
    const Radians angleThreshold = object->GetSameAngleTolerance();
    
    PoseConfirmation& poseConf = iter->second;
    
    const bool poseIsSame = newPose.IsSameAs(poseConf.lastPose, distThreshold, angleThreshold);
    
    if(poseIsSame)
    {
      poseConf.numTimesObserved++;
      
      if(poseConf.numTimesObserved >= kMinTimesToObserveObject)
      {

        // Makes the isMoving check more temporally accurate, but might not actually be helping that much.
        TimeStamp_t stoppedMovingTime;
        bool objectIsMoving = object->IsMoving(&stoppedMovingTime);
        if (!objectIsMoving) {
          if (stoppedMovingTime >= object->GetLastObservedTime() && !kDisableStillMovingCheck) {
            PRINT_CH_DEBUG("PoseConfirmer", "AddVisualObservation.ObjectIsStillMoving", "");
            objectIsMoving = true;
          }
        }
        
        const bool isRobotOnTreads = OffTreadsState::OnTreads == _robot.GetOffTreadsState();
        const bool isFarAway = Util::IsFltGT(obsDistance_mm,  kMaxLocalizationDistance_mm);
        const bool useDirty = !isRobotOnTreads || isFarAway || robotWasMoving || objectIsMoving;
        
        // Note that we never change a Known object to Dirty with a visual observation
        if(!useDirty || !object->IsPoseStateKnown())
        {
          const PoseState newPoseState = (useDirty ? PoseState::Dirty : PoseState::Known);
          SetPoseHelper(object, newPose, obsDistance_mm, newPoseState, "AddVisualObservation.Update");
          poseConf.lastPoseUpdatedTime = object->GetLastObservedTime();
        }
        
        // notify blockworld that we can confirm we have seen this object at its current pose. This allows blockworld
        // to reason about the markers seen this frame
        _robot.GetBlockWorld().OnObjectVisuallyVerified(object);
      }
    }
    else
    {
      // Seeing in different pose, reset counter
      poseConf.numTimesObserved = 1;
    }
    
    // These get set regardless of whether pose is same
    poseConf.lastPose = newPose;
    poseConf.numTimesUnobserved = 0;
  }
  
  return RESULT_OK;
}

  
Result ObjectPoseConfirmer::AddRobotRelativeObservation(ObservableObject* object, const Pose3d& poseRelToRobot, PoseState poseState)
{
  ASSERT_NAMED(nullptr != object, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.AddRobotRelativeObservation.UnSetObjectID");
  
  Pose3d poseWrtOrigin(poseRelToRobot);
  poseWrtOrigin.SetParent(&_robot.GetPose());
  poseWrtOrigin = poseWrtOrigin.GetWithRespectToOrigin();
  
  SetPoseHelper(object, poseWrtOrigin, -1.f, poseState, "AddRobotRelativeObservation");
  
  // numObservations is set to 1.
  // Basically, this requires that the object is observed a few times in a row without this method being called
  // in order for it to be localized to.
  _poseConfirmations[objectID] = PoseConfirmation(poseWrtOrigin, 1, object->GetLastObservedTime());
  
  return RESULT_OK;
}
  
  
Result ObjectPoseConfirmer::AddObjectRelativeObservation(ObservableObject* objectToUpdate, const Pose3d& newPose,
                                                         const ObservableObject* observedObject)
{
  ASSERT_NAMED(nullptr != objectToUpdate, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObjectToUpdate");
  ASSERT_NAMED(nullptr != observedObject, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObservedObject");
  
  const ObjectID& objectID = objectToUpdate->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.AddObjectRelativeObservation.UnSetObjectID");

  if(!observedObject->IsPoseStateUnknown())
  {
    const PoseState newPoseState = PoseState::Dirty; // do not inherit the pose state from the observed object
    SetPoseHelper(objectToUpdate, newPose, -1.f, newPoseState, "AddObjectRelativeObservation");
    _poseConfirmations[objectID].lastPoseUpdatedTime = observedObject->GetLastObservedTime();
  }
  
  _poseConfirmations[objectID].lastPose = newPose;
  
  return RESULT_OK;
}


Result ObjectPoseConfirmer::AddLiftRelativeObservation(ObservableObject* object, const Pose3d& newPoseWrtLift)
{
  ASSERT_NAMED(nullptr != object, "ObjectPoseConfirmer.AddLiftRelativeObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.AddLiftRelativeObservation.UnSetObjectID");
  
  // Sanity check
  ASSERT_NAMED(newPoseWrtLift.GetParent() == &_robot.GetLiftPose(),
               "ObjectPoseConfirmer.AddLiftRelativeObservation.PoseNotWrtLift");

  // If the object is on the lift, consider its pose as accurately known
  SetPoseHelper(object, newPoseWrtLift, -1, PoseState::Known, "AddLiftRelativeObservation");
  
  // numObservations is set to 1.
  // Basically, this requires that the object is observed a few times in a row without this method being called
  // in order for it to be localized to.
  _poseConfirmations[objectID] = PoseConfirmation(newPoseWrtLift, 1, object->GetLastObservedTime());

  
  return RESULT_OK;
}
  
  
Result ObjectPoseConfirmer::CopyWithNewPose(ObservableObject *newObject, const Pose3d &newPose,
                                            const ObservableObject *oldObject)
{
  ASSERT_NAMED(nullptr != newObject, "ObjectPoseConfirmer.CopyWithNewPose.NullNewObject");
  ASSERT_NAMED(nullptr != oldObject, "ObjectPoseConfirmer.CopyWithNewPose.NullOldObject");
  
  const ObjectID& objectID = newObject->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.CopyWithNewPose.UnSetObjectID");
  
  //SetPoseHelper(newObject, newPose, oldObject->GetLastPoseUpdateDistance(), oldObject->GetPoseState(), "CopyWithNewPose");
  newObject->SetPose(newPose, oldObject->GetLastPoseUpdateDistance(), oldObject->GetPoseState());
  
  // If IDs match, then this just updates the old object's pose (and leaves num observations
  // the same). If newObject has a different ID, then this will add an entry for it, leave observations
  // set to zero and set its pose.
  _poseConfirmations[objectID].lastPose = newPose;
  
  return RESULT_OK;
}
  

Result ObjectPoseConfirmer::AddInExistingPose(const ObservableObject* object)
{
  ASSERT_NAMED(nullptr != object, "ObjectPoseConfirmer.AddInExistingPose.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.AddInExistingPose.UnSetObjectID");
  
  _poseConfirmations[objectID].lastPose = object->GetPose();
  
  return RESULT_OK;
}
  
  
Result ObjectPoseConfirmer::MarkObjectUnobserved(ObservableObject* object)
{
  ASSERT_NAMED(nullptr != object, "ObjectPoseConfirmer.MarkObjectUnobserved.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  ASSERT_NAMED(objectID.IsSet(), "ObjectPoseConfirmer.MarkObjectUnobserved.UnSetObjectID");
  
  auto iter = _poseConfirmations.find(objectID);
  if(iter == _poseConfirmations.end())
  {
    PRINT_NAMED_ERROR("ObjectPoseConfirmer.MarkObjectUnobserved.ObjectNotFound",
                      "Object %d not previously added. Can't mark unobserved.",
                      objectID.GetValue());
    
    return RESULT_FAIL;
  }
  else
  {
    PoseConfirmation& poseConf = iter->second;
    poseConf.numTimesObserved = 0;
    poseConf.numTimesUnobserved++;
    
    if(poseConf.numTimesUnobserved >= kMinTimesToNotObserveDirtyObject)
    {
      PRINT_CH_INFO("PoseConfirmer", "MarkObjectUnobserved.MakingUnknown",
                    "ObjectID:%d unobserved %d times, marking Unknown",
                    objectID.GetValue(), poseConf.numTimesUnobserved);
      
      SetPoseHelper(object, object->GetPose(), -1.f, PoseState::Unknown, "MarkObjectUnknown");
    }
  }
  
  return RESULT_OK;
}
  
  
void ObjectPoseConfirmer::Clear()
{
  _poseConfirmations.clear();
}

  
TimeStamp_t ObjectPoseConfirmer::GetLastPoseUpdatedTime(const ObjectID& id) const
{
  auto poseConf = _poseConfirmations.find(id);
  if (poseConf != _poseConfirmations.end()) {
    return poseConf->second.lastPoseUpdatedTime;
  }
  
  return 0;
}
  
} // namespace Cozmo
} // namespace Anki
