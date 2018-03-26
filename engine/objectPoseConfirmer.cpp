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

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/navMap/mapComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeLightComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoObservableObject.h"
#include "engine/robot.h"

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

// TODO: (Al) Remove once bryon fixes the timestamps
// Disable the object is still moving check for visual observation entries due to incorrect
// timestamps in object moved messages
CONSOLE_VAR(bool, kDisableStillMovingCheck, "PoseConfirmation", true);

}
  
ObjectPoseConfirmer::ObjectPoseConfirmer()
: IDependencyManagedComponent(this, RobotComponentID::ObjectPoseConfirmer)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::UpdatePoseInInstance(ObservableObject* object,
                                               const ObservableObject* observation,
                                               const ObservableObject* confirmedMatch,
                                               const Pose3d& newPose,
                                               bool robotWasMoving,
                                               f32 obsDistance_mm)
{
  // andrew/rsam: KnownIssueLostAccuracy
  // We always update poses and poseStates of unconfirmed observations. We also do not keep history
  // of observations for confirmed matches, only the last one that confirms at the reference pose. For those two
  // reasons, in a scenario where we have: Known, Known, Dirty, Dirty observations, where we
  // need 4 observations to confirm, the last one (Dirty) is the one that we will set in the object.
  // This is not ideal, but simplifies reasoning and code greatly, and it's arguable whether it's not the right thing
  // to do (eg: should all 4 confirmations be Known, for the pose to be Known?)
  // The accuracy lost is due to Known not being overridden by Dirty.

  // if we are updating an object that we are carrying, we always want to update, so that we can correct the
  // fact that it's not in the lift, it's where the observation happens
  const bool isCarriedObject = (nullptr != confirmedMatch) &&
                               _robot->GetCarryingComponent().IsCarryingObject( confirmedMatch->GetID() );
  const bool isLocalizableObject = object->CanBeUsedForLocalization();
  const bool isConfirmedMatch = (object == confirmedMatch);
  const bool isNewObject = (nullptr == confirmedMatch);
  
  // now calculate what posestate we should set and if that poseState overrides the current one (we still need
  // to calculate which one we are going to set, even if forceUpdate is already true)
  
  // Makes the isMoving check more temporally accurate, but might not actually be helping that much.
  // In order to ask for moved status, we need to query either the connected instance or the confirmedMatch (if
  // they exist)
  const ObjectID& objectID = object->GetID();
  const ActiveObject* const connectedMatch = _robot->GetBlockWorld().GetConnectedActiveObjectByID( objectID );
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
      PRINT_CH_DEBUG("PoseConfirmer", "ObjectPoseConfirmer.UpdatePoseInInstance.ObjectIsStillMoving", "");
      objectIsMoving = true;
    }
  }

  const bool isRobotOnTreads = OffTreadsState::OnTreads == _robot->GetOffTreadsState();
  const bool isFarAway  = Util::IsFltGT(obsDistance_mm,  object->GetMaxLocalizationDistance_mm());
  const bool setAsKnown = isRobotOnTreads && !isFarAway && !robotWasMoving && !objectIsMoving;
  const bool isDockObject = _robot->GetDockingComponent().GetDockObject() == objectID;

  bool updatePose = false;
  if ( isCarriedObject || isDockObject)
  {
    // carried objects are always updated so that they get detached from the lift upon observing them
    // dock objects are always updated so that we send updated docking error signals
    updatePose = true;
  }
  else if ( isNewObject || !robotWasMoving )
  {
    // if the robot was not moving and we see an object we already knew about,
    // update the pose of the confirmed object due to this observation.
    // (if the robot was moving, we only confirm _new_ objects, so we don't bump into a cube
    // placed in front of the robot for example)
    updatePose = !isLocalizableObject ||
                 !isConfirmedMatch ||
                 setAsKnown ||
                 !object->IsPoseStateKnown();
  }
  
  if( updatePose )
  {
    const PoseState newPoseState = (setAsKnown ? PoseState::Known : PoseState::Dirty);
    
    if ( isConfirmedMatch )
    {
      if(isDockObject)
      {
        // Since we are constantly updating the pose of the dock object, even while moving, the pose
        // will not be super accurate. This inaccuracy can result in the pose rotating and possibly
        // no longer being considered flat. To prevent this just clamp the pose to flat.
        Pose3d clampedPose = newPose;
        const f32 kClampedAngleTol = DEG_TO_RAD(20);
        ObservableObject::ClampPoseToFlat(clampedPose, kClampedAngleTol);
        SetPoseHelper(object, clampedPose, obsDistance_mm, newPoseState,
                      "ObjectPoseConfirmer.UpdateClampedPoseInInstance");
      }
      else
      {
        SetPoseHelper(object, newPose, obsDistance_mm, newPoseState, "ObjectPoseConfirmer.UpdatePoseInInstance");
      }
      
      // when we set the pose for an object we were carrying, we also unset it from the lift, tell the robot
      // we are not carrying it anymore
      if ( isCarriedObject )
      {
        PRINT_CH_INFO("PoseConfirmer", "ObjectPoseConfirmer.UpdatePoseInInstance.SeeingCarriedObject",
                      "We changed the pose of %d, we must not be carrying it anymore. Unsetting as carried object.",
                      object->GetID().GetValue());
        _robot->GetCarryingComponent().UnSetCarryObject(object->GetID());
      }
    }
    else
    {
      DEV_ASSERT( !isCarriedObject, "ObjectPoseConfirmer.UpdatePoseInInstance.CantCarryUnconfirmedObjects");
      
      // currently we use UpdatePoseInInstance for both unconfirmed and confirmed objects. If we are changing the
      // pose of the unconfirmedInstance, do not notify anyone about it yet, only if we are modifying the pose
      // of an actual object in BlockWorld
      object->SetPose(newPose, obsDistance_mm, newPoseState);
    }
    
    // udpate the timestamp at which we are actually setting the pose
    DEV_ASSERT(_poseConfirmations.find(objectID) != _poseConfirmations.end(),
                "ObjectPoseConfirmer.UpdatePoseInInstace.EntryShouldExist");
    PoseConfirmation& poseConf = _poseConfirmations[objectID];
    poseConf.lastPoseUpdatedTime = observation->GetLastObservedTime();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void ObjectPoseConfirmer::SetPoseHelper(ObservableObject*& object, const Pose3d& newPose,
                                               f32 distance, PoseState newPoseState, const char* debugStr)
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
  
  DEV_ASSERT(object->HasValidPose(),
             "ObjectPoseConfirmer.SetPoseHelper.ExpectedValidObjectPose");
  // note otherwise we don't delete the memory but we clear the pointer!
  DEV_ASSERT( object == _robot->GetBlockWorld().GetLocatedObjectByID(object->GetID()),
             "ObjectPoseConfirmer.SetPoseHelper.ShouldOnlyBeUsedForBlockWorldObjects");
  // this method is only called for instances in the current origin, not in other origins (would not make sense to clients)
  DEV_ASSERT( _robot->IsPoseInWorldOrigin(object->GetPose()),
             "ObjectPoseConfirmer.SetPoseHelper.ShouldOnlyBeUsedForCurrentOriginCurrent");
  DEV_ASSERT( !ObservableObject::IsValidPoseState(newPoseState) || _robot->IsPoseInWorldOrigin(newPose),
             "ObjectPoseConfirmer.SetPoseHelper.ShouldOnlyBeUsedForCurrentOriginNew");
  
  // if setting an invalid pose, we want to destroy the located copy of this object in its origin
  const bool isNewPoseValid = ObservableObject::IsValidPoseState(newPoseState);
  if( isNewPoseValid )
  {
    // if new pose is not known, we can't be localized to it
    if(newPoseState != PoseState::Known)
    {
      DelocalizeRobotFromObject(object->GetID());
    }

    // copy vars we need to notify, since they are about to change
    const Pose3d oldPoseCopy = object->GetPose(); // note PoseState is expected to be valid here, otherwise will assert
    const PoseState oldPoseState = object->GetPoseState();
  
    // change
    object->SetPose(newPose, distance, newPoseState);

    // notify of the change
    BroadcastObjectPoseChanged(*object, &oldPoseCopy, oldPoseState);
  }
  else
  {
    // delegate on marking unknown
    const bool propagateStack = true; // if we unobserve an object, we should unobserve objects on top. They should not float
    MarkObjectUnknown(object, propagateStack);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::SetPoseStateHelper(ObservableObject* object, PoseState newState)
{
  DEV_ASSERT( object->HasValidPose(), "ObjectPoseConfirmer.SetPoseStateHelper.CantChangePoseStateOfInvalidObjects");

  if ( ObservableObject::IsValidPoseState(newState) )
  {
    if(newState != PoseState::Known)
    {
      DelocalizeRobotFromObject(object->GetID());
    }
  
    // do the change, store old for notification
    const PoseState oldState = object->GetPoseState();
    object->SetPoseState(newState);
    
    // this method can change poseStates in other origins (arguably this should not be the case, but we
    // support it atm because we need to change other instances to Dirty when they move, etc). In that
    // case do not notify
    const bool isInCurOrigin = _robot->IsPoseInWorldOrigin(object->GetPose());
    if ( isInCurOrigin )
    {
      // broadcast the poseState change
      BroadcastObjectPoseStateChanged(*object, oldState);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("ObjectPoseConfirmer.SetPoseStateHelper.CantSetInvalidPoseState",
                      "Can't set pose state to '%s' for object %d.",
                      EnumToString(newState),
                      object->GetID().GetValue());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectPoseConfirmer::PoseConfirmation::PoseConfirmation(const std::shared_ptr<ObservableObject>& observation,
                                                        s32 initNumTimesObserved,
                                                        TimeStamp_t initLastPoseUpdatedTime)
: referencePose(observation->GetPose())
, numTimesObserved(initNumTimesObserved)
, lastPoseUpdatedTime(initLastPoseUpdatedTime)
, lastVisuallyMatchedTime(observation->GetLastObservedTime())
, unconfirmedObject(observation)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::PoseConfirmation::UpdatePose(const Pose3d& newPose, TimeStamp_t poseUpdatedTime)
{
  numTimesObserved = 1;
  referencePose = newPose;
  lastPoseUpdatedTime = poseUpdatedTime;
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
  
  // COZMO-9602. If the ID is preset in the observations (via objectLibrary storage, then this is only needed
  // if the object is passive)
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
  if ( objSeen->IsUnique() )
  {
    // ask blockworld to find matches by type/family in the current origin, since we assume only one instance per type
    std::vector<ObservableObject*> confirmedMatches;
    _robot->GetBlockWorld().FindLocatedMatchingObjects(filter, confirmedMatches);
    
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
      objectToCopyIDFrom = _robot->GetBlockWorld().FindLocatedMatchingObject(filter);
      if ( nullptr == objectToCopyIDFrom ) {
        objectToCopyIDFrom = _robot->GetBlockWorld().FindConnectedActiveMatchingObject(filter);
      }
      
      return;
    }
    
  }
  else
  {
    // For passive objects, match based on pose (considering only objects in current frame)
    // Ignore objects we're carrying
    const ObjectID& carryingObjectID = _robot->GetCarryingComponent().GetCarryingObject();
    filter.AddFilterFcn([&carryingObjectID](const ObservableObject* candidate) {
      const bool isObjectBeingCarried = (candidate->GetID() == carryingObjectID);
      return !isObjectBeingCarried;
    });
    
    const ObservableObject* matchingObject = _robot->GetBlockWorld().FindLocatedClosestMatchingObject(*objSeen,
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
      const bool newPoseIsSameAsRef = newPose.IsSameAs(poseConf.referencePose, distThreshold, angleThreshold);
      
      if(newPoseIsSameAsRef)
      {
        ++poseConf.numTimesObserved; // this can cause IsReferencePoseConfirmed to become true
        
        const bool confirmingObjectAtObservedPose = poseConf.IsReferencePoseConfirmed();
        if(confirmingObjectAtObservedPose)
        {
          const bool isCurrentlyConfirmed = (nullptr == unconfirmedObjectPtr);
          if( !isCurrentlyConfirmed )
          {
            // udpate pose in unconfirmed instance before passing onto world
            UpdatePoseInInstance(unconfirmedObjectPtr, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
          
            // This is first confirmation, we have to give blockWorld ownership of the unconfirmed object
            _robot->GetBlockWorld().AddLocatedObject(poseConf.unconfirmedObject);
            poseConf.unconfirmedObject.reset(); // give up ownership of the pointer (the pointer is still valid)
            
            // flip pointers now that it is confirmed
            confirmedMatch = unconfirmedObjectPtr;
            unconfirmedObjectPtr = nullptr;
          }
          else
          {
            UpdatePoseInInstance(confirmedMatch, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
          }
          
          // notify listeners that we can confirm we have seen this object at its current pose. This allows them
          // to reason about the markers seen this frame
          _robot->GetMapComponent().ClearRobotToMarkers(confirmedMatch);
        }
        else
        {
          // Update unconfirmed object's pose if it exists (if the object is already confirmed, then it doesn't)
          if ( unconfirmedObjectPtr ) {
            UpdatePoseInInstance(unconfirmedObjectPtr, observation.get(), confirmedMatch, newPose, robotWasMoving, obsDistance_mm);
          }
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
      }
      
      // Set regardless of whether pose is same
      poseConf.numTimesUnobserved = 0;
      poseConf.lastVisuallyMatchedTime = observation->GetLastObservedTime();
    }
  }
  
  // Make sure we always update the lastVisuallMatchedTime
  DEV_ASSERT_MSG(_poseConfirmations.at(objectID).lastVisuallyMatchedTime == observation->GetLastObservedTime(),
                 "ObjectPoseConfirmer.AddVisualObservation.WrongLastVisuallyMatchedTime",
                 "PoseConf t=%u, Observation t=%u",
                 _poseConfirmations.at(objectID).lastVisuallyMatchedTime,
                 observation->GetLastObservedTime());
  
  // ask if this observation confirms the reference pose
  const bool isConfirmedAtPose = _poseConfirmations[objectID].IsReferencePoseConfirmed();
  return isConfirmedAtPose;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddRobotRelativeObservation(ObservableObject* object, const Pose3d& poseRelToRobot, PoseState poseState)
{
  DEV_ASSERT(nullptr != object, "ObjectPoseConfirmer.AddRobotRelativeObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  // Sanity checks
  DEV_ASSERT(objectID.IsSet(),
             "ObjectPoseConfirmer.AddRobotRelativeObservation.UnSetObjectID");
  DEV_ASSERT(ObservableObject::IsValidPoseState(poseState),
             "ObjectPoseConfirmer.AddRobotRelativeObservation.RelativePoseStateNotValid");

  Pose3d poseWrtOrigin(poseRelToRobot);
  poseWrtOrigin.SetParent(_robot->GetPose());
  poseWrtOrigin = poseWrtOrigin.GetWithRespectToRoot();
  
  // When we add a relative observation we can be adding an observation for a new object (that doesn't exist
  // in the BlockWorld), or for an existing one, depending on the case, we do different things
  const ObservableObject* curWorldObjectWithID = _robot->GetBlockWorld().GetLocatedObjectByID(objectID);
  if ( nullptr != curWorldObjectWithID )
  {
    // we currently have an object in this origin, it should be the same, since the objectID is unique
    DEV_ASSERT(curWorldObjectWithID == object,
             "ObjectPoseConfirmer.AddRobotRelativeObservation.InstanceIsNotTheBlockWorldInstance");
    
    // in this case, we are simply updating the pose of an the object
    SetPoseHelper(object, poseWrtOrigin, -1.f, poseState, "AddRobotRelativeObservation");
  }
  else
  {
    // we don't currently have an object, this is the same as using the observation to confirm the object exists
    // change the pose in this instance, and then add to the world
    object->SetPose(poseWrtOrigin, -1, poseState);
    _robot->GetBlockWorld().AddLocatedObject(std::shared_ptr<ObservableObject>(object));
  }
  
  // numObservations is set to 1.
  // Basically, this requires that the object is observed a few times in a row without this method being called
  // in order for it to be localized to.
  auto & poseConf = _poseConfirmations[objectID]; // insert or find existing
  poseConf.UpdatePose(poseWrtOrigin, object->GetLastObservedTime());
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddObjectRelativeObservation(ObservableObject* objectToUpdate, const Pose3d& newPose,
                                                         const ObservableObject* observedObject)
{
  DEV_ASSERT(nullptr != objectToUpdate, "ObjectPoseConfirmer.AddObjectRelativeObservation.NullObjectToUpdate");
  DEV_ASSERT(nullptr != observedObject, "ObjectPoseConfirmer.AddObjectRelativeObservation.NullObservedObject");
  
  const ObjectID& objectID = objectToUpdate->GetID();
  
  // Sanity checks
  DEV_ASSERT(objectID.IsSet(),
             "ObjectPoseConfirmer.AddObjectRelativeObservation.UnSetObjectID");
  DEV_ASSERT(observedObject->HasValidPose(),
             "ObjectPoseConfirmer.AddObjectRelativeObservation.ReferenceNotValid");
  
  // if the object is in blockWorld, we need to notify of changes, use the helper for that
  const ObservableObject* objectInBlockWorld = _robot->GetBlockWorld().GetLocatedObjectByID(objectID);
  if ( nullptr != objectInBlockWorld )
  {
    // if the ID retrieved something from BlockWorld, it has to be the objectToUpdate, otherwise are we updating
    // an unconfirmed observation based on relative observations?
    DEV_ASSERT( objectToUpdate == objectInBlockWorld,
               "ObjectPoseConfirmer.AddObjectRelativeObservation.NotTheObjectInBlockWorldForID");
    // the object to update should have an entry in poseConfirmations, otherwise how did it become an
    // object that can be grabbed to add a relative observation?
    DEV_ASSERT(_poseConfirmations.find(objectID) != _poseConfirmations.end(),
              "ObjectPoseConfirmer.AddObjectRelativeObservation.NoPreviousObservationsForObjectToUpdate");

    // update pose
    const PoseState newPoseState = PoseState::Dirty; // do not inherit the pose state from the observed object
    SetPoseHelper(objectToUpdate, newPose, -1.f, newPoseState, "AddObjectRelativeObservation");
  }
  else
  {
    // here we should set the pose and then add the BlockWorld. We can definitely support it, but I don't have
    // a use case for it, and don't want to support it yet if it's not a thing
    PRINT_NAMED_ERROR("ObjectPoseConfirmer.AddObjectRelativeObservation.NotABlockWorldObject",
                      "Object %d is not in the blockWorld. We could add it, but we don't support it at the moment.",
                      objectID.GetValue());
  }

  // in any case, add to objects tracked by PoseConfirmer
  _poseConfirmations[objectID].referencePose = newPose;
  _poseConfirmations[objectID].lastPoseUpdatedTime = observedObject->GetLastObservedTime();
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::SetGhostObjectPose(ObservableObject* ghostObject, const Pose3d& newPose, PoseState newPoseState)
{
  DEV_ASSERT(nullptr != ghostObject, "ObjectPoseConfirmer.AddGhostObjectRelativeObservation.NullGhostObjectToUpdate");
  
  // Sanity checks
  DEV_ASSERT(ghostObject->GetID().IsSet(),
             "ObjectPoseConfirmer.AddGhostObjectRelativeObservation.UnSetObjectID");
  DEV_ASSERT(nullptr == _robot->GetBlockWorld().GetLocatedObjectByID(ghostObject->GetID()),
             "ObjectPoseConfirmer.AddGhostObjectRelativeObservation.ObjectInBlockWorld");

  // simply update the object directly, since it's a ghost and doesn't need confirmations
  ghostObject->SetPose(newPose, -1.0f, newPoseState);

  // I don't think we need to add ghost objects to PoseConfirmer
  // _poseConfirmations[objectID] = ???;
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::AddLiftRelativeObservation(ObservableObject* object, const Pose3d& newPoseWrtLift)
{
  DEV_ASSERT(nullptr != object, "ObjectPoseConfirmer.AddLiftRelativeObservation.NullObject");
  
  const ObjectID& objectID = object->GetID();
  
  // Sanity checks
  DEV_ASSERT(objectID.IsSet(),
             "ObjectPoseConfirmer.AddLiftRelativeObservation.UnSetObjectID");
  DEV_ASSERT(newPoseWrtLift.IsChildOf(_robot->GetComponent<FullRobotPose>().GetLiftPose()),
             "ObjectPoseConfirmer.AddLiftRelativeObservation.PoseNotWrtLift");
  DEV_ASSERT( object == _robot->GetBlockWorld().GetLocatedObjectByID(objectID),
             "ObjectPoseConfirmer.AddLiftRelativeObservation.NotTheObjectInBlockWorldForID");

  // If the object is on the lift, consider its pose as accurately known
  SetPoseHelper(object, newPoseWrtLift, -1, PoseState::Known, "AddLiftRelativeObservation");
  
  // numObservations is set to 1.
  // Basically, this requires that the object is observed a few times in a row without this method being called
  // in order for it to be localized to.
  auto & poseConf = _poseConfirmations[objectID]; // insert or find existing
  poseConf.UpdatePose(newPoseWrtLift, object->GetLastObservedTime());
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::CopyWithNewPose(ObservableObject *newObject, const Pose3d &newPose,
                                            const ObservableObject *oldObject)
{
  DEV_ASSERT(nullptr != newObject, "ObjectPoseConfirmer.CopyWithNewPose.NullNewObject");
  DEV_ASSERT(nullptr != oldObject, "ObjectPoseConfirmer.CopyWithNewPose.NullOldObject");
  
  const ObjectID& objectID = newObject->GetID();
  
  DEV_ASSERT(objectID.IsSet(),
             "ObjectPoseConfirmer.CopyWithNewPose.UnSetObjectID");

  // do not use SetPoseHelper to avoid notifying of the change (this is not a change, but inheriting someone else's)
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
  
  DEV_ASSERT(objectID.IsSet(),
             "ObjectPoseConfirmer.AddInExistingPose.UnSetObjectID");
  DEV_ASSERT( object == _robot->GetBlockWorld().GetLocatedObjectByID(objectID),
             "ObjectPoseConfirmer.AddInExistingPose.NotTheObjectInBlockWorldForID");
  
  _poseConfirmations[objectID].referencePose = object->GetPose();
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::BroadcastObjectPoseChanged(const ObservableObject& object,
                                                     const Pose3d* oldPose, PoseState oldPoseState)
{
  const ObjectID& objectID = object.GetID();

  // Sanity checks. These are guaranteed before we call the listeners, so they don't have to check
  if(ANKI_DEVELOPER_CODE)
  {
    const PoseState newPoseState = object.GetPoseState();
    // Check: objectID is valid
    DEV_ASSERT(objectID.IsSet(), "ObjectPoseConfirmer.BroadcastObjectPoseChanged.InvalidObjectID");
    // Check: PoseState=Invalid <-> Pose=nullptr
    const bool oldStateInvalid = !ObservableObject::IsValidPoseState(oldPoseState);
    const bool oldPoseNull     = (nullptr == oldPose);
    DEV_ASSERT(oldStateInvalid == oldPoseNull, "ObjectPoseConfirmer.BroadcastObjectPoseChanged.InvalidOldPoseParameters");
    const bool newStateInvalid = !ObservableObject::IsValidPoseState(newPoseState);
    // Check: Can't set from Invalid to Invalid
    DEV_ASSERT(!newStateInvalid || !oldStateInvalid, "ObjectPoseConfirmer.BroadcastObjectPoseChanged.BothStatesAreInvalid");
    // Check: newPose valid/invalid correlates with the object instance in the world (if invalid, null object;
    // if valid, non-null object). This guarantees we only notify for objects in current world
    const ObservableObject* locatedObject = _robot->GetBlockWorld().GetLocatedObjectByID(objectID);
    const bool isObjectNull = (nullptr == locatedObject);
    DEV_ASSERT(newStateInvalid == isObjectNull, "ObjectPoseConfirmer.BroadcastObjectPoseChanged.PoseStateAndObjectDontMatch");
    const bool isCopy = (&object != locatedObject);
    DEV_ASSERT(newStateInvalid == isCopy, "ObjectPoseConfirmer.BroadcastObjectPoseChanged.ObjectCopyExpectedOnlyIfInvalid");
    // we broadcast only current origin
    DEV_ASSERT(newStateInvalid || _robot->IsPoseInWorldOrigin(object.GetPose()),
              "ObjectPoseConfirmer.BroadcastObjectPoseChanged.BroadcastNotInCurrentOrigin");
  }

  // note we don't check if the Pose or PoseState actually changes. We assume something has changed if we called
  // this Broadcast

  // listeners
  _robot->GetBlockWorld().OnObjectPoseChanged(object, oldPose, oldPoseState);
  _robot->GetMapComponent().UpdateObjectPose(object, oldPose, oldPoseState); 
  
  // notify poseState changes if it changed
  BroadcastObjectPoseStateChanged(object, oldPoseState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::BroadcastObjectPoseStateChanged(const ObservableObject& object, PoseState oldPoseState)
{
  const ObjectID& objectID = object.GetID();
  const PoseState newPoseState = object.GetPoseState();
  DEV_ASSERT(objectID.IsSet(),
             "ObjectPoseConfirmer.BroadcastObjectPoseStateChanged.InvalidObjectID");
  
  // we broadcast only current origin
  DEV_ASSERT(!object.HasValidPose() || _robot->IsPoseInWorldOrigin(object.GetPose()),
             "ObjectPoseConfirmer.BroadcastObjectPoseStateChanged.BroadcastNotInCurrentOrigin");
  
  // only if it changes
  if ( oldPoseState != newPoseState )
  {
    // only update maps if the object already has a valid pose
    if (ObservableObject::IsValidPoseState(object.GetPoseState())) {
      _robot->GetMapComponent().UpdateObjectPose(object, &object.GetPose(), oldPoseState); 
    }

    // listeners
    const bool isActive = object.IsActive();
    if(isActive) {  // notify cubeLights only if the object is active
      _robot->GetCubeLightComponent().OnActiveObjectPoseStateChanged(objectID, oldPoseState, newPoseState);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectPoseConfirmer::MarkObjectUnobserved(ObservableObject*& object)
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::MarkObjectUnknown(ObservableObject*& object, bool propagateStack)
{
  DEV_ASSERT( object == _robot->GetBlockWorld().GetLocatedObjectByID(object->GetID()),
             "ObjectPoseConfirmer.MarkObjectUnknown.ShouldOnlyBeUsedForBlockWorldObjects");
  
  std::set<ObjectID> affectedObjectIDs;
  affectedObjectIDs.insert(object->GetID());
  
  // first iterate stacks because we will be flagging all those objects
  if ( propagateStack )
  {
    // TODO Consider using ConfigurationManager. I think now it would be worse, since I would have to check Stacks or
    // pyramids. Maybe configuration manager could cache BlocksOnTopOfOtherBlocks?
    const ObservableObject* curObject = object;
    
    while( nullptr != curObject )
    {
      BlockWorldFilter filterOnTop;
      // Ignore the object we are looking on top off so that we don't consider it as on top of itself
      filterOnTop.AddIgnoreID(curObject->GetID());
  
      // some objects should not be updated
      filterOnTop.AddIgnoreFamily(Anki::Cozmo::ObjectFamily::MarkerlessObject);
      filterOnTop.AddIgnoreFamily(Anki::Cozmo::ObjectFamily::CustomObject);

      // find object on top
      ObservableObject* objectOnTop = _robot->GetBlockWorld().FindLocatedObjectOnTopOf(*curObject, STACKED_HEIGHT_TOL_MM, filterOnTop);
      if ( nullptr != objectOnTop )
      {
        // we found an object currently on top
        const ObjectID& topID = objectOnTop->GetID();
      
        // if this is not an object we are carrying (rsam: I copied this from BlockWorld, not sure if it would even
        // happen, but sounds like a good check, since detaching from lift may require additional bookkeeping)
        if ( !_robot->GetCarryingComponent().IsCarryingObject(topID) )
        {
          // add to objects to delete
          affectedObjectIDs.insert(topID);
        }
        else
        {
          // warn
          PRINT_NAMED_WARNING("ObjectPoseConfirmer.MarkObjectUnknown",
                              "Found object %d on top, but we are carrying it, so we don't want to mess with that.",
                              topID.GetValue());
        }
      }
      
      // advance stack
      curObject = objectOnTop;
      
    } // while: curObject
  } // propagate stack

  BlockWorldFilter filterOfObjectsToDelete;
  for( const auto& objID : affectedObjectIDs )
  {
    // Unknown objects are deleted, add their IDs to this filter
    filterOfObjectsToDelete.AddAllowedID(objID);
    
    // TODO RobotMarkedObjectPoseUnknown is redundant with RobotDeletedLocatedObject. Remove and update game and SDK.
    // Still here for legacy purposes
    // Notify listeners if object is becoming Unknown
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(RobotMarkedObjectPoseUnknown(object->GetID().GetValue())));
  }

  // delete with the given filter
  _robot->GetBlockWorld().DeleteLocatedObjects(filterOfObjectsToDelete);
  object = nullptr; // clear the pointer because the object has been destroyed
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::MarkObjectDirty(ObservableObject* object, bool propagateStack)
{
  SetPoseStateHelper(object, PoseState::Dirty);
  
  // we changed the pose, propagate if needed
  if ( propagateStack )
  {
    BlockWorldFilter filterOnTop;
    // Ignore the object we are looking on top off so that we don't consider it as on top of itself
    filterOnTop.AddIgnoreID(object->GetID());
    // some objects should not be updated
    filterOnTop.AddIgnoreFamily(Anki::Cozmo::ObjectFamily::MarkerlessObject);
    filterOnTop.AddIgnoreFamily(Anki::Cozmo::ObjectFamily::CustomObject);

    // find object on top
    ObservableObject* objectOnTop = _robot->GetBlockWorld().FindLocatedObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM, filterOnTop);
    if ( nullptr != objectOnTop )
    {
      // if this is not an object we are carrying (rsam: I copied this from BlockWorld, not sure if it would even
      // happen, but sounds like a good check, since cubes on lift are considered Known at the moment
      if ( !_robot->GetCarryingComponent().IsCarryingObject(objectOnTop->GetID()) )
      {
        // can call recursively
        MarkObjectDirty(objectOnTop, propagateStack);
      }
      else
      {
        PRINT_CH_INFO("PoseConfirmer", "ObjectPoseConfirmer.MarkObjectDirty.TryingToChangeCarriedObject",
                      "Carrying %d, considered part of a Dirty stack. Ignoring propagation",
                      objectOnTop->GetID().GetValue());
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::Clear()
{
  _poseConfirmations.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::DeletedObjectInCurrentOrigin(const ObjectID& ID)
{
  _poseConfirmations.erase(ID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t ObjectPoseConfirmer::GetLastPoseUpdatedTime(const ObjectID& ID) const
{
  auto poseConf = _poseConfirmations.find(ID);
  if (poseConf != _poseConfirmations.end()) {
    return poseConf->second.lastPoseUpdatedTime;
  }
  
  return 0;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t ObjectPoseConfirmer::GetLastVisuallyMatchedTime(const ObjectID& ID) const
{
  auto poseConf = _poseConfirmations.find(ID);
  if (poseConf != _poseConfirmations.end()) {
    return poseConf->second.lastVisuallyMatchedTime;
  }
  
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectPoseConfirmer::DelocalizeRobotFromObject(const ObjectID& objectID) const
{
  if(_robot->GetLocalizedTo() == objectID)
  {
    _robot->SetLocalizedTo(nullptr);
  }
}
  
} // namespace Cozmo
} // namespace Anki
