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
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/dockingComponent.h"
#include "engine/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Vector {

CarryingComponent::CarryingComponent()
: IDependencyManagedComponent(this, RobotComponentID::Carrying)
{
  
}

void CarryingComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
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
  
  return _robot->SendRobotMessage<Anki::Vector::PlaceObjectOnGround>(0, 0, 0,
                                                                   DEFAULT_PATH_MOTION_PROFILE.speed_mmps,
                                                                   DEFAULT_PATH_MOTION_PROFILE.accel_mmps2,
                                                                   DEFAULT_PATH_MOTION_PROFILE.decel_mmps2);
}

Result CarryingComponent::SendSetCarryState(CarryState state) const
{
  return _robot->SendMessage(RobotInterface::EngineToRobot(Anki::Vector::CarryStateUpdate(state)));
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
  const Result poseResult = _robot->GetBlockWorld().SetObjectPose(object->GetID(), placedPoseWrtRobot, PoseState::Dirty);
  if(RESULT_OK != poseResult)
  {
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopRobotRelativeObservationFailed",
                      "AddRobotRealtiveObservation failed for %d", object->GetID().GetValue());
    return poseResult;
  }
  
  PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.ObjectPlaced",
                   "Robot successfully placed object %d at (%.2f, %.2f, %.2f).",
                   object->GetID().GetValue(),
                   object->GetPose().GetTranslation().x(),
                   object->GetPose().GetTranslation().y(),
                   object->GetPose().GetTranslation().z());
  
  // Store the object ID we were carrying before we unset it so we can clear it later if needed
  auto const& carriedObjectID = GetCarryingObjectID();
  
  UnSetCarryingObject();
  
  if(deleteLocatedObjects)
  {
    BlockWorldFilter filter;
    filter.AddAllowedID(carriedObjectID);
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

    SendSetCarryState(CarryState::CARRY_1_BLOCK);
  }
}

void CarryingComponent::UnSetCarryingObject()
{
  // Note this if statement body doesn't actually _do_ anything. It's just sanity checks.
  if (IsCarryingObject())
  {
    const auto& objID = GetCarryingObjectID();
    ObservableObject* carriedObject = _robot->GetBlockWorld().GetLocatedObjectByID(objID);
    if(carriedObject == nullptr) {
      PRINT_NAMED_ERROR("Robot.UnSetCarryingObject.NullObject",
                        "Object %d robot thought it was carrying no longer exists in the world.",
                        objID.GetValue());
    } else if ( carriedObject->GetPose().IsChildOf(_robot->GetComponent<FullRobotPose>().GetLiftPose())) {
      // if the carried object is still attached to the lift it can cause issues. We had a bug
      // in which we delocalized and unset as carrying, but would not dettach from lift, causing
      // the cube to accidentally inherit the new origin via its parent, the lift, since the robot is always
      // in the current origin. It would still be the a copy in the old origin in blockworld, but its pose
      // would be pointing to the current one, which caused issues with relocalization.
      // It's a warning because I think there are legit cases (like ClearObject), where it would be fine to
      // ignore the current pose, since it won't be used again.
      PRINT_NAMED_WARNING("Robot.UnSetCarryingObject.StillAttached",
                          "Setting carried object '%d' as not being carried, but the pose is still attached to the lift", objID.GetValue());
    } else if ( !carriedObject->GetPose().GetParent().IsRoot() )
    {
      // this happened as a bug when we had a stack of 2 cubes in the lift. The top one was not being detached properly,
      // so its pose was left attached to the bottom cube, which could cause issues if we ever deleted the bottom
      // object without seeing the top one ever again, since the pose for the bottom one (which is still top's pose's
      // parent) is deleted.
      // Also like &_liftPose check above, I believe it can happen when we delete the object, so downgraded to warning
      // instead of ANKI_VERIFY (which would be my ideal choice if delete took care of also detaching)
      PRINT_NAMED_WARNING("Robot.UnSetCarryingObject.StillAttachedToSomething",
                          "Setting carried object '%d' as not being carried, but the pose is still attached to something (other cube?!)",
                          objID.GetValue());
    }
  }
  
  // this method should not affect the object's pose or pose state; just clear the ID
  
  // Tell the robot it's not carrying anything
  if (_carryingObjectID.IsSet()) {
    SendSetCarryState(CarryState::CARRY_NONE);
  }
  
  // Even if the above failed, still mark the robot's carry ID as unset
  _carryingObjectID.UnSet();
  _carryingMarkerCode = Vision::MARKER_INVALID;
}

