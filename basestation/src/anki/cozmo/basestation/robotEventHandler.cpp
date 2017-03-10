/**
 * File: robotEventHandler.cpp
 *
 * Author: Lee
 * Created: 08/11/15
 *
 * Description: Class for subscribing to and handling events going to robots.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#include "anki/cozmo/basestation/robotEventHandler.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveOffChargerContactsAction.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/flipBlockAction.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/actions/setFaceAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"

#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/poseStructs.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {
  
u32 RobotEventHandler::_gameActionTagCounter = ActionConstants::FIRST_GAME_INTERNAL_TAG;
  
// =====================================================================================================================
#pragma mark -
#pragma mark GetAction() Helpers
  
// Implement a specialization of this method for each action message type
template<class MessageType>
static IActionRunner* GetActionHelper(Robot& robot, const MessageType& msg);
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//IActionRunner* GetPlaceObjectOnGroundHereAction(Robot& robot, const ExternalInterface::PlaceObjectOnGroundHere& msg)
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlaceObjectOnGroundHere& msg)
    {
  return new PlaceObjectOnGroundAction(robot);
    }
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//IActionRunner* GetPlaceObjectOnGroundAction(Robot& robot, const ExternalInterface::PlaceObjectOnGround& msg)
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlaceObjectOnGround& msg)
{
  // Create an action to drive to specied pose and then put down the carried
  // object.
  // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
  Rotation3d rot(UnitQuaternion(msg.qw, msg.qx, msg.qy, msg.qz));
  Pose3d targetPose(rot, Vec3f(msg.x_mm, msg.y_mm, 22.f), robot.GetWorldOrigin());
  return new PlaceObjectOnGroundAtPoseAction(robot,
                                             targetPose,
                                             msg.useExactRotation,
                                             msg.useManualSpeed,
                                             msg.checkDestinationFree);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlayAnimation& msg)
{
  return new PlayAnimationAction(robot, msg.animationName, msg.numLoops);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper function that is friended by TriggerCubeAnimationAction so we can call its private constructor
IActionRunner* GetPlayCubeAnimationHelper(Robot& robot, const ExternalInterface::PlayCubeAnimationTrigger& msg)
{
  return new TriggerCubeAnimationAction(robot, msg.objectID, msg.trigger);
}
  
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlayCubeAnimationTrigger& msg)
{
  return GetPlayCubeAnimationHelper(robot, msg);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::GotoPose& msg)
{
  // TODO: Add ability to indicate z too!
  // TODO: Better way to specify the target pose's parent
  Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0), robot.GetWorldOrigin());
  targetPose.SetName("GotoPoseTarget");
  
  // TODO: expose whether or not to drive with head down in message?
  // TODO: hard-code to false instead (shouldn't be necessary with no mat markers to see)
  // (For now it is hard-coded to true)
  const bool driveWithHeadDown = true;
  
  DriveToPoseAction* action = new DriveToPoseAction(robot,
                                                    targetPose,
                                                    driveWithHeadDown,
                                                    msg.useManualSpeed);
  
  if(msg.motionProf.isCustom)
  {
    action->SetMotionProfile(msg.motionProf);
  }
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::FlipBlock& msg)
{
  ObjectID selectedObjectID = msg.objectID;
  if(selectedObjectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  }

  DriveAndFlipBlockAction* action = new DriveAndFlipBlockAction(robot, selectedObjectID);
  
  if(msg.motionProf.isCustom)
{
    action->SetMotionProfile(msg.motionProf);
  }
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PanAndTilt& msg)
{
  return new PanAndTiltAction(robot, Radians(msg.bodyPan),
                              Radians(msg.headTilt),
                              msg.isPanAbsolute,
                              msg.isTiltAbsolute);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PickupObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose))
  {
    DriveToPickupObjectAction* action = new DriveToPickupObjectAction(robot,
                                                                      selectedObjectID,
                                                                      msg.useApproachAngle,
                                                                      msg.approachAngle_rad,
                                                                      msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    action->SetShouldCheckForObjectOnTopOf(msg.checkForObjectOnTop);
    
    return action;
  }
  else
  {
    PickupObjectAction* action = new PickupObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about a specific marker just that we are docking with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    action->SetShouldCheckForObjectOnTopOf(msg.checkForObjectOnTop);
    return action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlaceRelObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToPlaceRelObjectAction* action = new DriveToPlaceRelObjectAction(robot,
                                                                          selectedObjectID,
                                                                          true,
                                                                          msg.placementOffsetX_mm,
                                                                          0,
                                                                          msg.useApproachAngle,
                                                                          msg.approachAngle_rad,
                                                                          msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    return action;
  } else {
    PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                            selectedObjectID,
                                                            true,
                                                            msg.placementOffsetX_mm,
                                                            0,
                                                            msg.useManualSpeed);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about a specific marker just that we are docking with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    return action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlaceOnObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    
    DriveToPlaceOnObjectAction* action = new DriveToPlaceOnObjectAction(robot,
                                                                        selectedObjectID,
                                                                        msg.useApproachAngle,
                                                                        msg.approachAngle_rad,
                                                                        msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    action->SetShouldCheckForObjectOnTopOf(msg.checkForObjectOnTop);
    return action;
  } else {
    PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                            selectedObjectID,
                                                            false,
                                                            0,
                                                            0,
                                                            msg.useManualSpeed);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about a specific marker just that we are docking with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    action->SetShouldCheckForObjectOnTopOf(msg.checkForObjectOnTop);
    return action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::GotoObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  DriveToObjectAction* action;
  if(msg.usePreDockPose)
  {
    action = new DriveToObjectAction(robot,
                                     selectedObjectID,
                                     PreActionPose::ActionType::DOCKING);
  } else {
    action = new DriveToObjectAction(robot,
                                     selectedObjectID,
                                     msg.distanceFromObjectOrigin_mm,
                                     msg.useManualSpeed);
  }
  
  if(msg.motionProf.isCustom)
  {
    action->SetMotionProfile(msg.motionProf);
  }
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::AlignWithObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(robot,
                                                                            selectedObjectID,
                                                                            msg.distanceFromMarker_mm,
                                                                            msg.useApproachAngle,
                                                                            msg.approachAngle_rad,
                                                                            msg.alignmentType,
                                                                            msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    
    return action;
  } else {
    AlignWithObjectAction* action = new AlignWithObjectAction(robot,
                                                              selectedObjectID,
                                                              msg.distanceFromMarker_mm,
                                                              msg.alignmentType,
                                                              msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about aligning with a specific marker just that we are aligning with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    return action;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::CalibrateMotors& msg)
{
    CalibrateMotorAction* action = new CalibrateMotorAction(robot,
                                                            msg.calibrateHead,
                                                            msg.calibrateLift);
    return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::DriveStraight& msg)
{
  return new DriveStraightAction(robot, msg.dist_mm, msg.speed_mmps, msg.shouldPlayAnimation);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::DriveOffChargerContacts& msg)
{
  return new DriveOffChargerContactsAction(robot);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::RollObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToRollObjectAction* action = new DriveToRollObjectAction(robot,
                                                                  selectedObjectID,
                                                                  msg.useApproachAngle,
                                                                  msg.approachAngle_rad,
                                                                  msg.useManualSpeed);
    action->EnableDeepRoll(msg.doDeepRoll);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    action->SetShouldCheckForObjectOnTopOf(msg.checkForObjectOnTop);
    return action;
  } else {
    RollObjectAction* action = new RollObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->EnableDeepRoll(msg.doDeepRoll);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about a specific marker just that we are docking with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    action->SetShouldCheckForObjectOnTopOf(msg.checkForObjectOnTop);
    return action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PopAWheelie& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToPopAWheelieAction* action = new DriveToPopAWheelieAction(robot,
                                                                    selectedObjectID,
                                                                    msg.useApproachAngle,
                                                                    msg.approachAngle_rad,
                                                                    msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    
    return action;
  } else {
    PopAWheelieAction* action = new PopAWheelieAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about a specific marker just that we are docking with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    return action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::FacePlant& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToFacePlantAction* action = new DriveToFacePlantAction(robot,
                                                                selectedObjectID,
                                                                msg.useApproachAngle,
                                                                msg.approachAngle_rad,
                                                                msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    
    return action;
  } else {
    FacePlantAction* action = new FacePlantAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetDoNearPredockPoseCheck(false);
    // We don't care about a specific marker just that we are docking with the correct object
    action->SetShouldVisuallyVerifyObjectOnly(true);
    return action;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TraverseObject& msg)
{
  ObjectID selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToAndTraverseObjectAction* action = new DriveToAndTraverseObjectAction(robot,
                                                                                selectedObjectID,
                                                                                msg.useManualSpeed);
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    return action;
  } else {
    TraverseObjectAction* traverseAction = new TraverseObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    traverseAction->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2);
    return traverseAction;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::MountCharger& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToAndMountChargerAction* action =  new DriveToAndMountChargerAction(robot,
                                                                             selectedObjectID,
                                                                             msg.useManualSpeed);
    
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    return action;
  } else {
    MountChargerAction* chargerAction = new MountChargerAction(robot, selectedObjectID, msg.useManualSpeed);
    chargerAction->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    return chargerAction;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::RealignWithObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  DriveToRealignWithObjectAction* driveToRealignWithObjectAction = new DriveToRealignWithObjectAction(robot, selectedObjectID, msg.dist_mm);
  return driveToRealignWithObjectAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TurnInPlace& msg)
{
  TurnInPlaceAction* action = new TurnInPlaceAction(robot, msg.angle_rad, msg.isAbsolute);
  action->SetMaxSpeed(msg.speed_rad_per_sec);
  action->SetAccel(msg.accel_rad_per_sec2);
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TurnTowardsObject& msg)
{
  ObjectID objectID;
  if(msg.objectID == std::numeric_limits<u32>::max()) {
    objectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    objectID = msg.objectID;
  }
  
  TurnTowardsObjectAction* action = new TurnTowardsObjectAction(robot,
                                                                objectID,
                                                                Radians(msg.maxTurnAngle_rad),
                                                                msg.visuallyVerifyWhenDone,
                                                                msg.headTrackWhenDone);
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TurnTowardsPose& msg)
{
  Pose3d pose(0, Z_AXIS_3D(), {msg.world_x, msg.world_y, msg.world_z},
              robot.GetWorldOrigin());
  
  TurnTowardsPoseAction* action = new TurnTowardsPoseAction(robot, pose, Radians(msg.maxTurnAngle_rad));
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TurnTowardsFace& msg)
{
  TurnTowardsFaceAction* action = new TurnTowardsFaceAction(robot, msg.faceID, Radians(msg.maxTurnAngle_rad), msg.sayName);
  
  if(msg.sayName)
  {
    action->SetSayNameAnimationTrigger(msg.namedTrigger);
    action->SetNoNameAnimationTrigger(msg.unnamedTrigger);
  }
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TurnTowardsImagePoint& msg)
{
  TurnTowardsImagePointAction* action = new TurnTowardsImagePointAction(robot, Point2f(msg.x, msg.y), msg.timestamp);
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TurnTowardsLastFacePose& msg)
{
  TurnTowardsLastFacePoseAction* action = new TurnTowardsLastFacePoseAction(robot, Radians(msg.maxTurnAngle_rad), msg.sayName);
  
  if(msg.sayName)
  {
    action->SetSayNameAnimationTrigger(msg.namedTrigger);
    action->SetNoNameAnimationTrigger(msg.unnamedTrigger);
  }
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TrackToFace& trackFace)
{
  TrackFaceAction* action = new TrackFaceAction(robot, trackFace.faceID);
  
  // TODO: Support body-only mode
  if(trackFace.headOnly) {
    action->SetMode(ITrackAction::Mode::HeadOnly);
  }
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TrackToObject& trackObject)
{
  TrackObjectAction* action = new TrackObjectAction(robot, trackObject.objectID);
  action->SetMoveEyes(true);
  
  // TODO: Support body-only mode
  if(trackObject.headOnly) {
    action->SetMode(ITrackAction::Mode::HeadOnly);
  }
  
  return action;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::TrackToPet& trackPet)
{
  TrackPetFaceAction* action = nullptr;
  
  if(trackPet.petID != Vision::UnknownFaceID)
  {
    action = new TrackPetFaceAction(robot, trackPet.petID);
  }
  else
  {
    action = new TrackPetFaceAction(robot, trackPet.petType);
  }
  
  action->SetUpdateTimeout(trackPet.timeout_sec);
  
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::SetHeadAngle& setHeadAngle)
{
  MoveHeadToAngleAction* action = new MoveHeadToAngleAction(robot, setHeadAngle.angle_rad);
  action->SetMaxSpeed(setHeadAngle.max_speed_rad_per_sec);
  action->SetAccel(setHeadAngle.accel_rad_per_sec2);
  action->SetDuration(setHeadAngle.duration_sec);
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Version for SayText message
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::SayText& sayText)
{
  SayTextAction* sayTextAction = new SayTextAction(robot,
                                                   sayText.text,
                                                   sayText.voiceStyle,
                                                   sayText.durationScalar,
                                                   sayText.voicePitch);
  sayTextAction->SetAnimationTrigger(sayText.playEvent);
  sayTextAction->SetFitToDuration(sayText.fitToDuration);
  return sayTextAction;
}
  
// Version for SayTextWithIntent message
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::SayTextWithIntent& sayTextWithIntent)
{
  SayTextAction* sayTextAction = new SayTextAction(robot, sayTextWithIntent.text, sayTextWithIntent.intent);
  sayTextAction->SetAnimationTrigger(sayTextWithIntent.playEvent);
  sayTextAction->SetFitToDuration(sayTextWithIntent.fitToDuration);
  return sayTextAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::SetLiftHeight& msg)
{
  // Special case if commanding low dock height while carrying a block...
  if (msg.height_mm == LIFT_HEIGHT_LOWDOCK && robot.IsCarryingObject())
  {
    // ...put the block down right here.
    IActionRunner* newAction = new PlaceObjectOnGroundAction(robot);
    return newAction;
  }
  else
  {
      // In the normal case directly set the lift height
    MoveLiftToHeightAction* action = new MoveLiftToHeightAction(robot, msg.height_mm);
      action->SetMaxLiftSpeed(msg.max_speed_rad_per_sec);
      action->SetLiftAccel(msg.accel_rad_per_sec2);
      action->SetDuration(msg.duration_sec);
      
    return action;
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::VisuallyVerifyFace& msg)
{
  VisuallyVerifyFaceAction* action = new VisuallyVerifyFaceAction(robot, msg.faceID);
  action->SetNumImagesToWaitFor(msg.numImagesToWait);
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::VisuallyVerifyObject& msg)
{
  VisuallyVerifyObjectAction* action = new VisuallyVerifyObjectAction(robot, msg.objectID);
  action->SetNumImagesToWaitFor(msg.numImagesToWait);
  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::VisuallyVerifyNoObjectAtPose& msg)
{
  Pose3d p(0, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, msg.z_mm), robot.GetWorldOrigin());
  return new VisuallyVerifyNoObjectAtPoseAction(robot, p, {msg.x_thresh_mm, msg.y_thresh_mm, msg.z_thresh_mm});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::PlayAnimationTrigger& msg)
{
  IActionRunner* newAction = nullptr;
  std::underlying_type<AnimTrackFlag>::type ignoreTracks = Util::EnumToUnderlying(AnimTrackFlag::NO_TRACKS);
  
  if(msg.ignoreBodyTrack)
  {
    ignoreTracks |= Util::EnumToUnderlying(AnimTrackFlag::BODY_TRACK);
  }
  if(msg.ignoreHeadTrack)
  {
    ignoreTracks |= Util::EnumToUnderlying(AnimTrackFlag::HEAD_TRACK);
  }
  if(msg.ignoreLiftTrack)
  {
    ignoreTracks |= Util::EnumToUnderlying(AnimTrackFlag::LIFT_TRACK);
  }
  
  const bool kInterruptRunning = true; // TODO: expose this option in CLAD?
  
  if( msg.useLiftSafe ) {
    newAction = new TriggerLiftSafeAnimationAction(robot, msg.trigger, msg.numLoops, kInterruptRunning, ignoreTracks);
  }
  else {
    newAction = new TriggerAnimationAction(robot, msg.trigger, msg.numLoops, kInterruptRunning, ignoreTracks);
  }
  return newAction;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::ReadToolCode& msg)
{
  return new ReadToolCodeAction(robot);
}
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::SearchForNearbyObject& msg)
{
  return new SearchForNearbyObjectAction(robot, msg.desiredObjectID, msg.backupDistance_mm, msg.backupSpeed_mms, msg.headAngle_rad);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::Wait& msg)
{
  return new WaitAction(robot, msg.time_s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::WaitForImages& msg)
{
  return new WaitForImagesAction(robot, msg.numImages, msg.visionMode, msg.afterTimeStamp);
}
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Version for ProceduralFace
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::DisplayProceduralFace& msg)
{
  ProceduralFace procFace;
  procFace.SetFromMessage(msg);
      
  SetFaceAction* action = new SetFaceAction(robot, procFace, msg.duration_ms);
  return action;
}
      
// Version for face image
template<>
IActionRunner* GetActionHelper(Robot& robot, const ExternalInterface::DisplayFaceImage& msg)
{
  // Expand the bit-packed msg.faceData (every bit == 1 pixel) to byte array (every byte == 1 pixel)
  Vision::Image image(ProceduralFace::HEIGHT, ProceduralFace::WIDTH);
  static_assert(std::tuple_size<decltype(msg.faceData)>::value * 8 == (ProceduralFace::HEIGHT*ProceduralFace::WIDTH),
                "Mismatched face image and bit image sizes");
      
  assert(image.IsContinuous());
      
  uint8_t* imageData_i = image.GetDataPointer();
      
  uint32_t destI = 0;
  for (int i = 0; i < msg.faceData.size(); ++i)
  {
    uint8_t currentByte = msg.faceData[i];
      
    for (uint8_t bit = 0; bit < 8; ++bit)
    {
      imageData_i[destI] = ((currentByte & 0x80) > 0) ? 255 : 0;
      ++destI;
      currentByte = (uint8_t)(currentByte << 1);
    }
  }
  assert(destI == (ProceduralFace::WIDTH * ProceduralFace::HEIGHT));
      
  SetFaceAction* action = new SetFaceAction(robot, image, msg.duration_ms);
      
  return action;
}
      
// =====================================================================================================================
#pragma mark -
#pragma mark ActionMessageHandler/Entry/Array
      
// This section of helper classes is used to guarantee all RobotActionUnionTags
// are associated with a GetAction() handler method above, AND that each corresponding
// MessageGameToEngine for an action (i.e. commanded with no queuing) also call
// the same method.
      
template<ExternalInterface::RobotActionUnionTag ActionUnionTag, ExternalInterface::MessageGameToEngineTag GameToEngineTag>
struct GetActionWrapper
{
  static IActionRunner* GetActionUnionFcn(Robot& robot, const ExternalInterface::RobotActionUnion& actionUnion)
  {
    return GetActionHelper(robot, actionUnion.Get_<ActionUnionTag>());
  }
      
  static IActionRunner* GetGameToEngineFcn(Robot& robot, const ExternalInterface::MessageGameToEngine& msg)
  {
    return GetActionHelper(robot, msg.Get_<GameToEngineTag>());
  }
};
      
struct ActionMessageHandler
{
  using ActionUnionTag  = ExternalInterface::RobotActionUnionTag;
  using GameToEngineTag = ExternalInterface::MessageGameToEngineTag;
  using ActionUnionFcn  = RobotEventHandler::ActionUnionFcn;
  using GameToEngineFcn = RobotEventHandler::GameToEngineFcn;
      
  const ActionUnionTag     actionUnionTag;
  const GameToEngineTag    gameToEngineTag;
  const ActionUnionFcn     getActionFromActionUnion;
  const GameToEngineFcn    getActionFromMessage;
  const s32                numRetries;
      
  constexpr ActionMessageHandler(const ActionUnionTag&     actionUnionTag,
                                 const GameToEngineTag&    gameToEngineTag,
                                 const ActionUnionFcn      getActionFromActionUnion,
                                 const GameToEngineFcn     getActionFromMessage,
                                 const s32                 defaultNumRetries)
  : actionUnionTag(actionUnionTag)
  , gameToEngineTag(gameToEngineTag)
  , getActionFromActionUnion(getActionFromActionUnion)
  , getActionFromMessage(getActionFromMessage)
  , numRetries(defaultNumRetries)
  {
      
  }
      
};

using FullActionMessageHandlerArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<
  ActionMessageHandler::ActionUnionTag,
  ActionMessageHandler,
  ActionMessageHandler::ActionUnionTag::count>;
  
// =====================================================================================================================
#pragma mark -
#pragma mark RobotEventHandler Methods
      
RobotEventHandler::RobotEventHandler(const CozmoContext* context)
: _context(context)
{
  auto externalInterface = _context->GetExternalInterface();
      
  if (externalInterface != nullptr)
  {
    using namespace ExternalInterface;
      
    //
    // Handle action messages specially
    //
      
    // We'll use this callback for all action events
    auto actionEventCallback = std::bind(&RobotEventHandler::HandleActionEvents, this, std::placeholders::_1);
      
    // This macro makes adding handler defitions less painful/verbose by adding namespaces
    // and grabbing the right function pointer for the right method in the templated
    // GetActionWrapper helper struct above.
#   define DEFINE_HANDLER(__actionTag__, __g2eTag__, __numRetries__) \
    { \
      RobotActionUnionTag::__actionTag__ , \
      ActionMessageHandler(RobotActionUnionTag::__actionTag__, MessageGameToEngineTag::__g2eTag__, \
        &GetActionWrapper<RobotActionUnionTag::__actionTag__, MessageGameToEngineTag::__g2eTag__>::GetActionUnionFcn, \
        &GetActionWrapper<RobotActionUnionTag::__actionTag__, MessageGameToEngineTag::__g2eTag__>::GetGameToEngineFcn, \
        __numRetries__) \
    }

    constexpr static const FullActionMessageHandlerArray kActionHandlerArray {
      
      //
      // Create an entry pairing a RobotActionUnionTag with a MessageGameToEngineTag and
      // associating the template-specialized GetActionHelper() method here.
      // These should be added in the same order as they are defined in the
      // RobotActionUnion in messageActions.clad.
      //
      // If the order or number is not done correct, you will get a compilation error!
      //
      // Usage:
      //   DEFINE_HANDLER(actionUnionTag, msgGameToEngineTag, defaultNumRetriesa)
      //
      // NOTE: numRetries is only used when action is requested via MessageGameToEngine.
      //       (Otherwise, the numRetries in the action queueing message is used.)
      //
      
      DEFINE_HANDLER(alignWithObject,          AlignWithObject,          0),
      DEFINE_HANDLER(calibrateMotors,          CalibrateMotors,          0),
      DEFINE_HANDLER(displayFaceImage,         DisplayFaceImage,         0),
      DEFINE_HANDLER(displayProceduralFace,    DisplayProceduralFace,    0),
      DEFINE_HANDLER(driveOffChargerContacts,  DriveOffChargerContacts,  1),
      DEFINE_HANDLER(driveStraight,            DriveStraight,            0),
      DEFINE_HANDLER(facePlant,                FacePlant,                0),
      DEFINE_HANDLER(flipBlock,                FlipBlock,                0),
      DEFINE_HANDLER(gotoObject,               GotoObject,               0),
      DEFINE_HANDLER(gotoPose,                 GotoPose,                 2),
      DEFINE_HANDLER(mountCharger,             MountCharger,             1),
      DEFINE_HANDLER(panAndTilt,               PanAndTilt,               0),
      DEFINE_HANDLER(pickupObject,             PickupObject,             0),
      DEFINE_HANDLER(placeObjectOnGround,      PlaceObjectOnGround,      1),
      DEFINE_HANDLER(placeObjectOnGroundHere,  PlaceObjectOnGroundHere,  0),
      DEFINE_HANDLER(placeOnObject,            PlaceOnObject,            1),
      DEFINE_HANDLER(placeRelObject,           PlaceRelObject,           1),
      DEFINE_HANDLER(playAnimation,            PlayAnimation,            0),
      DEFINE_HANDLER(playAnimationTrigger,     PlayAnimationTrigger,     0),
      DEFINE_HANDLER(playCubeAnimationTrigger, PlayCubeAnimationTrigger, 0),
      DEFINE_HANDLER(popAWheelie,              PopAWheelie,              1),
      DEFINE_HANDLER(readToolCode,             ReadToolCode,             0),
      DEFINE_HANDLER(realignWithObject,        RealignWithObject,        1),
      DEFINE_HANDLER(rollObject,               RollObject,               1),
      DEFINE_HANDLER(sayText,                  SayText,                  0),
      DEFINE_HANDLER(sayTextWithIntent,        SayTextWithIntent,        0),
      DEFINE_HANDLER(searchForNearbyObject,    SearchForNearbyObject,    0),
      DEFINE_HANDLER(setHeadAngle,             SetHeadAngle,             0),
      DEFINE_HANDLER(setLiftHeight,            SetLiftHeight,            0),
      DEFINE_HANDLER(trackFace,                TrackToFace,              0),
      DEFINE_HANDLER(trackObject,              TrackToObject,            0),
      DEFINE_HANDLER(trackPet,                 TrackToPet,               0),
      DEFINE_HANDLER(traverseObject,           TraverseObject,           1),
      DEFINE_HANDLER(turnInPlace,              TurnInPlace,              0),
      DEFINE_HANDLER(turnTowardsFace,          TurnTowardsFace,          0),
      DEFINE_HANDLER(turnTowardsImagePoint,    TurnTowardsImagePoint,    0),
      DEFINE_HANDLER(turnTowardsLastFacePose,  TurnTowardsLastFacePose,  0),
      DEFINE_HANDLER(turnTowardsObject,        TurnTowardsObject,        0),
      DEFINE_HANDLER(turnTowardsPose,          TurnTowardsPose,          0),
      DEFINE_HANDLER(visuallyVerifyFace,       VisuallyVerifyFace,       0),
      DEFINE_HANDLER(visuallyVerifyNoObjectAtPose, VisuallyVerifyNoObjectAtPose, 0),
      DEFINE_HANDLER(visuallyVerifyObject,     VisuallyVerifyObject,     0),
      DEFINE_HANDLER(wait,                     Wait,                     0),
      DEFINE_HANDLER(waitForImages,            WaitForImages,            0),
    };

    static_assert(Util::FullEnumToValueArrayChecker::IsSequentialArray(kActionHandlerArray),
                  "Duplicated or out-of-order entries in action handler array.");
  
    // Build lookup tables so we don't have to linearly search through the above
    // array each time we want to find the handler
    for(auto & entry : kActionHandlerArray)
    {
      const ActionMessageHandler& handler = entry.Value();
  
      _actionUnionHandlerLUT[handler.actionUnionTag] = handler.getActionFromActionUnion;
      _gameToEngineHandlerLUT[handler.gameToEngineTag] = std::make_pair(handler.getActionFromMessage, handler.numRetries);
  
      // Also subscribe to the event here:
      _signalHandles.push_back(externalInterface->Subscribe(handler.gameToEngineTag, actionEventCallback));
    }
      
    //
    // For all other messages, just use an AnkiEventUtil object:
    //
    auto helper = MakeAnkiEventUtil(*_context->GetExternalInterface(), *this, _signalHandles);
      
    // GameToEngine: (in alphabetical order)
    helper.SubscribeGameToEngine<MessageGameToEngineTag::AbortAll>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::AbortPath>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::BehaviorManagerMessage>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CameraCalibration>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CancelAction>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CancelActionByIdTag>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ClearCalibrationImages>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ComputeCameraCalibration>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DrawPoseMarker>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCliffSensor>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableColorImages>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableLiftPower>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ExecuteTestPlan>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ForceDelocalizeRobot>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::IMURequest>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::QueueSingleAction>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::QueueCompoundAction>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestUnlockDataFromBackup>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveCalibrationImage>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SendAvailableObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetRobotCarryingObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::StopRobotForSdk>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::StreamObjectAccel>();
      
    // EngineToGame: (in alphabetical order)
    helper.SubscribeEngineToGame<MessageEngineToGameTag::AnimationAborted>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotCompletedAction>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotConnectionResponse>();
  }
      
} // RobotEventHandler Constructor
      
  
// =====================================================================================================================
#pragma mark -
#pragma mark Action Event Handlers
  
  
u32 RobotEventHandler::GetNextGameActionTag() {
  if (++_gameActionTagCounter > ActionConstants::LAST_GAME_INTERNAL_TAG) {
    _gameActionTagCounter = ActionConstants::FIRST_GAME_INTERNAL_TAG;
  }
  
  return _gameActionTagCounter;
}

void RobotEventHandler::HandleActionEvents(const GameToEngineEvent& event)
    {
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
      
  // If we don't have a valid robot there's nothing to do
  if (nullptr == robot)
  {
    return;
  }
      
  // Create the action
  auto const& msg = event.GetData();
  auto handlerIter = _gameToEngineHandlerLUT.find(msg.GetTag());
  if(handlerIter == _gameToEngineHandlerLUT.end())
    {
    // This should really never happen because we are supposed to be guaranteed at
    // compile time that all action tags are inserted.
    PRINT_NAMED_ERROR("RobotEventHandler.HandleActionEvents.MissingTag",
                      "%s (%hhu)", MessageGameToEngineTagToString(msg.GetTag()), msg.GetTag());
      return;
    }
  
  // Now we fill out our Action and possibly update number of retries:
  IActionRunner* newAction = handlerIter->second.first(*robot, msg);
  const u8 numRetries = handlerIter->second.second;
  newAction->SetTag(GetNextGameActionTag());

  
  // Everything's ok and we have an action, so queue it
  robot->GetActionList().QueueAction(QueueActionPosition::NOW, newAction, numRetries);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::QueueSingleAction& msg)
{
  // Can't queue actions for nonexistent robots...
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  auto handlerIter = _actionUnionHandlerLUT.find(msg.action.GetTag());
  if(handlerIter == _actionUnionHandlerLUT.end())
  {
    // This should really never happen because we are supposed to be guaranteed at
    // compile time that all action tags are inserted.
    PRINT_NAMED_ERROR("RobotEventHandler.HandleQueueSingleAction.MissingActionTag",
                      "%s (%hhu)", RobotActionUnionTagToString(msg.action.GetTag()), msg.action.GetTag());
    return;
  }
  
  IActionRunner* action = handlerIter->second(*robot, msg.action);
  action->SetTag(msg.idTag);

  // Put the action in the given position of the specified queue
  robot->GetActionList().QueueAction(msg.position, action, msg.numRetries);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::QueueCompoundAction& msg)
{
  // Can't queue actions for nonexistent robots...
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  // Create an empty parallel or sequential compound action:
  ICompoundAction* compoundAction = nullptr;
  if(msg.parallel) {
    compoundAction = new CompoundActionParallel(*robot);
  } else {
    compoundAction = new CompoundActionSequential(*robot);
  }
  
  // Add all the actions in the message to the compound action, according
  // to their type
  for(size_t iAction=0; iAction < msg.actions.size(); ++iAction)
  {
    auto const& actionUnion = msg.actions[iAction];
    
    auto handlerIter = _actionUnionHandlerLUT.find(actionUnion.GetTag());
    if(handlerIter == _actionUnionHandlerLUT.end())
    {
      // This should really never happen because we are supposed to be guaranteed at
      // compile time that all action tags are inserted.
      PRINT_NAMED_ERROR("RobotEventHandler.HandleQueueCompoundAction.MissingActionTag",
                        "Action %zu: %s (%hhu)", iAction,
                        RobotActionUnionTagToString(actionUnion.GetTag()), actionUnion.GetTag());
      return;
    }
    
    IActionRunner* action = handlerIter->second(*robot, actionUnion);
    
    compoundAction->AddAction(action);
    
  } // for each action/actionType
  
  compoundAction->SetTag(msg.idTag);

  // Put the action in the given position of the specified queue
  robot->GetActionList().QueueAction(msg.position, compoundAction, msg.numRetries);
}

  
// =====================================================================================================================
#pragma mark -
#pragma mark All Other Event Handlers

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::EnableLiftPower& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  if(robot->GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK)) {
    PRINT_NAMED_INFO("RobotEventHandler.HandleEnableLiftPower.LiftLocked",
                     "Ignoring ExternalInterface::EnableLiftPower while lift is locked.");
  } else {
    robot->GetMoveComponent().EnableLiftPower(msg.enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::EnableCliffSensor& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  if (nullptr != robot)
  {
    PRINT_NAMED_INFO("RobotEventHandler.HandleMessage.EnableCliffSensor","Setting to %s", msg.enable ? "true" : "false");
    robot->SetEnableCliffSensor(msg.enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::ForceDelocalizeRobot& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot) {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleForceDelocalizeRobot.InvalidRobotID",
                        "Failed to find robot to delocalize.");
      
  } else if(!robot->IsPhysical()) {
    PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.ForceDelocalize",
                     "Forcibly delocalizing robot %d", robot->GetID());
    
    robot->SendRobotMessage<RobotInterface::ForceDelocalizeSimulatedRobot>();
  } else {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleForceDelocalizeRobot.PhysicalRobot",
                        "Refusing to force delocalize physical robot.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::BehaviorManagerMessage& msg)
{
  const RobotID_t robotID = msg.robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleBehaviorManagerEvent.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetBehaviorManager().HandleMessage(msg.BehaviorManagerMessageUnion);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SendAvailableObjects& msg)
{
  const RobotID_t robotID = msg.robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSendAvailableObjects.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->BroadcastAvailableObjects(msg.enable);
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SaveCalibrationImage& msg)
{
  const RobotID_t robotID = msg.robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSaveCalibrationImage.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetVisionComponent().StoreNextImageForCameraCalibration();
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::ClearCalibrationImages& msg)
{
  const RobotID_t robotID = msg.robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleClearCalibrationImages.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetVisionComponent().ClearCalibrationImages();
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::ComputeCameraCalibration& msg)
{
  const RobotID_t robotID = msg.robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleComputeCameraCalibration.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const CameraCalibration& calib)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleCameraCalibration.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    std::vector<u8> calibVec(calib.Size());
    calib.Pack(calibVec.data(), calib.Size());
    robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CameraCalib, calibVec.data(), calibVec.size());
    
    PRINT_NAMED_INFO("RobotEventHandler.HandleCameraCalibration.SendingCalib",
                     "fx: %f, fy: %f, cx: %f, cy: %f, nrows %d, ncols %d",
                     calib.focalLength_x, calib.focalLength_y, calib.center_x, calib.center_y, calib.nrows, calib.ncols);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::AnimationAborted& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  if(nullptr == robot) {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleAnimationAborted.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->AbortAnimation();
    PRINT_NAMED_INFO("RobotEventHandler.HandleAnimationAborted.SendingRobotAbortAnimation", "");
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::RobotCompletedAction& msg)
{
  // Log DAS events for specific action completions
  switch(msg.actionType)
  {
    case RobotActionType::ALIGN_WITH_OBJECT:
    case RobotActionType::ASCEND_OR_DESCEND_RAMP:
    case RobotActionType::CROSS_BRIDGE:
    case RobotActionType::MOUNT_CHARGER:
    case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
    case RobotActionType::PICKUP_OBJECT_HIGH:
    case RobotActionType::PICKUP_OBJECT_LOW:
    case RobotActionType::PLACE_OBJECT_HIGH:
    case RobotActionType::PLACE_OBJECT_LOW:
    case RobotActionType::POP_A_WHEELIE:
    case RobotActionType::ROLL_OBJECT_LOW:
    {
      // Don't log incomplete docks -- they can happen for many reasons (such as
      // interruptions / cancellations on the way to docking) and we're most interested
      // in figuring out how successful the robot is when it gets a chance to actually
      // start trying to dock with the object
      if(msg.result != ActionResult::NOT_STARTED)
      {
        // Put action type in DDATA field and action result in s_val
        Util::sEventF("robot.dock_action_completed", {{DDATA, EnumToString(msg.actionType)}},
                      "%s", EnumToString(msg.result));
      }
      
      break;
    }
      
    default:
      break;
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::RobotConnectionResponse& msg)
{
  if (msg.result == RobotConnectionResult::Success) {
    Robot* robot = _context->GetRobotManager()->GetFirstRobot();
    
    if(nullptr == robot) {
      PRINT_NAMED_WARNING("RobotEventHandler.HandleRobotConnectionResponse.InvalidRobotID", "Failed to find robot.");
    }
    else
    {
      robot->SyncTime();
      PRINT_NAMED_INFO("RobotEventHandler.HandleRobotConnectionResponse.SendingSyncTime", "");
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::CancelAction& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleCancelAction.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->GetActionList().Cancel((RobotActionType)msg.actionType);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::CancelActionByIdTag& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleCancelActionByIdTag.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->GetActionList().Cancel(msg.idTag);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::DrawPoseMarker& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleDrawPoseMarker.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    if(robot->IsCarryingObject()) {
      Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0));
      const ObservableObject* carryObject = robot->GetBlockWorld().GetLocatedObjectByID(robot->GetCarryingObject());
      if(nullptr == carryObject)
      {
        PRINT_NAMED_WARNING("RobotEventHandler.HandleDrawPoseMarker.NullCarryObject",
                            "Carry object set to ID=%d, but BlockWorld returned NULL",
                            robot->GetCarryingObject().GetValue());
        return;
      }
      Quad2f objectFootprint = carryObject->GetBoundingQuadXY(targetPose);
      robot->GetContext()->GetVizManager()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const IMURequest& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleIMURequest.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->RequestIMU(msg.length_ms);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::ExecuteTestPlan& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleExecuteTestPlan.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    Planning::Path p;
    
    PathMotionProfile motionProfile(msg.motionProf);
    
    robot->GetLongPathPlannerPtr()->GetTestPath(robot->GetPose(), p, &motionProfile);
    robot->ExecutePath(p);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SetRobotCarryingObject& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSetRobotCarryingObject.InvalidRobotID", "Failed to find robot.");
    
  }
  else
  {
    if(msg.objectID < 0) {
      robot->SetCarriedObjectAsUnattached();
    } else {
      robot->SetCarryingObject(msg.objectID, Vision::MARKER_INVALID);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::StopRobotForSdk& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.StopRobotForSdk.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->GetActionList().Cancel();
    robot->GetAnimationStreamer().SetIdleAnimation(AnimationTrigger::Count);
    robot->GetMoveComponent().StopAllMotors();
    robot->GetBodyLightComponent().ClearAllBackpackLightConfigs();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::StreamObjectAccel& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.StreamObjectAccel.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    ActiveObject* obj = robot->GetBlockWorld().GetConnectedActiveObjectByID(msg.objectID);
    if (obj != nullptr) {
      PRINT_NAMED_INFO("RobotEventHandler.StreamObjectAccel", "ObjectID %d (activeID %d), enable %d", msg.objectID, obj->GetActiveID(), msg.enable);
      robot->SendMessage(RobotInterface::EngineToRobot(StreamObjectAccel(obj->GetActiveID(), msg.enable)));
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::AbortPath& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleAbortPath.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->AbortDrivingToPose();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::AbortAll& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleAbortAll.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->AbortAll();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::EnableColorImages& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleEnableColorImages.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    // Forward to robot
    robot->SendRobotMessage<RobotInterface::EnableColorImages>(msg.enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::RequestUnlockDataFromBackup& msg)
{
# ifndef COZMO_V2
  RobotDataBackupManager::HandleRequestUnlockDataFromBackup(msg, _context);
# else
  PRINT_NAMED_WARNING("RobotEventHandler.HandleRequestUnlockDataFromBackup.UnsupportedForCozmo2", "Restoring from backup belongs in Unity in Cozmo 2.0");
# endif
}

void RobotEventHandler::SetupGainsHandlers(IExternalInterface& externalInterface)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.SetupGainsHandlers.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    // SetControllerGains
    _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::ControllerGains,
                                                         [this, robot] (const GameToEngineEvent& event)
                                                         {
                                                           const ExternalInterface::ControllerGains& msg = event.GetData().Get_ControllerGains();
                                                           
                                                           robot->SendRobotMessage<RobotInterface::ControllerGains>(msg.kp, msg.ki, msg.kd, msg.maxIntegralError, msg.controller);
                                                         }));
    
    // SetMotionModelParams
    _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetMotionModelParams,
                                                         [this, robot] (const GameToEngineEvent& event)
                                                         {
                                                           const ExternalInterface::SetMotionModelParams& msg = event.GetData().Get_SetMotionModelParams();
                                                           
                                                           robot->SendRobotMessage<RobotInterface::SetMotionModelParams>(msg.slipFactor);
                                                         }));
    
    // RollActionParams
    _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::RollActionParams,
                                                         [this, robot] (const GameToEngineEvent& event)
                                                         {
                                                           const ExternalInterface::RollActionParams& msg = event.GetData().Get_RollActionParams();
                                                           
                                                           robot->SendRobotMessage<RobotInterface::RollActionParams>(msg.liftHeight_mm,
                                                                                                                     msg.driveSpeed_mmps,
                                                                                                                     msg.driveAccel_mmps2,
                                                                                                                     msg.driveDuration_ms,
                                                                                                                     msg.backupDist_mm);
                                                         }));
    
  }
}
  
} // namespace Cozmo
} // namespace Anki
