/**
 * File: carryingComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 6/13/2017
 *
* Description: Component for managing carrying/lift related things
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/carryingComponent.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/components/dockingComponent.h"
#include "engine/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Cozmo {

CarryingComponent::CarryingComponent()
: IDependencyManagedComponent(this, RobotComponentID::Carrying)
{
  
}

void CarryingComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
}


Result CarryingComponent::PlaceObjectOnGround()
{
  if(!IsCarryingObject()) {
    PRINT_NAMED_ERROR("Robot.PlaceObjectOnGround.NotCarryingObject",
                      "Robot told to place object on ground, but is not carrying an object.");
    return RESULT_FAIL;
  }
  
  _robot->GetDockingComponent().SetLastPickOrPlaceSucceeded(false);
  
  return _robot->SendRobotMessage<Anki::Cozmo::PlaceObjectOnGround>(0, 0, 0,
                                                                   DEFAULT_PATH_MOTION_PROFILE.speed_mmps,
                                                                   DEFAULT_PATH_MOTION_PROFILE.accel_mmps2,
                                                                   DEFAULT_PATH_MOTION_PROFILE.decel_mmps2);
}

Result CarryingComponent::SendSetCarryState(CarryState state) const
{
  return _robot->SendMessage(RobotInterface::EngineToRobot(Anki::Cozmo::CarryStateUpdate(state)));
}

Result CarryingComponent::SetDockObjectAsAttachedToLift()
{
  const ObjectID& dockObjectID = _robot->GetDockingComponent().GetDockObject();
  const Vision::KnownMarker::Code dockMarkerCode = _robot->GetDockingComponent().GetDockMarkerCode();
  return SetObjectAsAttachedToLift(dockObjectID, dockMarkerCode);
}

Result CarryingComponent::SetCarriedObjectAsUnattached(bool deleteLocatedObjects)
{
  if(IsCarryingObject() == false) {
    PRINT_NAMED_WARNING("Robot.SetCarriedObjectAsUnattached.CarryingObjectNotSpecified",
                        "Robot not carrying object, but told to place one. "
                        "(Possibly actually rolling or balancing or popping a wheelie.");
    return RESULT_FAIL;
  }
  
  // we currently only support attaching/detaching two objects
  DEV_ASSERT(GetCarryingObjects().size()<=2,"Robot.SetCarriedObjectAsUnattached.CountNotSupported");
  
  
  ObservableObject* object = _robot->GetBlockWorld().GetLocatedObjectByID(_carryingObjectID);
  
  if(object == nullptr)
  {
    // This really should not happen.  How can a object being carried get deleted?
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.CarryingObjectDoesNotExist",
                      "Carrying object with ID=%d no longer exists.", _carryingObjectID.GetValue());
    return RESULT_FAIL;
  }
  
  Pose3d placedPoseWrtRobot;
  if(object->GetPose().GetWithRespectTo(_robot->GetPose(), placedPoseWrtRobot) == false) {
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.OriginMisMatch",
                      "Could not get carrying object's pose relative to robot's origin.");
    return RESULT_FAIL;
  }
  
  // Initially just mark the pose as Dirty. Iff deleteLocatedObjects=true, then the ClearObject
  // call at the end will mark as Unknown. This is necessary because there are some
  // unfortunate ordering dependencies with how we set the pose, set the pose state, and
  // unset the carrying objects below. It's safer to do all of that, and _then_
  // clear the objects at the end.
  const Result poseResult = _robot->GetObjectPoseConfirmer().AddRobotRelativeObservation(object, placedPoseWrtRobot, PoseState::Dirty);
  if(RESULT_OK != poseResult)
  {
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopRobotRelativeObservationFailed",
                      "AddRobotRealtiveObservation failed for %d", object->GetID().GetValue());
    return poseResult;
  }
  
  PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.ObjectPlaced",
                   "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).",
                   _robot->GetID(), object->GetID().GetValue(),
                   object->GetPose().GetTranslation().x(),
                   object->GetPose().GetTranslation().y(),
                   object->GetPose().GetTranslation().z());
  
  // if we have a top one, we expect it to currently be attached to the bottom one (pose-wise)
  // recalculate its pose right where we think it is, but detach from the block, and hook directly to the origin
  if ( _carryingObjectOnTopID.IsSet() )
  {
    ObservableObject* topObject = _robot->GetBlockWorld().GetLocatedObjectByID(_carryingObjectOnTopID);
    if ( nullptr != topObject )
    {
      // get wrt robot so that we can add a robot observation (handy way to modify a pose)
      Pose3d topPlacedPoseWrtRobot;
      if(topObject->GetPose().GetWithRespectTo(_robot->GetPose(), topPlacedPoseWrtRobot) == true)
      {
        const Result topPoseResult = _robot->GetObjectPoseConfirmer().AddRobotRelativeObservation(topObject, topPlacedPoseWrtRobot, PoseState::Dirty);
        if(RESULT_OK == topPoseResult)
        {
          PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.TopObjectPlaced",
                           "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).",
                           _robot->GetID(),
                           topObject->GetID().GetValue(),
                           topObject->GetPose().GetTranslation().x(),
                           topObject->GetPose().GetTranslation().y(),
                           topObject->GetPose().GetTranslation().z());
        }
        else
        {
          PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopRobotRelativeObservationFailed",
                            "AddRobotRealtiveObservation failed for %d", topObject->GetID().GetValue());
        }
      }
      else
      {
        PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopOriginMisMatch",
                          "Could not get top carrying object's pose relative to robot's origin.");
      }
    }
    else
    {
      PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopCarryingObjectDoesNotExist",
                        "Top carrying object with ID=%d no longer exists.", _carryingObjectOnTopID.GetValue());
    }
  }
  
  // Store the object IDs we were carrying before we unset them so we can clear them later if needed
  auto const& carriedObjectIDs = GetCarryingObjects();
  
  UnSetCarryingObjects();
  
  if(deleteLocatedObjects)
  {
    BlockWorldFilter filter;
    filter.AddAllowedIDs(carriedObjectIDs);
    _robot->GetBlockWorld().DeleteLocatedObjects(filter);
  }
  
  return RESULT_OK;
  
}

void CarryingComponent::SetCarryingObject(ObjectID carryObjectID, Vision::Marker::Code atMarkerCode)
{
  ObservableObject* object = _robot->GetBlockWorld().GetLocatedObjectByID(carryObjectID);
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.SetCarryingObject.NullCarryObject",
                      "Object %d no longer exists in the world. Can't set it as robot's carried object.",
                      carryObjectID.GetValue());
  }
  else
  {
    _carryingObjectID = carryObjectID;
    _carryingMarkerCode = atMarkerCode;
    
    // Don't remain localized to an object if we are now carrying it
    if(_carryingObjectID == _robot->GetLocalizedTo())
    {
      // Note that the robot may still remaing localized (based on its
      // odometry), but just not *to an object*
      _robot->SetLocalizedTo(nullptr);
    } // if(_carryingObjectID == GetLocalizedTo())
    
    // Tell the robot it's carrying something
    // TODO: This is probably not the right way/place to do this (should we pass in carryObjectOnTopID?)
    if(_carryingObjectOnTopID.IsSet()) {
      SendSetCarryState(CarryState::CARRY_2_BLOCK);
    } else {
      SendSetCarryState(CarryState::CARRY_1_BLOCK);
    }
  }
}

void CarryingComponent::UnSetCarryingObjects(bool topOnly)
{
  // Note this loop doesn't actually _do_ anything. It's just sanity checks.
  std::set<ObjectID> carriedObjectIDs = GetCarryingObjects();
  for (auto& objID : carriedObjectIDs)
  {
    if (topOnly && objID != _carryingObjectOnTopID) {
      continue;
    }
    
    ObservableObject* carriedObject = _robot->GetBlockWorld().GetLocatedObjectByID(objID);
    if(carriedObject == nullptr) {
      PRINT_NAMED_ERROR("Robot.UnSetCarryingObjects.NullObject",
                        "Object %d robot %d thought it was carrying no longer exists in the world.",
                        objID.GetValue(), _robot->GetID());
      continue;
    }
    
    if ( carriedObject->GetPose().IsChildOf(_robot->GetComponent<FullRobotPose>().GetLiftPose())) {
      // if the carried object is still attached to the lift it can cause issues. We had a bug
      // in which we delocalized and unset as carrying, but would not dettach from lift, causing
      // the cube to accidentally inherit the new origin via its parent, the lift, since the robot is always
      // in the current origin. It would still be the a copy in the old origin in blockworld, but its pose
      // would be pointing to the current one, which caused issues with relocalization.
      // It's a warning because I think there are legit cases (like ClearObject), where it would be fine to
      // ignore the current pose, since it won't be used again.
      PRINT_NAMED_WARNING("Robot.UnSetCarryingObjects.StillAttached",
                          "Setting carried object '%d' as not being carried, but the pose is still attached to the lift", objID.GetValue());
      continue;
    }
    
    if ( !carriedObject->GetPose().GetParent().IsRoot() ) {
      // this happened as a bug when we had a stack of 2 cubes in the lift. The top one was not being detached properly,
      // so its pose was left attached to the bottom cube, which could cause issues if we ever deleted the bottom
      // object without seeing the top one ever again, since the pose for the bottom one (which is still top's pose's
      // parent) is deleted.
      // Also like &_liftPose check above, I believe it can happen when we delete the object, so downgraded to warning
      // instead of ANKI_VERIFY (which would be my ideal choice if delete took care of also detaching)
      PRINT_NAMED_WARNING("Robot.UnSetCarryingObjects.StillAttachedToSomething",
                          "Setting carried object '%d' as not being carried, but the pose is still attached to something (other cube?!)",
                          objID.GetValue());
    }
  }
  
  // this method should not affect the objects pose or pose state; just clear the IDs
  
  if (!topOnly) {
    // Tell the robot it's not carrying anything
    if (_carryingObjectID.IsSet()) {
      SendSetCarryState(CarryState::CARRY_NONE);
    }
    
    // Even if the above failed, still mark the robot's carry ID as unset
    _carryingObjectID.UnSet();
    _carryingMarkerCode = Vision::MARKER_INVALID;
  }
  _carryingObjectOnTopID.UnSet();
}

void CarryingComponent::UnSetCarryObject(ObjectID objID)
{
  // If it's the bottom object in the stack, unset all carried objects.
  if (_carryingObjectID == objID) {
    UnSetCarryingObjects(false);
  } else if (_carryingObjectOnTopID == objID) {
    UnSetCarryingObjects(true);
  }
}

const std::set<ObjectID> CarryingComponent::GetCarryingObjects() const
{
  std::set<ObjectID> objects;
  if (_carryingObjectID.IsSet()) {
    objects.insert(_carryingObjectID);
  }
  if (_carryingObjectOnTopID.IsSet()) {
    objects.insert(_carryingObjectOnTopID);
  }
  return objects;
}

bool CarryingComponent::IsCarryingObject(const ObjectID& objectID) const
{
  return _carryingObjectID == objectID || _carryingObjectOnTopID == objectID;
}

Result CarryingComponent::SetObjectAsAttachedToLift(const ObjectID& objectID,
                                                   const Vision::KnownMarker::Code atMarkerCode)
{
  if(!objectID.IsSet()) {
    // This can happen if the robot is picked up after/right at the end of completing a dock
    // The dock object gets cleared from the world but the robot ends up sending a
    // pickAndPlaceResult success
    PRINT_NAMED_WARNING("Robot.PickUpDockObject.ObjectIDNotSet",
                        "No docking object ID set, but told to pick one up.");
    return RESULT_FAIL;
  }
  
  if(IsCarryingObject()) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.AlreadyCarryingObject",
                      "Already carrying an object, but told to pick one up.");
    return RESULT_FAIL;
  }
  
  ObservableObject* object = _robot->GetBlockWorld().GetLocatedObjectByID(objectID);
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectDoesNotExist",
                      "Dock object with ID=%d no longer exists for picking up.", objectID.GetValue());
    return RESULT_FAIL;
  }
  
  // get the marker from the code
  const auto& markersWithCode = object->GetMarkersWithCode(atMarkerCode);
  if ( markersWithCode.empty() ) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoMarkerWithCode",
                      "No marker found with that code.");
    return RESULT_FAIL;
  }
  
  // notify if we have more than one, since we are going to assume that any is fine to use its pose
  // we currently don't have this, so treat as warning. But if we ever allow, make sure that they are
  // simetrical with respect to the object
  const bool hasMultipleMarkers = (markersWithCode.size() > 1);
  if(hasMultipleMarkers) {
    PRINT_NAMED_WARNING("Robot.PickUpDockObject.MultipleMarkersForCode",
                        "Multiple markers found for code '%d'. Using first for lift attachment", atMarkerCode);
  }
  
  const Vision::KnownMarker* attachmentMarker = markersWithCode[0];
  
  // Base the object's pose relative to the lift on how far away the dock
  // marker is from the center of the block
  // TODO: compute the height adjustment per object or at least use values from cozmoConfig.h
  Pose3d objectPoseWrtLiftPose;
  if(object->GetPose().GetWithRespectTo(_robot->GetComponent<FullRobotPose>().GetLiftPose(), objectPoseWrtLiftPose) == false) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectAndLiftPoseHaveDifferentOrigins",
                      "Object robot is picking up and robot's lift must share a common origin.");
    return RESULT_FAIL;
  }
  
  objectPoseWrtLiftPose.SetTranslation({attachmentMarker->GetPose().GetTranslation().Length() +
    LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f});
  
  // If we know there's an object on top of the object we are picking up,
  // mark it as being carried too
  // TODO: Do we need to be able to handle non-actionable objects on top of actionable ones?
  
  ObservableObject* objectOnTop = _robot->GetBlockWorld().FindLocatedObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM);
  if(objectOnTop != nullptr)
  {
    Pose3d onTopPoseWrtCarriedPose;
    if(objectOnTop->GetPose().GetWithRespectTo(object->GetPose(), onTopPoseWrtCarriedPose) == false)
    {
      PRINT_NAMED_WARNING("Robot.SetObjectAsAttachedToLift",
                          "Found object on top of carried object, but could not get its "
                          "pose w.r.t. the carried object.");
    } else {
      PRINT_NAMED_INFO("Robot.SetObjectAsAttachedToLift",
                       "Setting object %d on top of carried object as also being carried.",
                       objectOnTop->GetID().GetValue());
      
      onTopPoseWrtCarriedPose.SetParent(object->GetPose());
      
      // Related to COZMO-3384: Consider whether top cubes (in a stack) should notify memory map
      // Notify blockworld of the change in pose for the object on top, but pretend the new pose is unknown since
      // we are not dropping the cube yet
      Result poseResult = _robot->GetObjectPoseConfirmer().AddObjectRelativeObservation(objectOnTop, onTopPoseWrtCarriedPose, object);
      if(RESULT_OK != poseResult)
      {
        PRINT_NAMED_WARNING("Robot.SetObjectAsAttachedToLift.AddObjectRelativeObservationFailed",
                            "objectID:%d", object->GetID().GetValue());
        return poseResult;
      }
      
      _carryingObjectOnTopID = objectOnTop->GetID();
    }
    
  } else {
    _carryingObjectOnTopID.UnSet();
  }
  
  SetCarryingObject(objectID, atMarkerCode); // also marks the object as carried
  
  // Don't actually change the object's pose until we've checked for objects on top
  Result poseResult = _robot->GetObjectPoseConfirmer().AddLiftRelativeObservation(object, objectPoseWrtLiftPose);
  if(RESULT_OK != poseResult)
  {
    // TODO: warn
    return poseResult;
  }
  
  return RESULT_OK;
  
}

}
}
