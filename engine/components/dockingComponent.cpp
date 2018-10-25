/**
 * File: dockingComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 6/12/2017
 *
* Description: Component for managing docking related things
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/dockingComponent.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/vision/visionSystem.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

// Don't send docking error signal if body is rotating faster than this
CONSOLE_VAR(f32, kDockingRotatingTooFastThresh_degPerSec, "WasRotatingTooFast.Dock.Body_deg/s", RAD_TO_DEG(0.4f));

DockingComponent::DockingComponent()
: IDependencyManagedComponent(this, RobotComponentID::Docking)
{
  
}


void DockingComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
}


Result DockingComponent::DockWithObject(const ObjectID objectID,
                                        const f32 speed_mmps,
                                        const f32 accel_mmps2,
                                        const f32 decel_mmps2,
                                        const Vision::KnownMarker::Code markerCode,
                                        const Vision::KnownMarker::Code markerCode2,
                                        const DockAction dockAction,
                                        const f32 placementOffsetX_mm,
                                        const f32 placementOffsetY_mm,
                                        const f32 placementOffsetAngle_rad,
                                        const u8 numRetries,
                                        const DockingMethod dockingMethod,
                                        const bool doLiftLoadCheck,
                                        const bool backUpWhileLiftingCube)
{
  ActionableObject* object = dynamic_cast<ActionableObject*>(_robot->GetBlockWorld().GetLocatedObjectByID(objectID));
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.ObjectDoesNotExist",
                      "Object with ID=%d no longer exists for docking.", objectID.GetValue());
    return RESULT_FAIL;
  }
  
  // Need to store these so that when we receive notice from the physical
  // robot that it has picked up an object we can transition the docking
  // object to being carried, using PickUpDockObject()
  _dockObjectID   = objectID;
  _dockMarkerCode = markerCode;
  
  // get the marker from the code
  const auto& markersWithCode = object->GetMarkersWithCode(markerCode);
  if ( markersWithCode.empty() ) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.NoMarkerWithCode",
                      "No marker found with that code.");
    return RESULT_FAIL;
  }
  
  // notify if we have more than one, since we are going to assume that any is fine to use its pose
  // we currently don't have this, so treat as warning. But if we ever allow, make sure that they are
  // symmetrical with respect to the object
  const bool hasMultipleMarkers = (markersWithCode.size() > 1);
  if(hasMultipleMarkers) {
    PRINT_NAMED_WARNING("Robot.DockWithObject.MultipleMarkersForCode",
                        "Multiple markers found for code '%d'. Using first for lift attachment", markerCode);
  }
  
  const Vision::KnownMarker* dockMarker = markersWithCode[0];
  DEV_ASSERT(dockMarker != nullptr, "Robot.DockWithObject.InvalidMarker");
  
  // Dock marker has to be a child of the dock block
  if(!dockMarker->GetPose().IsChildOf(object->GetPose())) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.MarkerNotOnObject",
                      "Specified dock marker must be a child of the specified dock object.");
    return RESULT_FAIL;
  }
  
  // Mark as dirty so that the robot no longer localizes to this object
  const bool propagateStack = false;
  _robot->GetObjectPoseConfirmer().MarkObjectDirty(object, propagateStack);
  
  _lastPickOrPlaceSucceeded = false;
  
  _dockPlacementOffsetX_mm = placementOffsetX_mm;
  _dockPlacementOffsetY_mm = placementOffsetY_mm;
  _dockPlacementOffsetAngle_rad = placementOffsetAngle_rad;
  
  // Sends a message to the robot to dock with the specified marker
  // that it should currently be seeing. If pixel_radius == std::numeric_limits<u8>::max(),
  // the marker can be seen anywhere in the image (same as above function), otherwise the
  // marker's center must be seen at the specified image coordinates
  // with pixel_radius pixels.
  Result sendResult = _robot->SendRobotMessage<::Anki::Vector::DockWithObject>(0.0f,
                                                                             speed_mmps,
                                                                             accel_mmps2,
                                                                             decel_mmps2,
                                                                             dockAction,
                                                                             numRetries,
                                                                             dockingMethod,
                                                                             doLiftLoadCheck,
                                                                             backUpWhileLiftingCube);
  
  return sendResult;
}

Result DockingComponent::AbortDocking() const
{
  return _robot->SendMessage(RobotInterface::EngineToRobot(Anki::Vector::AbortDocking()));
}

void DockingComponent::UpdateDockingErrorSignal(const RobotTimeStamp_t t) const
{
  // The WasRotatingTooFast threshold for sending the docking error signal should be
  // tighter than the general WasRotatingTooFast threshold for marker detection
  DEV_ASSERT((kDockingRotatingTooFastThresh_degPerSec <= VisionSystem::GetBodyTurnSpeedThresh_degPerSec()),
             "VisionComponent.UpdateDockingErrorSignal.BodyTurnSpeedThreshTooRestrictive");
  
  const ObjectID& dockObjectID = GetDockObject();
  
  if(dockObjectID.IsUnknown())
  {
    return;
  }
  
  if(_robot->GetImuComponent().GetImuHistory().WasBodyRotatingTooFast(t, kDockingRotatingTooFastThresh_degPerSec))
  {
    PRINT_CH_INFO("VisionComponent",
                  "VisionComponent.UpdateDockingErrorSignal.RotatingTooFast",
                  "Body rotating too fast at time %u",
                  (TimeStamp_t)t);
    return;
  }
  
  BlockWorldFilter filter;
  filter.AddAllowedID(dockObjectID);
  
  const ObservableObject* object = _robot->GetBlockWorld().FindLocatedMatchingObject(filter);
  if(object != nullptr)
  {
    std::vector<Vision::KnownMarker*> dockMarkers = object->GetMarkersWithCode(_dockMarkerCode);
    
    // There should only be one marker with the docking marker code
    if(dockMarkers.size() == 1 && dockMarkers.front() != nullptr)
    {
      Pose3d robotPose;
      const Result res = _robot->GetComputedStateAt(t, robotPose);
      if(res == RESULT_OK)
      {
        Pose3d markerWrtRobot;
        const bool wrtSuccess = dockMarkers.front()->GetPose().GetWithRespectTo(robotPose, markerWrtRobot);
        if(wrtSuccess)
        {          
          DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = (TimeStamp_t)t;
          // xOffset should always be positive distance in front of the object so subtract it from
          // the x dist error
          dockErrMsg.x_distErr = markerWrtRobot.GetTranslation().x() - _dockPlacementOffsetX_mm;
          dockErrMsg.y_horErr  = markerWrtRobot.GetTranslation().y() + _dockPlacementOffsetY_mm;
          dockErrMsg.z_height  = markerWrtRobot.GetTranslation().z();
          
          // At most rotations, it is fine to grab individual axis angles from the rotation
          // However there are often many combinations of individual axis angles to compose
          // a given 3d rotation. This tends to become apparent as the z-axis nears downwards
          // vertical resulting in odd individual axis angles while the full 3d rotation is
          // still completely valid (this issue is not present at exactly upsidedown vertical).
          // Therefore to get around the issue, the rotation is clamped to flat so the z-axis
          // will be guaranteed vertical while maintaining rotation around z and grabbing the
          // individual angle around z-axis will always result in a correct/expected angle.
          const f32 kAngleTolToFlat = DEG_TO_RAD(40.f);
          ObservableObject::ClampPoseToFlat(markerWrtRobot, kAngleTolToFlat);
          dockErrMsg.angleErr  = markerWrtRobot.GetRotation().GetAngleAroundZaxis().ToFloat() + M_PI_2 + _dockPlacementOffsetAngle_rad;
          
          // Visualize docking error signal
          _robot->GetContext()->GetVizManager()->SetDockingError(dockErrMsg.x_distErr,
                                                                dockErrMsg.y_horErr,
                                                                dockErrMsg.z_height,
                                                                dockErrMsg.angleErr);
          
          // Try to use this for closed-loop control by sending it on to the robot
          _robot->SendRobotMessage<DockingErrorSignal>(std::move(dockErrMsg));
        }
        else
        {
          PRINT_NAMED_ERROR("VisionComponent.UpdateVisionMarkers.GetMarkerWRTRobotFailed",
                            "Failed to get dockMarker %s wrt robot",
                            dockMarkers.front()->GetCodeName());
        }
      }
      else
      {
        // Potentially expected?
        PRINT_NAMED_WARNING("VisionComponent.UpdateVisionMarkers.GetHistoricRobotPoseFailed",
                            "Failed to get computed state at time %u, result %u",
                            (TimeStamp_t)t,
                            res);
      }
    }
    else
    {
      PRINT_NAMED_ERROR("VisionComponent.UpdateVisionMarkers.BadNumDockingMarkers",
                        "Expecting 1 dockMarker have %zu",
                        dockMarkers.size());
    }
  }
  else
  {
    PRINT_NAMED_ERROR("VisionComponent.UpdateVisionMarkers.NullDockObject",
                      "Dock object %d is null, can't send docking error signal",
                      dockObjectID.GetValue());
  }
}

bool DockingComponent::CanStackOnTopOfObject(const ObservableObject& objectToStackOn) const
{
  // Note rsam/kevin: this only works currently for original cubes. Doing height checks would require more
  // comparison of sizes, checks for I can stack but not pick up due to slack required to pick up, etc. In order
  // to simplify just cover the most basic case here (for the moment)
  
  Pose3d relPos;
  if( !CanInteractWithObjectHelper(objectToStackOn, relPos) ) {
    return false;
  }
  
  // check if it's too high to stack on
  if ( objectToStackOn.IsPoseTooHigh(relPos, 1.f, STACKED_HEIGHT_TOL_MM, 0.5f) ) {
    return false;
  }
  
  // all checks clear
  return true;
}

bool DockingComponent::CanPickUpObject(const ObservableObject& objectToPickUp) const
{
  Pose3d relPos;
  if( !CanInteractWithObjectHelper(objectToPickUp, relPos) ) {
    return false;
  }
  
  // check if it's too high to pick up
  if ( objectToPickUp.IsPoseTooHigh(relPos, 2.f, STACKED_HEIGHT_TOL_MM, 0.5f) ) {
    return false;
  }
  
  // all checks clear
  return true;
}

bool DockingComponent::CanPickUpObjectFromGround(const ObservableObject& objectToPickUp) const
{
  Pose3d relPos;
  if( !CanInteractWithObjectHelper(objectToPickUp, relPos) ) {
    return false;
  }
  
  // check if it's too high to pick up
  if ( objectToPickUp.IsPoseTooHigh(relPos, 0.5f, ON_GROUND_HEIGHT_TOL_MM, 0.f) ) {
    return false;
  }
  
  // all checks clear
  return true;
}

bool DockingComponent::CanInteractWithObjectHelper(const ObservableObject& object, Pose3d& relPose) const
{
  // TODO:(bn) maybe there should be some central logic for which object families are valid here
  if( object.GetFamily() != ObjectFamily::Block &&
     object.GetFamily() != ObjectFamily::LightCube ) {
    return false;
  }
  
  // check that the object is ready to place on top of
  if( !object.IsRestingFlat() ||
     (_robot->GetCarryingComponent().IsCarryingObject() &&
      _robot->GetCarryingComponent().GetCarryingObject() == object.GetID()) ) {
       return false;
     }
  
  // check if we can transform to robot space
  if ( !object.GetPose().GetWithRespectTo(_robot->GetPose(), relPose) ) {
    return false;
  }
  
  // check if it has something on top
  const ObservableObject* objectOnTop = _robot->GetBlockWorld().FindLocatedObjectOnTopOf(object,
                                                                                        STACKED_HEIGHT_TOL_MM);
  if ( nullptr != objectOnTop ) {
    return false;
  }
  
  return true;
}

}
}
