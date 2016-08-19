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


#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/robotEventHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/enrollNamedFaceAction.h"
#include "anki/cozmo/basestation/actions/flipBlockAction.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/actions/searchForObjectAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/poseStructs.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {
  
RobotEventHandler::RobotEventHandler(const CozmoContext* context)
: _context(context)
{
  auto externalInterface = _context->GetExternalInterface();
  
  if (externalInterface != nullptr)
  {
    using namespace ExternalInterface;
    
    // We'll use this callback for all action events
    auto actionEventCallback = std::bind(&RobotEventHandler::HandleActionEvents, this, std::placeholders::_1);
    
    // These are the all handled action event tags, in alphabetical order
    // TODO: Can we get all the tags from messageActions.clad programatically?
    std::vector<MessageGameToEngineTag> actionTagList =
    {
      MessageGameToEngineTag::AlignWithObject,
      MessageGameToEngineTag::FlipBlock,
      MessageGameToEngineTag::GotoPose,
      MessageGameToEngineTag::GotoObject,
      MessageGameToEngineTag::EnrollNamedFace,
      MessageGameToEngineTag::MountCharger,
      MessageGameToEngineTag::PanAndTilt,
      MessageGameToEngineTag::PickupObject,
      MessageGameToEngineTag::PlaceObjectOnGround,
      MessageGameToEngineTag::PlaceObjectOnGroundHere,
      MessageGameToEngineTag::PlaceOnObject,
      MessageGameToEngineTag::PlaceRelObject,
      MessageGameToEngineTag::PlayAnimation,
      MessageGameToEngineTag::PlayAnimationTrigger,
      MessageGameToEngineTag::PopAWheelie,
      MessageGameToEngineTag::ReadToolCode,
      MessageGameToEngineTag::RealignWithObject,
      MessageGameToEngineTag::RollObject,
      MessageGameToEngineTag::SayText,
      MessageGameToEngineTag::SayTextWithIntent,
      MessageGameToEngineTag::SearchForObject,
      MessageGameToEngineTag::SetHeadAngle,
      MessageGameToEngineTag::SetLiftHeight,
      MessageGameToEngineTag::TrackToFace,
      MessageGameToEngineTag::TrackToObject,
      MessageGameToEngineTag::TraverseObject,
      MessageGameToEngineTag::TurnInPlace,
      MessageGameToEngineTag::TurnTowardsFace,
      MessageGameToEngineTag::TurnTowardsLastFacePose,
      MessageGameToEngineTag::TurnTowardsObject,
      MessageGameToEngineTag::TurnTowardsPose,
      MessageGameToEngineTag::VisuallyVerifyFace,
      MessageGameToEngineTag::VisuallyVerifyObject,
      MessageGameToEngineTag::Wait,
      MessageGameToEngineTag::WaitForImages,
    };
    
    // Subscribe to all action events
    for (auto tag : actionTagList)
    {
      _signalHandles.push_back(externalInterface->Subscribe(tag, actionEventCallback));
    }
    
    // For all other messages, just use an AnkiEventUtil object:
    auto helper = MakeAnkiEventUtil(*context->GetExternalInterface(), *this, _signalHandles);
    
    // GameToEngine: (in alphabetical order)
    helper.SubscribeGameToEngine<MessageGameToEngineTag::AbortAll>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::AbortPath>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::BehaviorManagerMessage>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CameraCalibration>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CancelAction>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ClearCalibrationImages>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ComputeCameraCalibration>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DisplayProceduralFace>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DrawPoseMarker>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCliffSensor>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableLiftPower>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableRobotPickupParalysis>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ExecuteTestPlan>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ForceDelocalizeRobot>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::IMURequest>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::QueueSingleAction>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::QueueCompoundAction>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveCalibrationImage>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SendAvailableObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetActiveObjectLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetAllActiveObjectLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBackpackLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetHeadlight>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetRobotCarryingObject>();
    
    // EngineToGame: (in alphabetical order)
    helper.SubscribeEngineToGame<MessageEngineToGameTag::AnimationAborted>();
  }
}

