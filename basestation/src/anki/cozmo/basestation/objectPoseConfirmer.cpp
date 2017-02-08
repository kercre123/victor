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
  // if we are updating an object that we are carrying, we always want to update, so that we can correct the
  // fact that it's not in the lift, it's where the observation happens
  bool updateDueToObservingCarriedObject = (nullptr != confirmedMatch) && _robot.IsCarryingObject( confirmedMatch->GetID() );
  
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

  const bool updatePose = updateDueToObservingCarriedObject || setAsKnown || !object->IsPoseStateKnown();
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
  
  DEV_ASSERT(object->HasValidPose() || ObservableObject::IsValidPoseState(newPoseState),
             "ObjectPoseConfirmer.SetPoseHelper.BothPoseStatesAreInvalid");
  
  // TODO these notifications should probably be post-change, rather than pre-change
  // Notify about the change we are about to make from old to new:
  _robot.GetBlockWorld().OnObjectPoseWillChange(object->GetID(), newPose, newPoseState);
  _robot.GetCubeLightComponent().OnObjectPoseStateWillChange(object->GetID(), object->GetPoseState(), newPoseState);
  
  // if state changed from Invalid to anything, fire an event object detected/seen
  if (!object->HasValidPose())
  {
    // should not set from invalid to invalid
    DEV_ASSERT(ObservableObject::IsValidPoseState(newPoseState), "ObjectPoseConfirmer.SetPoseHelper.InvalidNewPose");
    // event
    Util::sEventF("robot.object_seen",
                  {{DDATA, std::to_string(distance).c_str()}},
                  "%s",
                  EnumToString(object->GetType()));
  }
  
  // copy objectID since we can destroy the object
  const ObjectID objectID = object->GetID();
  
  // if setting an invalid pose, we want to destroy the located copy of this object in its origin
  const bool isNewPoseValid = ObservableObject::IsValidPoseState(newPoseState);
  if( isNewPoseValid )
  {
    object->SetPose(newPose, distance, newPoseState);
  }
  else
  {
    // Notify listeners if object is becoming Unknown
    using namespace ExternalInterface;
    _robot.Broadcast(MessageEngineToGame(RobotMarkedObjectPoseUnknown(_robot.GetID(), objectID.GetValue())));
    
    _robot.GetBlockWorld().DeleteLocatedObjectByIDInCurOrigin(object->GetID());
    object = nullptr; // do not use anymore, since it's deleted
  }
  
  _robot.GetBlockWorld().NotifyBlockConfigurationManagerObjectPoseChanged(objectID);
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
bool ObjectPoseConfirmer::PoseConfirmation::IsReferencePoseConfirmed() const
{
  const bool confirmed = (numTimesObserved >= kMinTimesToObserveObject);
  return confirmed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectPoseConfirmer::IsObjectConfirmedAtObservedPose(const std::shared_ptr<ObservableObject>& objSeen,
                                       const ObservableObject*& objectToCopyIDFrom ) const
{
  bool poseIsSameAsReference = false;
  
  // first, find the ID for that observation
  FindObjectMatchForObservation(objSeen, objectToCopyIDFrom);
  
  // match the pose against the referencePose. Note there are some cases that would not need this (for example passive
  // objects), but it simplifies the logic checking all with no exceptions
  if ( nullptr != objectToCopyIDFrom ) {
    const ObjectID& objectID = objectToCopyIDFrom->GetID();
    const auto& pairIterator = _poseConfirmations.find(objectID);
    if ( pairIterator != _poseConfirmations.end() )
    {
      // we have an entry, first check if the reference pose has been confirmed
      const PoseConfirmation& poseConf = pairIterator->second;
      if ( poseConf.IsReferencePoseConfirmed() )
      {
        // ok, reference is confirmed, is it the one we are asking about?
        const Point3f distThreshold  = objSeen->GetSameDistanceTolerance();
        const Radians angleThreshold = objSeen->GetSameAngleTolerance();
        poseIsSameAsReference = objSeen->GetPose().IsSameAs(poseConf.referencePose, distThreshold, angleThreshold);
      }
    }
  }
  
  // return whether the observed pose and the reference pose are the same
  return poseIsSameAsReference;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::FindObjectMatchForObservation(const std::shared_ptr<ObservableObject>& objSeen,
                                                        const ObservableObject*& objectToCopyIDFrom) const
{
  objectToCopyIDFrom = nullptr;

  BlockWorldFilter filter;
  filter.AddAllowedFamily(objSeen->GetFamily());
  filter.AddAllowedType(objSeen->GetType());

  // find confirmed object or unconfirmed
  if ( objSeen->IsActive() )
  {
    // ask blockworld to find matches by type/family in the current origin, since we assume only one instance per type
    std::vector<ObservableObject*> confirmedMatches;
    _robot.GetBlockWorld().FindLocatedMatchingObjects(filter, confirmedMatches);
    
    // should match 0 or 1
    DEV_ASSERT( confirmedMatches.size() <= 1, "ObjectPoseConfirmer.IsObjectConfirmed.TooManyMatches" );
    
    // if there is a match, return that one
    if ( confirmedMatches.size() == 1 )
    {
      objectToCopyIDFrom = confirmedMatches.front();
      return;
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
            objectToCopyIDFrom = confirmationInfo.unconfirmedObject.get();
            return;
          }
        }
      }
      
      // did not find an unconfirmed entry, search in other frames or in connected
      
      filter.SetOriginMode(BlockWorldFilter::OriginMode::NotInRobotFrame);
      objectToCopyIDFrom = _robot.GetBlockWorld().FindLocatedMatchingObject(filter);
      if ( nullptr == objectToCopyIDFrom ) {
        objectToCopyIDFrom = _robot.GetBlockWorld().FindConnectedActiveMatchingObject(filter);
      }
      
      return;
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
      // found a match
      objectToCopyIDFrom = matchingObject;
      return;
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

      // copy whatever we found (if anything)
      objectToCopyIDFrom = closestObject;
      return;
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
    
    // When we unobserve something we have an entry in PoseConfirmation, but we don't have confirmed (deleted)
    // or unconfirmed (was confirmed). In that case we need to treat the PoseConfirmation entry as if it
    // was the first one
    if ( (nullptr == confirmedMatch) && (nullptr == unconfirmedObjectPtr) )
    {
      // reset the observations to 1 in the current observation pose. This also grabs the observation
      // as the unconfirmed pointer
      poseConf = PoseConfirmation(observation, 1, 0);
    }
    else
    {
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
        ++poseConf.numTimesObserved; // this can cause IsReferencePoseConfirmed to become true
        
        const bool confirmingObjectAtObservedPose = poseConf.IsReferencePoseConfirmed();
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
      
      // Set regardless of whether pose is same
      poseConf.numTimesUnobserved = 0;
    }
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
  
// This fails for ghost objects. Hopefully in the future those are handled differently, not through poseConfirmation,
// since they are actually not real
//  // the object to update should have an entry in poseConfirmations, otherwise how did it become an
//  // object that can be grabbed to add a relative observation?
//  DEV_ASSERT(_poseConfirmations.find(objectID) != _poseConfirmations.end(),
//             "ObjectPoseConfirmer.AddObjectRelativeObservation.NoPreviousObservationsForObjectToUpdate");

  DEV_ASSERT(observedObject->HasValidPose(), "ObjectPoseConfirmer.AddObjectRelativeObservation.ReferenceNotValid");

  const PoseState newPoseState = PoseState::Dirty; // do not inherit the pose state from the observed object
  SetPoseHelper(objectToUpdate, newPose, -1.f, newPoseState, "AddObjectRelativeObservation");
  _poseConfirmations[objectID].lastPoseUpdatedTime = observedObject->GetLastObservedTime();
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
      
      SetPoseHelper(object, object->GetPose(), -1.f, PoseState::Invalid, "MarkObjectUnobserved");
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