bool CarryingComponent::IsCarryingObject(const ObjectID& objectID) const
{
  return _carryingObjectID == objectID;
}

Result CarryingComponent::SetObjectAsAttachedToLift(const ObjectID& objectID,
                                                    const Vision::KnownMarker::Code atMarkerCode)
{
  if(!objectID.IsSet()) {
    // This can happen if the robot is picked up after/right at the end of completing a dock
    // The dock object gets cleared from the world but the robot ends up sending a
    // pickAndPlaceResult success
    LOG_WARNING("CarryingComponent.SetObjectAsAttachedToLift.ObjectIDNotSet",
                "Object ID not set.");
    return RESULT_FAIL;
  }
  
  if(IsCarryingObject()) {
    LOG_ERROR("CarryingComponent.SetObjectAsAttachedToLift.AlreadyCarryingObject",
              "Already carrying an object!");
    return RESULT_FAIL;
  }
  
  ObservableObject* object = _robot->GetBlockWorld().GetLocatedObjectByID(objectID);
  if(object == nullptr) {
    LOG_ERROR("CarryingComponent.SetObjectAsAttachedToLift.ObjectDoesNotExist",
              "Object with ID=%d does not exist", objectID.GetValue());
    return RESULT_FAIL;
  }
  
  // get the marker from the code
  const auto& markersWithCode = object->GetMarkersWithCode(atMarkerCode);
  if ( markersWithCode.empty() ) {
    LOG_ERROR("CarryingComponent.SetObjectAsAttachedToLift.NoMarkerWithCode",
              "No marker found with code %d.", atMarkerCode);
    return RESULT_FAIL;
  }
  
  // We assume that there is only one marker with the given code
  DEV_ASSERT(markersWithCode.size() == 1, "Robot.PickUpDockObject.MultipleMarkersForCode");
  
  const Vision::KnownMarker* attachmentMarker = markersWithCode[0];
  
  // Base the object's pose relative to the lift on how far away the dock
  // marker is from the center of the block
  Pose3d objectPoseWrtLiftPose;
  if(object->GetPose().GetWithRespectTo(_robot->GetComponent<FullRobotPose>().GetLiftPose(), objectPoseWrtLiftPose) == false) {
    LOG_ERROR("CarryingComponent.SetObjectAsAttachedToLift.ObjectAndLiftPoseHaveDifferentOrigins",
              "Object robot is picking up and robot's lift must share a common origin.");
    return RESULT_FAIL;
  }
  
  objectPoseWrtLiftPose.SetTranslation({attachmentMarker->GetPose().GetTranslation().Length() +
    LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f});
  
  SetCarryingObject(objectID, atMarkerCode); // also marks the object as carried
  
  const bool makePoseWrtOrigin = false;
  Result poseResult = _robot->GetBlockWorld().SetObjectPose(objectID,
                                                            objectPoseWrtLiftPose,
                                                            PoseState::Known,
                                                            makePoseWrtOrigin);
  if(RESULT_OK != poseResult)
  {
    LOG_WARNING("CarryingComponent.SetObjectAsAttachedToLift.FailedSettingPose",
                "Failed setting carried object pose (%d)", poseResult);
    return poseResult;
  }
  
  return RESULT_OK;
  
}

}
}
