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

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::UpdatePoseInInstance(ObservableObject* object,
                           const ObservableObject* observation,
                           const ObservableObject* confirmedMatch,
                           const Pose3d& newPose,
                           bool robotWasMoving,
                           f32 obsDistance_mm)
{
  // sometimes we don't want to override a previous pose state with a newer one if it's less accurate. Check that here:
  bool forceUpdate = false;

  if ( nullptr != confirmedMatch ) {
    const Point3f distThreshold  = object->GetSameDistanceTolerance();
    const Radians angleThreshold = object->GetSameAngleTolerance();
    const bool poseIsSame = newPose.IsSameAs(confirmedMatch->GetPose(), distThreshold, angleThreshold);
    
    // if we are confirming in a new pose that is far away from the current one, then we can no longer trust
    // our current pose state. In that case, we always override
    forceUpdate = poseIsSame;
  }
  
  // now calculate what posestate we should set and if that poseState overrides the current one (we still need
  // to calculate which one we are going to set, even if forceUpdate is already true)
  
  // Makes the isMoving check more temporally accurate, but might not actually be helping that much.
  // In order to ask for moved status, we need to query either the connected instance or the confirmedMatch (if
  // they exist)
  const ObjectID& objectID = object->GetID();
  const ActiveObject* const connectedMatch = _robot.GetBlockWorld().GetConnectedActiveObjectByID( objectID );
  TimeStamp_t stoppedMovingTime = 0;
  // ask both instances
  bool objectIsMoving = false;
  if (connectedMatch) {
    objectIsMoving = connectedMatch->IsMoving(&stoppedMovingTime);
  } else if (confirmedMatch) {
    objectIsMoving = confirmedMatch->IsMoving(&stoppedMovingTime);
  }
  if (!objectIsMoving)
  {
    if (stoppedMovingTime >= observation->GetLastObservedTime() && !kDisableStillMovingCheck) {
      PRINT_CH_DEBUG("PoseConfirmer", "ObjectPoseConfirmer.AddVisualObservation.ObjectIsStillMoving", "");
      objectIsMoving = true;
    }
  }

  const bool isRobotOnTreads = OffTreadsState::OnTreads == _robot.GetOffTreadsState();
  const bool isFarAway  = Util::IsFltGT(obsDistance_mm,  kMaxLocalizationDistance_mm);
  const bool setAsKnown = isRobotOnTreads && !isFarAway && !robotWasMoving && !objectIsMoving;

  const bool updatePose = forceUpdate || setAsKnown || !object->IsPoseStateKnown();
  if( updatePose )
  {
    const PoseState newPoseState = (setAsKnown ? PoseState::Known : PoseState::Dirty);
    SetPoseHelper(object, newPose, obsDistance_mm, newPoseState, "ObjectPoseConfirmer.UpdatePoseInInstace");
    
    // udpate the timestamp at which we are actually setting the pose
    DEV_ASSERT(_poseConfirmations.find(objectID) != _poseConfirmations.end(),
                "ObjectPoseConfirmer.UpdatePoseInInstace.EntryShouldExist");
    PoseConfirmation& poseConf = _poseConfirmations[objectID];
    poseConf.lastPoseUpdatedTime = observation->GetLastObservedTime();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  _robot.GetCubeLightComponent().OnObjectPoseStateWillChange(object->GetID(), object->GetPoseState(), newPoseState);
  
  // if state changed from unknown to known or dirty
  if (object->IsPoseStateUnknown() && newPoseState != PoseState::Unknown) {
    Util::sEventF("robot.object_seen",
                  {{DDATA, std::to_string(distance).c_str()}},
                  "%s",
                  EnumToString(object->GetType()));
  }
  
  // copy objectID since we can destroy the object
  const ObjectID objectID = object->GetID();
  
  if(newPoseState == PoseState::Unknown)
  {
    MarkObjectUnknown(object);
    
    
    // // TODO: ClearObject also calls MarkObjectUnknown(). Necessary to call twice? (COZMO-7128)
    // _robot.GetBlockWorld().ClearObject(object);
    _robot.GetBlockWorld().DeleteLocatedObjectByIDInCurOrigin(object->GetID());
    object = nullptr; // do not use anymore, since it's deleted
  }
  else
  {
    object->SetPose(newPose, distance, newPoseState);
  }
  
  _robot.GetBlockWorld().NotifyBlockConfigurationManagerObjectPoseChanged(objectID);
}

// TODO if this destroys the object, receive a reference to the pointer so that it gets nulled out, to
// detect when we are using it afterwards
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
  _robot.GetCubeLightComponent().OnObjectPoseStateWillChange(object->GetID(), object->GetPoseState(), newState);
  
  object->SetPoseState(newState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectPoseConfirmer::PoseConfirmation::PoseConfirmation(const std::shared_ptr<ObservableObject>& object,
                                                        s32 initNumTimesObserved,
                                                        TimeStamp_t initLastPoseUpdatedTime)
: referencePose(object->GetPose())
, numTimesObserved(initNumTimesObserved)
, lastPoseUpdatedTime(initLastPoseUpdatedTime)
, unconfirmedObject(object)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectPoseConfirmer::PoseConfirmation::PoseConfirmation(const Pose3d& initPose,
                                                        s32 initNumTimesObserved,
                                                        TimeStamp_t initLastPoseUpdatedTime)
: referencePose(initPose)
, numTimesObserved(initNumTimesObserved)
, lastPoseUpdatedTime(initLastPoseUpdatedTime)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectPoseConfirmer::IsObjectConfirmedInCurrentOrigin(const std::shared_ptr<ObservableObject>& objSeen,
                                                           const ObservableObject*& outMatchInOrigin,
                                                           const ObservableObject*& outMatchInOtherOrigin) const
{
  outMatchInOrigin = nullptr;
  outMatchInOtherOrigin = nullptr;

  BlockWorldFilter filter;
  filter.AddAllowedFamily(objSeen->GetFamily());
  filter.AddAllowedType(objSeen->GetType());

  // find confirmed object or unconfirmed
  if ( objSeen->IsActive() )
  {
    // ask blockworld to find matches by type/family in the current origin, since we assume only one instance per type
    // filter.AddFilterFcn(&BlockWorldFilter::ActiveObjectsFilter); // Should not be needed because of family/type
    std::vector<ObservableObject*> confirmedMatches;
    _robot.GetBlockWorld().FindLocatedMatchingObjects(filter, confirmedMatches);
    
    // should match 0 or 1
    DEV_ASSERT( confirmedMatches.size() <= 1, "ObjectPoseConfirmer.IsObjectConfirmed.TooManyMatches" );
    
    // if there is a match, return that one
    if ( confirmedMatches.size() == 1 )
    {
      outMatchInOrigin = confirmedMatches.front();
      outMatchInOtherOrigin = nullptr;
      return true;
    }
    else
    {
      // did not find a confirmed match, is there an unconfirmed match by family and type?
      for( const auto& confirmationInfoPair : _poseConfirmations )
      {
        const PoseConfirmation& confirmationInfo = confirmationInfoPair.second;
        // check only entries that have an unconfirmed object
        if ( confirmationInfo.unconfirmedObject )
        {
          // compare family and type
          const bool matchOk = (confirmationInfo.unconfirmedObject->GetFamily() == objSeen->GetFamily()) &&
                               (confirmationInfo.unconfirmedObject->GetType()   == objSeen->GetType()  );
          if ( matchOk ) {
            DEV_ASSERT(confirmationInfo.unconfirmedObject->GetID() == confirmationInfoPair.first,
                       "ObjectPoseConfirmer.IsObjectConfirmed.KeyNotMatchingID");
            outMatchInOrigin = confirmationInfo.unconfirmedObject.get();
            outMatchInOtherOrigin = nullptr;
            return false;
          }
        }
      }
      
      // did not find an unconfirmed entry
      outMatchInOrigin = nullptr;
      
      // search for this object in other frames
      filter.SetOriginMode(BlockWorldFilter::OriginMode::NotInRobotFrame);
      outMatchInOtherOrigin = _robot.GetBlockWorld().FindLocatedMatchingObject(filter);
      if ( nullptr == outMatchInOtherOrigin ) {
        // note originMode means nothing for connected objects. However clear Unknown filter set by default
        // TODO remove when filter is not default
        filter.SetFilterFcn(nullptr);
        outMatchInOtherOrigin = _robot.GetBlockWorld().FindConnectedActiveMatchingObject(filter);
      }
      
      // not confirmed in origin
      return false;
    }
    
  }
  else
  {
    // For passive objects, match based on pose (considering only objects in current frame)
    // Ignore objects we're carrying
    const ObjectID& carryingObjectID = _robot.GetCarryingObject();
    filter.AddFilterFcn([&carryingObjectID](const ObservableObject* candidate) {
      const bool isObjectBeingCarried = (candidate->GetID() == carryingObjectID);
      return !isObjectBeingCarried;
    });
    
    ObservableObject* matchingObject = _robot.GetBlockWorld().FindLocatedClosestMatchingObject(*objSeen,
                                                                objSeen->GetSameDistanceTolerance(),
                                                                objSeen->GetSameAngleTolerance(),
                                                                filter);
    // depending on whether we found a match
    if ( nullptr != matchingObject )
    {
      // we did
      outMatchInOrigin = matchingObject;
      outMatchInOtherOrigin = nullptr;
      return true;
    }
    else
    {
      // we didn't find a confirmed match in current origin
      const ObservableObject* closestObject = nullptr;
      Vec3f closestDist(objSeen->GetSameDistanceTolerance());
      Radians closestAngle(objSeen->GetSameAngleTolerance());

      // no confirmed match, look in unconfirmed
      for( const auto& confirmationInfoPair : _poseConfirmations )
      {
        const PoseConfirmation& confirmationInfo = confirmationInfoPair.second;
        // check only entries that have an unconfirmed object
        if ( confirmationInfo.unconfirmedObject )
        {
          // should not be possible to carry unconfirmed objects
          DEV_ASSERT( carryingObjectID != confirmationInfo.unconfirmedObject->GetID(),
                      "ObjectPoseConfirmer.IsObjectConfirmed.CarryingUnconfirmed");
          // unconfirmed objects are not applied the filter, check for family/type now
          const bool isSameType = (confirmationInfo.unconfirmedObject->GetFamily() == objSeen->GetFamily()) &&
                                  (confirmationInfo.unconfirmedObject->GetType()   == objSeen->GetType()  );
          if ( isSameType )
          {
            // compare location
            Vec3f Tdiff;
            Radians angleDiff;
            const bool closerMatch = (confirmationInfo.unconfirmedObject->IsSameAs(*objSeen.get(), closestDist, closestAngle, Tdiff, angleDiff));
            if ( closerMatch ) {
              closestDist = Tdiff.GetAbs();
              closestAngle = angleDiff.getAbsoluteVal();
              closestObject = confirmationInfo.unconfirmedObject.get();
            }
          }
        }
      }
      
      if ( nullptr != closestObject )
      {
        // found a close match
        outMatchInOrigin = closestObject;
        outMatchInOtherOrigin = nullptr;
        return true;
      }
      else
      {
        // did not find an unconfirmed entry
        outMatchInOrigin = nullptr;
        outMatchInOtherOrigin = nullptr;
        return false;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectPoseConfirmer::AddVisualObservation(const std::shared_ptr<ObservableObject>& observation,
                                               ObservableObject* confirmedMatch,
                                               bool robotWasMoving,
                                               f32 obsDistance_mm)
{
  DEV_ASSERT(observation, "ObjectPoseConfirmer.AddVisualObservation.NullObject");
  
  const ObjectID& objectID = observation->GetID();
  const Pose3d& newPose = observation->GetPose();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.AddVisualObservation.UnSetObjectID");

  auto iter = _poseConfirmations.find(objectID);
  if(iter == _poseConfirmations.end())
  {
    PRINT_CH_DEBUG("PoseConfirmer", "AddVisualObservation.NewEntry",
                   "ObjectID:%d at (%.1f,%.1f,%.1f), currently %s",
                   objectID.GetValue(),
                   newPose.GetTranslation().x(),
                   newPose.GetTranslation().y(),
                   newPose.GetTranslation().z(),
                   EnumToString(observation->GetPoseState()));
    
    // Initialize new entry with one observation (this one)
    _poseConfirmations[objectID] = PoseConfirmation(observation, 1, 0);
  }
  else
  {
    /*
      Note: Consider this scenario
      1) see object in pose A
      ... (n observations)
      2) see object in pose A -> confirm, pass unconfirmed object to confirmed
      3) see object in pose B -> this is the first observation on a given pose, should this have an unconfirmed pointer too?
      
      If not, then it should be impossible to Delete the object from the blockWorld. Otherwise we can have
      a nullptr here for unconfirmed, when the object has been deleted from blockWorld, so this entry would have
      neither unconfirmed nor confirmed object counterpart.
    */
  
    // Existing entry
    PoseConfirmation& poseConf = iter->second;
    ObservableObject* unconfirmedObjectPtr = poseConf.unconfirmedObject.get();
    
    // Confirmed and unconfirmed have to be mutually exclusive. If we bring an object from a different origin
    // during rejigeering, it should release at that point the unconfirmed instance (not the reference pose or
    // the count, but the instance itself)
    DEV_ASSERT( (nullptr == confirmedMatch) != (nullptr == unconfirmedObjectPtr),
                "ObjectPoseConfirmer.AddVisualObservation.ConfirmedAndUnconfirmedPointers" );
  
    // Check if the new observation is (roughly) in the same pose as the last one.
    const Point3f distThreshold  = observation->GetSameDistanceTolerance();
    const Radians angleThreshold = observation->GetSameAngleTolerance();
    const bool poseIsSame = newPose.IsSameAs(poseConf.referencePose, distThreshold, angleThreshold);
    
    if(poseIsSame)
    {
      ++poseConf.numTimesObserved;
      
      const bool confirmingObjectAtObservedPose = poseConf.numTimesObserved >= kMinTimesToObserveObject;
      if(confirmingObjectAtObservedPose)
      {
        const bool isCurrentlyUnconfirmed = (nullptr != unconfirmedObjectPtr);
        if( isCurrentlyUnconfirmed )
        {
          // udpate pose in unconfirmed instance before passing onto world
          UpdatePoseInInstance(unconfirmedObjectPtr, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
        
          // This is first confirmation, we have to give blockWorld ownership of the unconfirmed object
          _robot.GetBlockWorld().AddLocatedObject(poseConf.unconfirmedObject);
          poseConf.unconfirmedObject.reset(); // give up ownership of the pointer (the pointer is still valid)
          
          // flip pointers now that it is confirmed
          confirmedMatch = unconfirmedObjectPtr;
          unconfirmedObjectPtr = nullptr;
        }
        else
        {
          // We need to update the pose because we have proof that it's in a different location. Even if the
          // new location's PoseState is less accurate that the one we have at the moment, we need to update it.
          // KnownIssue: we do not store unconfirmed poses for objects that are already confirmed. However we do not
          // update their pose until they become confirmed. This means that only the last pose and pose state
          // are actually considered. In a scenario where we have: Known, Known, Dirty, Dirty observations, where we
          // need 4 observations to confirm, the last one (Dirty) is the one that we would set here. Note this is not
          // a problem before the first confirmation, since the Unconfirmed pointer updates continuously before
          // confirmation, and stores the most accurate. Search below (in this function) for KnownIssueLostAccuracy
          UpdatePoseInInstance(confirmedMatch, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
        }
        
        // notify blockworld that we can confirm we have seen this object at its current pose. This allows blockworld
        // to reason about the markers seen this frame
        _robot.GetBlockWorld().OnObjectVisuallyVerified(confirmedMatch);
      }
      else
      {
        // Update unconfirmed object's pose if it exists (if the object is already confirmed, then it doesn't)
        if ( unconfirmedObjectPtr ) {
          UpdatePoseInInstance(unconfirmedObjectPtr, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
        }
        // else KnownIssueLostAccuracy
      }
    }
    else
    {
      // Seeing in different pose, reset counter
      poseConf.numTimesObserved = 1;
      poseConf.referencePose = newPose;
      
      // Update unconfirmed object's pose if we have one
      if ( unconfirmedObjectPtr ) {
        UpdatePoseInInstance(unconfirmedObjectPtr, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
      }
      // else KnownIssueLostAccuracy
    }
    
    // These get set regardless of whether pose is same
    poseConf.numTimesUnobserved = 0;
  }
  
  // return whether the object is confirmed (could return true only if the observation contributed to it, but at the
  // moment either one is fine, and this is easier to understand)
  const bool isConfirmed = (nullptr == _poseConfirmations[objectID].unconfirmedObject);
  return isConfirmed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddRobotRelativeObservation(ObservableObject* object, const Pose3d& poseRelToRobot, PoseState poseState)
{
  DEV_ASSERT(nullptr != object, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.AddRobotRelativeObservation.UnSetObjectID");
  
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddObjectRelativeObservation(ObservableObject* objectToUpdate, const Pose3d& newPose,
                                                         const ObservableObject* observedObject)
{
  DEV_ASSERT(nullptr != objectToUpdate, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObjectToUpdate");
  DEV_ASSERT(nullptr != observedObject, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObservedObject");
  
  const ObjectID& objectID = objectToUpdate->GetID();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.AddObjectRelativeObservation.UnSetObjectID");
  
  // the object to update should have an entry in poseConfirmations, otherwise how did it become an
  // object that can be grabbed to add a relative observation?
  DEV_ASSERT(_poseConfirmations.find(objectID) != _poseConfirmations.end(),
             "ObjectPoseConfirmer.AddObjectRelativeObservation.NoPreviousObservationsForObjectToUpdate");

  if(!observedObject->IsPoseStateUnknown())
  {
    const PoseState newPoseState = PoseState::Dirty; // do not inherit the pose state from the observed object
    SetPoseHelper(objectToUpdate, newPose, -1.f, newPoseState, "AddObjectRelativeObservation");
    _poseConfirmations[objectID].lastPoseUpdatedTime = observedObject->GetLastObservedTime();
  }
  
  _poseConfirmations[objectID].referencePose = newPose;
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddLiftRelativeObservation(ObservableObject* object, const Pose3d& newPoseWrtLift)
{
  DEV_ASSERT(nullptr != object, "ObjectPoseConfirmer.AddLiftRelativeObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.AddLiftRelativeObservation.UnSetObjectID");
  
  // Sanity check
  DEV_ASSERT(newPoseWrtLift.GetParent() == &_robot.GetLiftPose(),
             "ObjectPoseConfirmer.AddLiftRelativeObservation.PoseNotWrtLift");

  // If the object is on the lift, consider its pose as accurately known
  SetPoseHelper(object, newPoseWrtLift, -1, PoseState::Known, "AddLiftRelativeObservation");
  
  // numObservations is set to 1.
  // Basically, this requires that the object is observed a few times in a row without this method being called
  // in order for it to be localized to.
  _poseConfirmations[objectID] = PoseConfirmation(newPoseWrtLift, 1, object->GetLastObservedTime());

  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::CopyWithNewPose(ObservableObject *newObject, const Pose3d &newPose,
                                            const ObservableObject *oldObject)
{
  DEV_ASSERT(nullptr != newObject, "ObjectPoseConfirmer.CopyWithNewPose.NullNewObject");
  DEV_ASSERT(nullptr != oldObject, "ObjectPoseConfirmer.CopyWithNewPose.NullOldObject");
  
  const ObjectID& objectID = newObject->GetID();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.CopyWithNewPose.UnSetObjectID");
  
  //SetPoseHelper(newObject, newPose, oldObject->GetLastPoseUpdateDistance(), oldObject->GetPoseState(), "CopyWithNewPose");
  newObject->SetPose(newPose, oldObject->GetLastPoseUpdateDistance(), oldObject->GetPoseState());
  
  // If IDs match, then this just updates the old object's pose (and leaves num observations
  // the same). If newObject has a different ID, then this will add an entry for it, leave observations
  // set to zero and set its pose.
  _poseConfirmations[objectID].referencePose = newPose;
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddInExistingPose(const ObservableObject* object)
{
  DEV_ASSERT(nullptr != object, "ObjectPoseConfirmer.AddInExistingPose.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.AddInExistingPose.UnSetObjectID");
  
  _poseConfirmations[objectID].referencePose = object->GetPose();
  
  return RESULT_OK;
}
  
  
Result ObjectPoseConfirmer::MarkObjectUnobserved(ObservableObject* object)
{
  DEV_ASSERT(nullptr != object, "ObjectPoseConfirmer.MarkObjectUnobserved.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.MarkObjectUnobserved.UnSetObjectID");
  
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