IActionRunner* GetPlaceObjectOnGroundActionHelper(Robot& robot, const ExternalInterface::PlaceObjectOnGround& msg)
{
  // Create an action to drive to specied pose and then put down the carried
  // object.
  // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
  Rotation3d rot(UnitQuaternion<f32>(msg.qw, msg.qx, msg.qy, msg.qz));
  Pose3d targetPose(rot, Vec3f(msg.x_mm, msg.y_mm, 22.f), robot.GetWorldOrigin());
  return new PlaceObjectOnGroundAtPoseAction(robot,
                                             targetPose,
                                             msg.useExactRotation,
                                             msg.useManualSpeed,
                                             msg.checkDestinationFree);
}

IActionRunner* GetDriveToPoseActionHelper(Robot& robot, const ExternalInterface::GotoPose& msg)
{
  // TODO: Add ability to indicate z too!
  // TODO: Better way to specify the target pose's parent
  Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0), robot.GetWorldOrigin());
  targetPose.SetName("GotoPoseTarget");
  
  // TODO: expose whether or not to drive with head down in message?
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


IActionRunner* GetPickupActionHelper(Robot& robot, const ExternalInterface::PickupObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    DriveToPickupObjectAction* action = new DriveToPickupObjectAction(robot,
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
    PickupObjectAction* action = new PickupObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}


IActionRunner* GetPlaceRelActionHelper(Robot& robot, const ExternalInterface::PlaceRelObject& msg)
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
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}


IActionRunner* GetPlaceOnActionHelper(Robot& robot, const ExternalInterface::PlaceOnObject& msg)
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
    return action;
  } else {
    PlaceRelObjectAction* action = new PlaceRelObjectAction(robot,
                                                            selectedObjectID,
                                                            false,
                                                            0,
                                                            0,
                                                            msg.useManualSpeed);
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}

IActionRunner* GetDriveToObjectActionHelper(Robot& robot, const ExternalInterface::GotoObject& msg)
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

IActionRunner* GetDriveToAlignWithObjectActionHelper(Robot& robot, const ExternalInterface::AlignWithObject& msg)
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
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}

IActionRunner* GetRollObjectActionHelper(Robot& robot, const ExternalInterface::RollObject& msg)
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
    
    return action;
  } else {
    RollObjectAction* action = new RollObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    action->EnableDeepRoll(msg.doDeepRoll);
    return action;
  }
}

IActionRunner* GetPopAWheelieActionHelper(Robot& robot, const ExternalInterface::PopAWheelie& msg)
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
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}

IActionRunner* GetTraverseObjectActionHelper(Robot& robot, const ExternalInterface::TraverseObject& msg)
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

IActionRunner* GetMountChargerActionHelper(Robot& robot, const ExternalInterface::MountCharger& msg)
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
  
IActionRunner* GetRealignWithObjectActionHelper(Robot& robot, const ExternalInterface::RealignWithObject& msg)
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

IActionRunner* GetTurnInPlaceActionHelper(Robot& robot, const ExternalInterface::TurnInPlace& msg)
{
  TurnInPlaceAction* action = new TurnInPlaceAction(robot, msg.angle_rad, msg.isAbsolute);
  action->SetMaxSpeed(msg.speed_rad_per_sec);
  action->SetAccel(msg.accel_rad_per_sec2);
  return action;
}

IActionRunner* GetTurnTowardsObjectActionHelper(Robot& robot, const ExternalInterface::TurnTowardsObject& msg)
{
  ObjectID objectID;
  if(msg.objectID == u32_MAX) {
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

IActionRunner* GetTurnTowardsPoseActionHelper(Robot& robot, const ExternalInterface::TurnTowardsPose& msg)
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

IActionRunner* GetTurnTowardsFaceActionHelper(Robot& robot, const ExternalInterface::TurnTowardsFace& msg)
{
  TurnTowardsFaceAction* action = new TurnTowardsFaceAction(robot, msg.faceID, Radians(msg.maxTurnAngle_rad), msg.sayName);
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

IActionRunner* GetTurnTowardsLastFacePoseActionHelper(Robot& robot, const ExternalInterface::TurnTowardsLastFacePose& msg)
{
  TurnTowardsLastFacePoseAction* action = new TurnTowardsLastFacePoseAction(robot, Radians(msg.maxTurnAngle_rad), msg.sayName);
  
  action->SetMaxPanSpeed(msg.maxPanSpeed_radPerSec);
  action->SetPanAccel(msg.panAccel_radPerSec2);
  action->SetPanTolerance(msg.panTolerance_rad);
  action->SetMaxTiltSpeed(msg.maxTiltSpeed_radPerSec);
  action->SetTiltAccel(msg.tiltAccel_radPerSec2);
  action->SetTiltTolerance(msg.tiltTolerance_rad);
  
  return action;
}

IActionRunner* GetTrackFaceActionHelper(Robot& robot, const ExternalInterface::TrackToFace& trackFace)
{
  TrackFaceAction* action = new TrackFaceAction(robot, trackFace.faceID);
  
  // TODO: Support body-only mode
  if(trackFace.headOnly) {
    action->SetMode(ITrackAction::Mode::HeadOnly);
  }
  
  return action;
}

IActionRunner* GetTrackObjectActionHelper(Robot& robot, const ExternalInterface::TrackToObject& trackObject)
{
  TrackObjectAction* action = new TrackObjectAction(robot, trackObject.objectID);
  action->SetMoveEyes(true);
  
  // TODO: Support body-only mode
  if(trackObject.headOnly) {
    action->SetMode(ITrackAction::Mode::HeadOnly);
  }
  
  return action;
}

IActionRunner* GetMoveHeadToAngleActionHelper(Robot& robot, const ExternalInterface::SetHeadAngle& setHeadAngle)
{
  MoveHeadToAngleAction* action = new MoveHeadToAngleAction(robot, setHeadAngle.angle_rad);
  action->SetMaxSpeed(setHeadAngle.max_speed_rad_per_sec);
  action->SetAccel(setHeadAngle.accel_rad_per_sec2);
  action->SetDuration(setHeadAngle.duration_sec);
  return action;
}

/*
 Warning: Hard coding message handling functions who do not have robot id to get first robot in the same way: using robot manager to get first robot.
          Have also marked the messages who have robot id but value is incorrect and changed them to get first robot using step above.
*/

IActionRunner* GetEnrollNamedFaceActionHelper(Robot& robot, const ExternalInterface::EnrollNamedFace& enrollNamedFace)
{
  EnrollNamedFaceAction* enrollAction = new EnrollNamedFaceAction(robot, enrollNamedFace.faceID, enrollNamedFace.name, enrollNamedFace.mergeIntoID);
  enrollAction->SetSequenceType(enrollNamedFace.sequence);
  enrollAction->EnableSaveToRobot(enrollNamedFace.saveToRobot);
  return enrollAction;
}
  
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SetLiftHeight& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  if(robot->GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK)) {
    PRINT_NAMED_INFO("RobotEventHandler.HandleSetLiftHeight.LiftLocked",
                     "Ignoring ExternalInterface::SetLiftHeight while lift is locked.");
  } else {
    
    // Special case if commanding low dock height
    if (msg.height_mm == LIFT_HEIGHT_LOWDOCK && robot->IsCarryingObject()) {
      
      // Put the block down right here
      IActionRunner* newAction = new PlaceObjectOnGroundAction(*robot);
      robot->GetActionList().QueueAction(QueueActionPosition::NOW, newAction);
    }
    else {
      // In the normal case directly set the lift height
      MoveLiftToHeightAction* action = new MoveLiftToHeightAction(*robot, msg.height_mm);
      action->SetMaxLiftSpeed(msg.max_speed_rad_per_sec);
      action->SetLiftAccel(msg.accel_rad_per_sec2);
      action->SetDuration(msg.duration_sec);
      
      robot->GetActionList().QueueAction(QueueActionPosition::NOW, action);
    }
  }
}


IActionRunner* CreateSearchForObjectAction(Robot& robot, const ExternalInterface::SearchForObject& msg)
{
  SearchForObjectAction* newAction = new SearchForObjectAction(robot, msg.desiredObjectFamily, msg.desiredObjectID, msg.matchAnyObject );
  return newAction;
}

IActionRunner* CreateVisuallyVerifyFaceAction(Robot& robot, const ExternalInterface::VisuallyVerifyFace& msg)
{
  VisuallyVerifyFaceAction* action = new VisuallyVerifyFaceAction(robot, msg.faceID);
  action->SetNumImagesToWaitFor(msg.numImagesToWait);
  return action;
}

IActionRunner* CreateVisuallyVerifyObjectAction(Robot& robot, const ExternalInterface::VisuallyVerifyObject& msg)
{
  VisuallyVerifyObjectAction* action = new VisuallyVerifyObjectAction(robot, msg.objectID);
  action->SetNumImagesToWaitFor(msg.numImagesToWait);
  return action;
}

IActionRunner* CreateNewActionByType(Robot& robot,
                                     const ExternalInterface::RobotActionUnion& actionUnion)
{
  using namespace ExternalInterface;
  
  switch(actionUnion.GetTag())
  {
    case RobotActionUnionTag::turnInPlace:
      return GetTurnInPlaceActionHelper(robot, actionUnion.Get_turnInPlace());
      
    case RobotActionUnionTag::playAnimation:
    {
      auto & playAnimation = actionUnion.Get_playAnimation();
      return new PlayAnimationAction(robot, playAnimation.animationName, playAnimation.numLoops);
    }
    case RobotActionUnionTag::playAnimationTrigger:
    {
      auto & playAnimationTrigger = actionUnion.Get_playAnimationTrigger();
      return new TriggerAnimationAction(robot, playAnimationTrigger.trigger, playAnimationTrigger.numLoops);
    }
    case RobotActionUnionTag::pickupObject:
      return GetPickupActionHelper(robot, actionUnion.Get_pickupObject());
      
    case RobotActionUnionTag::placeOnObject:
      return GetPlaceOnActionHelper(robot, actionUnion.Get_placeOnObject());
      
    case RobotActionUnionTag::placeRelObject:
      return GetPlaceRelActionHelper(robot, actionUnion.Get_placeRelObject());
      
    case RobotActionUnionTag::placeObjectOnGround:
      return GetPlaceObjectOnGroundActionHelper(robot, actionUnion.Get_placeObjectOnGround());
      
    case RobotActionUnionTag::placeObjectOnGroundHere:
      return new PlaceObjectOnGroundAction(robot);
      
    case RobotActionUnionTag::setHeadAngle:
      return GetMoveHeadToAngleActionHelper(robot, actionUnion.Get_setHeadAngle());
      
    case RobotActionUnionTag::setLiftHeight:
      // TODO: Provide a means to pass in the speed/acceleration values to the action
      return new MoveLiftToHeightAction(robot, actionUnion.Get_setLiftHeight().height_mm);
      
    case RobotActionUnionTag::turnTowardsObject:
      return GetTurnTowardsObjectActionHelper(robot, actionUnion.Get_turnTowardsObject());
      
    case RobotActionUnionTag::turnTowardsPose:
      return GetTurnTowardsPoseActionHelper(robot, actionUnion.Get_turnTowardsPose());
      
    case RobotActionUnionTag::rollObject:
      return GetRollObjectActionHelper(robot, actionUnion.Get_rollObject());
      
    case RobotActionUnionTag::popAWheelie:
      return GetPopAWheelieActionHelper(robot, actionUnion.Get_popAWheelie());
      
    case RobotActionUnionTag::gotoObject:
      return GetDriveToObjectActionHelper(robot, actionUnion.Get_gotoObject());
      
    case RobotActionUnionTag::gotoPose:
      return GetDriveToPoseActionHelper(robot, actionUnion.Get_gotoPose());
      
    case RobotActionUnionTag::alignWithObject:
      return GetDriveToAlignWithObjectActionHelper(robot, actionUnion.Get_alignWithObject());
      
    case RobotActionUnionTag::trackFace:
      return GetTrackFaceActionHelper(robot, actionUnion.Get_trackFace());
      
    case RobotActionUnionTag::trackObject:
      return GetTrackObjectActionHelper(robot, actionUnion.Get_trackObject());
      
    case RobotActionUnionTag::driveStraight:
      return new DriveStraightAction(robot, actionUnion.Get_driveStraight().dist_mm,
                                     actionUnion.Get_driveStraight().speed_mmps,
                                     actionUnion.Get_driveStraight().shouldPlayAnimation);
      
    case RobotActionUnionTag::panAndTilt:
      return new PanAndTiltAction(robot, Radians(actionUnion.Get_panAndTilt().bodyPan),
                                  Radians(actionUnion.Get_panAndTilt().headTilt),
                                  actionUnion.Get_panAndTilt().isPanAbsolute,
                                  actionUnion.Get_panAndTilt().isTiltAbsolute);
      
    case RobotActionUnionTag::wait:
      return new WaitAction(robot, actionUnion.Get_wait().time_s);
      
    case RobotActionUnionTag::mountCharger:
      return GetMountChargerActionHelper(robot, actionUnion.Get_mountCharger());
      
    case RobotActionUnionTag::turnTowardsFace:
      return GetTurnTowardsFaceActionHelper(robot, actionUnion.Get_turnTowardsFace());
      
    case RobotActionUnionTag::turnTowardsLastFacePose:
      return GetTurnTowardsLastFacePoseActionHelper(robot, actionUnion.Get_turnTowardsLastFacePose());
      
    case RobotActionUnionTag::searchSideToSide:
      return new SearchSideToSideAction(robot);
      
    case RobotActionUnionTag::searchForObject:
      return CreateSearchForObjectAction(robot, actionUnion.Get_searchForObject());
      
    case RobotActionUnionTag::readToolCode:
      return new ReadToolCodeAction(robot);
      
    case RobotActionUnionTag::realignWithObject:
      return GetRealignWithObjectActionHelper(robot, actionUnion.Get_realignWithObject());
      
    case RobotActionUnionTag::sayText:
    {
      const auto msg = actionUnion.Get_sayText();
      SayTextAction* sayTextAction = new SayTextAction(robot, msg.text, msg.voiceStyle, msg.durationScalar);
      sayTextAction->SetAnimationTrigger(msg.playEvent);
      return sayTextAction;
    }
      
    case RobotActionUnionTag::sayTextWithIntent:
    {
      const auto msg = actionUnion.Get_sayTextWithIntent();
      SayTextAction* sayTextAction = new SayTextAction(robot, msg.text, msg.intent);
      sayTextAction->SetAnimationTrigger(msg.playEvent);
      return sayTextAction;
    }
      
    case RobotActionUnionTag::enrollNamedFace:
      return GetEnrollNamedFaceActionHelper(robot, actionUnion.Get_enrollNamedFace());
    
    case RobotActionUnionTag::flipBlock:
    {
      ObjectID selectedObjectID = actionUnion.Get_flipBlock().objectID;
      if(selectedObjectID < 0) {
        selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
      }
      
      DriveAndFlipBlockAction* action = new DriveAndFlipBlockAction(robot, selectedObjectID);
      
      if(actionUnion.Get_flipBlock().motionProf.isCustom)
      {
        action->SetMotionProfile(actionUnion.Get_flipBlock().motionProf);
      }
      return action;
    }
      
    case RobotActionUnionTag::waitForImages:
    {
      const ExternalInterface::WaitForImages& waitMsg = actionUnion.Get_waitForImages();
      return new WaitForImagesAction(robot, waitMsg.numImages, waitMsg.visionMode, waitMsg.afterTimeStamp);
    }
      
    case RobotActionUnionTag::visuallyVerifyFace:
      return CreateVisuallyVerifyFaceAction(robot, actionUnion.Get_visuallyVerifyFace());
      
    case RobotActionUnionTag::visuallyVerifyObject:
      return CreateVisuallyVerifyObjectAction(robot, actionUnion.Get_visuallyVerifyObject());
      
    default:
      PRINT_NAMED_ERROR("RobotEventHandler.CreateNewActionByType.InvalidActionTag",
                        "Failed to create an action for the given actionTag.");
      return nullptr;
  }
}

void RobotEventHandler::HandleActionEvents(const GameToEngineEvent& event)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // If we don't have a valid robot there's nothing to do
  if (nullptr == robot)
  {
    return;
  }
  
  // We'll pass around a reference to the Robot for convenience sake
  Robot& robotRef = *robot;
  
  // Now we fill out our Action and possibly update number of retries:
  IActionRunner* newAction = nullptr;
  u8 numRetries = 0;
  switch (event.GetData().GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGround:
    {
      numRetries = 1;
      newAction = GetPlaceObjectOnGroundActionHelper(robotRef, event.GetData().Get_PlaceObjectOnGround());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGroundHere:
    {
      newAction = new PlaceObjectOnGroundAction(robotRef);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::GotoPose:
    {
      numRetries = 2;
      newAction = GetDriveToPoseActionHelper(robotRef, event.GetData().Get_GotoPose());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::GotoObject:
    {
      newAction = GetDriveToObjectActionHelper(robotRef, event.GetData().Get_GotoObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::AlignWithObject:
    {
      newAction = GetDriveToAlignWithObjectActionHelper(robotRef, event.GetData().Get_AlignWithObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PickupObject:
    {
      numRetries = 0;
      newAction = GetPickupActionHelper(robotRef, event.GetData().Get_PickupObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlaceOnObject:
    {
      numRetries = 1;
      newAction = GetPlaceOnActionHelper(robotRef, event.GetData().Get_PlaceOnObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlaceRelObject:
    {
      numRetries = 1;
      newAction = GetPlaceRelActionHelper(robotRef, event.GetData().Get_PlaceRelObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::RollObject:
    {
      numRetries = 1;
      newAction = GetRollObjectActionHelper(robotRef, event.GetData().Get_RollObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PopAWheelie:
    {
      numRetries = 1;
      newAction = GetPopAWheelieActionHelper(robotRef, event.GetData().Get_PopAWheelie());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::MountCharger:
    {
      numRetries = 1;
      newAction = GetMountChargerActionHelper(robotRef, event.GetData().Get_MountCharger());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TraverseObject:
    {
      numRetries = 1;
      newAction = GetTraverseObjectActionHelper(robotRef, event.GetData().Get_TraverseObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlayAnimation:
    {
      const ExternalInterface::PlayAnimation& msg = event.GetData().Get_PlayAnimation();
      newAction = new PlayAnimationAction(robotRef, msg.animationName, msg.numLoops);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlayAnimationTrigger:
    {
      const ExternalInterface::PlayAnimationTrigger& msg = event.GetData().Get_PlayAnimationTrigger();
      newAction = new TriggerAnimationAction(robotRef, msg.trigger, msg.numLoops);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SearchForObject:
    {
      newAction = CreateSearchForObjectAction(robotRef, event.GetData().Get_SearchForObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsObject:
    {
      newAction = GetTurnTowardsObjectActionHelper(robotRef, event.GetData().Get_TurnTowardsObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsPose:
    {
      const ExternalInterface::TurnTowardsPose& facePose = event.GetData().Get_TurnTowardsPose();
      newAction = GetTurnTowardsPoseActionHelper(robotRef, facePose);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnInPlace:
    {
      newAction = GetTurnInPlaceActionHelper(robotRef, event.GetData().Get_TurnInPlace());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TrackToFace:
    {
      newAction = GetTrackFaceActionHelper(robotRef, event.GetData().Get_TrackToFace());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TrackToObject:
    {
      newAction = GetTrackObjectActionHelper(robotRef, event.GetData().Get_TrackToObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetHeadAngle:
    {
      newAction = GetMoveHeadToAngleActionHelper(robotRef, event.GetData().Get_SetHeadAngle());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetLiftHeight:
    {
      // Special case: doesn't use queuing below
      HandleMessage(event.GetData().Get_SetLiftHeight());
      return;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsFace:
    {
      newAction = GetTurnTowardsFaceActionHelper(robotRef, event.GetData().Get_TurnTowardsFace());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsLastFacePose:
    {
      newAction = GetTurnTowardsLastFacePoseActionHelper(robotRef, event.GetData().Get_TurnTowardsLastFacePose());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ReadToolCode:
    {
      newAction = new ReadToolCodeAction(robotRef);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::RealignWithObject:
    {
      numRetries = 1;
      newAction = GetRealignWithObjectActionHelper(robotRef, event.GetData().Get_RealignWithObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SayText:
    {
      const auto msg = event.GetData().Get_SayText();
      newAction = new SayTextAction(robotRef, msg.text, msg.voiceStyle, msg.durationScalar);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SayTextWithIntent:
    {
      const auto msg = event.GetData().Get_SayTextWithIntent();
      newAction = new SayTextAction(robotRef, msg.text, msg.intent);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::EnrollNamedFace:
    {
      newAction = GetEnrollNamedFaceActionHelper(robotRef, event.GetData().Get_EnrollNamedFace());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::FlipBlock:
    {
      auto& flipBlock = event.GetData().Get_FlipBlock();
      
      ObjectID selectedObjectID = flipBlock.objectID;
      if(selectedObjectID < 0) {
        selectedObjectID = robotRef.GetBlockWorld().GetSelectedObject();
      }
      
      DriveAndFlipBlockAction* action = new DriveAndFlipBlockAction(robotRef, selectedObjectID);
      
      if(flipBlock.motionProf.isCustom)
      {
        action->SetMotionProfile(flipBlock.motionProf);
      }
      newAction = action;
      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::Wait:
      newAction = new WaitAction(robotRef, event.GetData().Get_Wait().time_s);
      break;
      
    case ExternalInterface::MessageGameToEngineTag::WaitForImages:
    {
      const ExternalInterface::WaitForImages& waitMsg = event.GetData().Get_WaitForImages();
      newAction = new WaitForImagesAction(robotRef, waitMsg.numImages, waitMsg.visionMode, waitMsg.afterTimeStamp);
      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::VisuallyVerifyFace:
    {
      newAction = CreateVisuallyVerifyFaceAction(robotRef, event.GetData().Get_VisuallyVerifyFace());
      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::VisuallyVerifyObject:
    {
      newAction = CreateVisuallyVerifyObjectAction(robotRef, event.GetData().Get_VisuallyVerifyObject());
      break;
    }
      
    default:
    {
      PRINT_STREAM_ERROR("RobotEventHandler.HandleEvents",
                         "Subscribed to unhandled event of type "
                         << ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) << "!");
      
      // We don't know what to do; bail;
      return;
    }
  }
  
  if(_ignoreExternalActions)
  {
    PRINT_NAMED_INFO("RobotEventHandler.GameToEngineEvent",
                     "Ignoring GameToEngineEvent message while external actions are disabled");
    newAction->PrepForCompletion();
    Util::SafeDelete(newAction);
    return;
  }
  // Everything's ok and we have an action, so queue it
  robotRef.GetActionList().QueueAction(QueueActionPosition::NOW, newAction, numRetries);
}

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::QueueSingleAction& msg)
{
  // Can't queue actions for nonexistent robots...
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  IActionRunner* action = CreateNewActionByType(*robot, msg.action);
  
  const bool didSetTag = action->SetTag(msg.idTag);
  
  // If setting the tag failed then delete the action which will emit a completion signal indicating failure
  if(!didSetTag || _ignoreExternalActions)
  {
    if(_ignoreExternalActions)
    {
      PRINT_NAMED_WARNING("RobotEventHandler.HandleQueueSingleAction.IgnoringExternalActions",
                          "Ignoring QueueSingleAction message while external actions are disabled");
    }
    else
    {
      PRINT_NAMED_ERROR("RobotEventHandler.HandleQueueSingleAction.FailedToSetTag",
                        "Failed to set tag deleting action");
    }
    action->PrepForCompletion();
    Util::SafeDelete(action);
  }
  else
  {
    // Put the action in the given position of the specified queue
    robot->GetActionList().QueueAction(msg.position, action, msg.numRetries);
  }
}

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::QueueCompoundAction& msg)
{
  // Can't queue actions for nonexistent robots...
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  if(_ignoreExternalActions)
  {
    PRINT_NAMED_INFO("RobotEventHandler.QueueCompoundAction",
                     "Ignoring QueueCompoundAction message while external actions are disabled");
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
  for(size_t iAction=0; iAction < msg.actions.size(); ++iAction) {
    
    IActionRunner* action = CreateNewActionByType(*robot, msg.actions[iAction]);
    
    compoundAction->AddAction(action);
    
  } // for each action/actionType
  
  // If setting the tag failed then delete the action which will emit a completion signal indicating failure
  if(!compoundAction->SetTag(msg.idTag))
  {
    PRINT_NAMED_ERROR("RobotEventHandler.HandleQueueCompoundAction", "Failure to set action tag deleting action");
    Util::SafeDelete(compoundAction);
  }
  else
  {
    // Put the action in the given position of the specified queue
    robot->GetActionList().QueueAction(msg.position, compoundAction, msg.numRetries);
  }
}

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


template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::DisplayProceduralFace& msg)
{
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  robot->GetAnimationStreamer().GetLastProceduralFace()->SetFromMessage(msg);
}

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::ForceDelocalizeRobot& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot) {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleForceDelocalizeRobot.InvalidRobotID",
                        "Failed to find robot %d to delocalize.", robot->GetID());
      
  } else if(!robot->IsPhysical()) {
    PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.ForceDelocalize",
                     "Forcibly delocalizing robot %d", robot->GetID());
    
    robot->SendRobotMessage<RobotInterface::ForceDelocalizeSimulatedRobot>();
  } else {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleForceDelocalizeRobot.PhysicalRobot",
                        "Refusing to force delocalize physical robot.");
  }
}

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

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SetHeadlight& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSetHeadlight.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->SetHeadlight(msg.enable);
  }
}

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
      Quad2f objectFootprint = robot->GetBlockWorld().GetObjectByID(robot->GetCarryingObject())->GetBoundingQuadXY(targetPose);
      robot->GetContext()->GetVizManager()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
    }
  }
}

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::IMURequest& msg)
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
  
template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::EnableRobotPickupParalysis& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleEnableRobotPickupParalysis.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->SendRobotMessage<RobotInterface::EnablePickupParalysis>(msg.enable);
    //SendRobotMessage<RobotInterface::EnablePickupParalysis>(enable)
  }
}

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SetBackpackLEDs& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSetBackpackLEDs.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    robot->SetBackpackLights(msg.onColor, msg.offColor,
                             msg.onPeriod_ms, msg.offPeriod_ms,
                             msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms);
  }
}

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
      robot->UnSetCarryingObjects();
    } else {
      robot->SetCarryingObject(msg.objectID);
    }
  }
}

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

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SetActiveObjectLEDs& msg)
{
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSetActiveObjectLEDs.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    assert(msg.objectID <= s32_MAX);
    robot->SetObjectLights(msg.objectID,
                           msg.whichLEDs,
                           msg.onColor, msg.offColor,
                           msg.onPeriod_ms, msg.offPeriod_ms,
                           msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                           msg.turnOffUnspecifiedLEDs,
                           msg.makeRelative,
                           Point2f(msg.relativeToX, msg.relativeToY),
                           msg.rotationPeriod_ms);
  }
}

template<>
void RobotEventHandler::HandleMessage(const ExternalInterface::SetAllActiveObjectLEDs& msg)
{
  //const RobotID_t robotID = msg.robotID; This message contains garbage robot id. Requires fix.
  
  Robot* robot = _context->GetRobotManager()->GetFirstRobot();
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSetAllActiveObjectLEDss.InvalidRobotID", "Failed to find robot.");
  }
  else
  {
    assert(msg.objectID <= s32_MAX);
    robot->SetObjectLights(msg.objectID,
                           msg.onColor, msg.offColor,
                           msg.onPeriod_ms, msg.offPeriod_ms,
                           msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                           msg.onOffset, msg.offOffset,
                           msg.makeRelative, Point2f(msg.relativeToX, msg.relativeToY),
                           msg.rotationPeriod_ms);
  }
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
    
  }
}
  
} // namespace Cozmo
} // namespace Anki
