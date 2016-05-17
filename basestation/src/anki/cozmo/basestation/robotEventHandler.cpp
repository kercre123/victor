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
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/enrollNamedFaceAction.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/basestation/math/point_impl.h"
#include "clad/externalInterface/messageGameToEngine.h"
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
      MessageGameToEngineTag::PlayAnimationGroup,
      MessageGameToEngineTag::PopAWheelie,
      MessageGameToEngineTag::ReadToolCode,
      MessageGameToEngineTag::RollObject,
      MessageGameToEngineTag::SayText,
      MessageGameToEngineTag::SetHeadAngle,
      MessageGameToEngineTag::SetLiftHeight,
      MessageGameToEngineTag::TrackToFace,
      MessageGameToEngineTag::TrackToObject,
      MessageGameToEngineTag::TraverseObject,
      MessageGameToEngineTag::TurnInPlace,
      MessageGameToEngineTag::TurnTowardsLastFacePose,
      MessageGameToEngineTag::TurnTowardsObject,
      MessageGameToEngineTag::TurnTowardsPose,
    };
    
    // Subscribe to all action events
    for (auto tag : actionTagList)
    {
      _signalHandles.push_back(externalInterface->Subscribe(tag, actionEventCallback));
    }
    
    // Custom handler for QueueSingleAction
    auto queueSingleActionCallback = std::bind(&RobotEventHandler::HandleQueueSingleAction, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::QueueSingleAction, queueSingleActionCallback));
    
    // Custom handler for QueueCompoundAction
    auto queueCompoundActionCallback = std::bind(&RobotEventHandler::HandleQueueCompoundAction, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::QueueCompoundAction, queueCompoundActionCallback));
    
    // Custom handler for EnableLiftPower
    auto enableLiftPowerCallback = std::bind(&RobotEventHandler::HandleEnableLiftPower, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::EnableLiftPower, enableLiftPowerCallback));

    // Custom handler for EnableCliffSensor
    auto enableCliffSensorCallback = std::bind(&RobotEventHandler::HandleEnableCliffSensor, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::EnableCliffSensor, enableCliffSensorCallback));

    // Custom handler for DisplayProceduralFace
    auto dispProcFaceCallback = std::bind(&RobotEventHandler::HandleDisplayProceduralFace, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::DisplayProceduralFace, dispProcFaceCallback));
    
    // Custom handler for ForceDelocalizeRobot
    auto delocalizeCallabck = std::bind(&RobotEventHandler::HandleForceDelocalizeRobot, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::ForceDelocalizeRobot, delocalizeCallabck));
    
    // Custom handler for SendAvailableObjects event
    auto sendAvailableObjectsCallback = std::bind(&RobotEventHandler::HandleSendAvailableObjects, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::SendAvailableObjects, sendAvailableObjectsCallback));

    // Custom handler for SaveCalibrationImage event
    auto saveCalibrationImageCallback = std::bind(&RobotEventHandler::HandleSaveCalibrationImage, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::SaveCalibrationImage, saveCalibrationImageCallback));
    
    // Custom handler for ClearCalibrationImages event
    auto clearCalibrationImagesCallback = std::bind(&RobotEventHandler::HandleClearCalibrationImages, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::ClearCalibrationImages, clearCalibrationImagesCallback));
    
    // Custom handler for ComputeCameraCalibration event
    auto computeCameraCalibrationCallback = std::bind(&RobotEventHandler::HandleComputeCameraCalibration, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::ComputeCameraCalibration, computeCameraCalibrationCallback));
    
    // Custom handler for CameraCalibration event
    auto cameraCalibrationCallback = std::bind(&RobotEventHandler::HandleCameraCalibration, this, std::placeholders::_1);
    _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::CameraCalibration, cameraCalibrationCallback));

    // Custom handlers for BehaviorManager events
    {
      auto eventCallback = std::bind(&RobotEventHandler::HandleBehaviorManagerEvent, this, std::placeholders::_1);
      _signalHandles.push_back(externalInterface->Subscribe(MessageGameToEngineTag::BehaviorManagerMessage, eventCallback));
    }
        
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
                                                                          msg.placementOffsetX_mm,
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
  }
  else
  {
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
    if(msg.motionProf.isCustom)
    {
      action->SetMotionProfile(msg.motionProf);
    }
    return action;
  } else {
    RollObjectAction* action = new RollObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2, msg.motionProf.dockDecel_mmps2);
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
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
                                                  Radians(msg.maxTurnAngle),
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
  
  TurnTowardsPoseAction* action = new TurnTowardsPoseAction(robot, pose, Radians(msg.maxTurnAngle));
  
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
  TurnTowardsLastFacePoseAction* action = new TurnTowardsLastFacePoseAction(robot, Radians(msg.maxTurnAngle));
  
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
    case RobotActionUnionTag::playAnimationGroup:
    {
      auto & playAnimationGroup = actionUnion.Get_playAnimationGroup();
      return new PlayAnimationGroupAction(robot, playAnimationGroup.animationGroupName, playAnimationGroup.numLoops);
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
      
    case RobotActionUnionTag::goToObject:
      return GetDriveToObjectActionHelper(robot, actionUnion.Get_goToObject());
      
    case RobotActionUnionTag::goToPose:
      return GetDriveToPoseActionHelper(robot, actionUnion.Get_goToPose());

    case RobotActionUnionTag::alignWithObject:
      return GetDriveToAlignWithObjectActionHelper(robot, actionUnion.Get_alignWithObject());

    case RobotActionUnionTag::trackFace:
      return GetTrackFaceActionHelper(robot, actionUnion.Get_trackFace());
      
    case RobotActionUnionTag::trackObject:
      return GetTrackObjectActionHelper(robot, actionUnion.Get_trackObject());
      
    case RobotActionUnionTag::driveStraight:
      return new DriveStraightAction(robot, actionUnion.Get_driveStraight().dist_mm,
                                     actionUnion.Get_driveStraight().speed_mmps);
      
    case RobotActionUnionTag::panAndTilt:
      return new PanAndTiltAction(robot, Radians(actionUnion.Get_panAndTilt().bodyPan),
                                  Radians(actionUnion.Get_panAndTilt().headTilt),
                                  actionUnion.Get_panAndTilt().isPanAbsolute,
                                  actionUnion.Get_panAndTilt().isTiltAbsolute);
      
    case RobotActionUnionTag::wait:
      return new WaitAction(robot, actionUnion.Get_wait().time_s);

    case RobotActionUnionTag::mountCharger:
      return GetMountChargerActionHelper(robot, actionUnion.Get_mountCharger());
      
    case RobotActionUnionTag::turnTowardsLastFacePose:
      return new TurnTowardsLastFacePoseAction(robot, actionUnion.Get_turnTowardsLastFacePose().maxTurnAngle);

    case RobotActionUnionTag::searchSideToSide:
      return new SearchSideToSideAction(robot);

    case RobotActionUnionTag::readToolCode:
      return new ReadToolCodeAction(robot);
      
    case RobotActionUnionTag::sayText:
    {
      SayTextAction* sayTextAction = new SayTextAction(robot, actionUnion.Get_sayText().text, actionUnion.Get_sayText().style, true);
      //sayTextAction->SetGameEvent(actionUnion.Get_sayText().playEvent);
      return sayTextAction;
    }
      
    case RobotActionUnionTag::enrollNamedFace:
    {
      auto & enrollNamedFace = actionUnion.Get_enrollNamedFace();
      EnrollNamedFaceAction* action =  new EnrollNamedFaceAction(robot, enrollNamedFace.faceID, enrollNamedFace.name);
      action->SetSequenceType(enrollNamedFace.sequence);
      action->EnableSaveToRobot(enrollNamedFace.saveToRobot);
      return action;
    }
      
      // TODO: Add cases for other actions
      
    default:
      PRINT_NAMED_ERROR("RobotEventHandler.CreateNewActionByType.InvalidActionTag",
                        "Failed to create an action for the given actionTag.");
      return nullptr;
  }
}
  
void RobotEventHandler::HandleActionEvents(const GameToEngineEvent& event)
{
  RobotID_t robotID = 1; // We init the robotID to 1
  Robot* robotPointer = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // If we don't have a valid robot there's nothing to do
  if (nullptr == robotPointer)
  {
    return;
  }
  
  // We'll pass around a reference to the Robot for convenience sake
  Robot& robot = *robotPointer;
  
  // Now we fill out our Action and possibly update number of retries:
  IActionRunner* newAction = nullptr;
  u8 numRetries = 0;
  switch (event.GetData().GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGround:
    {
      numRetries = 1;
      newAction = GetPlaceObjectOnGroundActionHelper(robot, event.GetData().Get_PlaceObjectOnGround());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGroundHere:
    {
      newAction = new PlaceObjectOnGroundAction(robot);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::GotoPose:
    {
      numRetries = 2;
      newAction = GetDriveToPoseActionHelper(robot, event.GetData().Get_GotoPose());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::GotoObject:
    {
      newAction = GetDriveToObjectActionHelper(robot, event.GetData().Get_GotoObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::AlignWithObject:
    {
      newAction = GetDriveToAlignWithObjectActionHelper(robot, event.GetData().Get_AlignWithObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PickupObject:
    {
      numRetries = 0;
      newAction = GetPickupActionHelper(robot, event.GetData().Get_PickupObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlaceOnObject:
    {
      numRetries = 1;
      newAction = GetPlaceOnActionHelper(robot, event.GetData().Get_PlaceOnObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlaceRelObject:
    {
      numRetries = 1;
      newAction = GetPlaceRelActionHelper(robot, event.GetData().Get_PlaceRelObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::RollObject:
    {
      numRetries = 1;
      newAction = GetRollObjectActionHelper(robot, event.GetData().Get_RollObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PopAWheelie:
    {
      numRetries = 1;
      newAction = GetPopAWheelieActionHelper(robot, event.GetData().Get_PopAWheelie());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::MountCharger:
    {
      numRetries = 1;
      newAction = GetMountChargerActionHelper(robot, event.GetData().Get_MountCharger());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TraverseObject:
    {
      numRetries = 1;
      newAction = GetTraverseObjectActionHelper(robot, event.GetData().Get_TraverseObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlayAnimation:
    {
      const ExternalInterface::PlayAnimation& msg = event.GetData().Get_PlayAnimation();
      newAction = new PlayAnimationAction(robot, msg.animationName, msg.numLoops);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlayAnimationGroup:
    {
      const ExternalInterface::PlayAnimationGroup& msg = event.GetData().Get_PlayAnimationGroup();
      newAction = new PlayAnimationGroupAction(robot, msg.animationGroupName, msg.numLoops);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsObject:
    {
      newAction = GetTurnTowardsObjectActionHelper(robot, event.GetData().Get_TurnTowardsObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsPose:
    {
      const ExternalInterface::TurnTowardsPose& facePose = event.GetData().Get_TurnTowardsPose();
      newAction = GetTurnTowardsPoseActionHelper(robot, facePose);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnInPlace:
    {
      newAction = GetTurnInPlaceActionHelper(robot, event.GetData().Get_TurnInPlace());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TrackToFace:
    {
      newAction = GetTrackFaceActionHelper(robot, event.GetData().Get_TrackToFace());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TrackToObject:
    {
      newAction = GetTrackObjectActionHelper(robot, event.GetData().Get_TrackToObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetHeadAngle:
    {
      newAction = GetMoveHeadToAngleActionHelper(robot, event.GetData().Get_SetHeadAngle());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetLiftHeight:
    {
      // Special case: doesn't use queuing below
      HandleSetLiftHeight(event);
      return;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsLastFacePose:
    {
      newAction = GetTurnTowardsLastFacePoseActionHelper(robot, event.GetData().Get_TurnTowardsLastFacePose());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ReadToolCode:
    {
      newAction = new ReadToolCodeAction(robot);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SayText:
    {
      newAction = new SayTextAction(robot, event.GetData().Get_SayText().text, event.GetData().Get_SayText().style, true);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::EnrollNamedFace:
    {
      auto & enrollNamedFace = event.GetData().Get_EnrollNamedFace();
      EnrollNamedFaceAction* enrollAction = new EnrollNamedFaceAction(robot, enrollNamedFace.faceID, enrollNamedFace.name);
      enrollAction->SetSequenceType(enrollNamedFace.sequence);
      enrollAction->EnableSaveToRobot(enrollNamedFace.saveToRobot);
      newAction = enrollAction;
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
  
  // Everything's ok and we have an action, so queue it
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, newAction, numRetries);
}
  
void RobotEventHandler::HandleQueueSingleAction(const GameToEngineEvent& event)
{
  const ExternalInterface::QueueSingleAction& msg = event.GetData().Get_QueueSingleAction();
  
  // Can't queue actions for nonexistent robots...
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  IActionRunner* action = CreateNewActionByType(*robot, msg.action);
  
  // If setting the tag failed then delete the action which will emit a completion signal indicating failure
  if(!action->SetTag(msg.idTag))
  {
    PRINT_NAMED_ERROR("RobotEventHandler.HandleQueueSingleAction", "Failure to set action tag deleting action");
    Util::SafeDelete(action);
  }
  else
  {
    // Put the action in the given position of the specified queue
    robot->GetActionList().QueueAction(msg.position, action, msg.numRetries);
  }
}
  
void RobotEventHandler::HandleQueueCompoundAction(const GameToEngineEvent& event)
{
  const ExternalInterface::QueueCompoundAction& msg = event.GetData().Get_QueueCompoundAction();
  
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
  
void RobotEventHandler::HandleSetLiftHeight(const GameToEngineEvent& event)
{
  // TODO: get RobotID in a non-hack way
  RobotID_t robotID = 1;
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  if(robot->GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK)) {
    PRINT_NAMED_INFO("RobotEventHandler.HandleSetLiftHeight.LiftLocked",
                     "Ignoring ExternalInterface::SetLiftHeight while lift is locked.");
  } else {
    const ExternalInterface::SetLiftHeight& msg = event.GetData().Get_SetLiftHeight();
    
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

void RobotEventHandler::HandleEnableLiftPower(const GameToEngineEvent& event)
{
  // TODO: get RobotID in a non-hack way
  RobotID_t robotID = 1;
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  if(robot->GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK)) {
    PRINT_NAMED_INFO("RobotEventHandler.HandleEnableLiftPower.LiftLocked",
                     "Ignoring ExternalInterface::EnableLiftPower while lift is locked.");
  } else {
    const ExternalInterface::EnableLiftPower& msg = event.GetData().Get_EnableLiftPower();
    robot->GetMoveComponent().EnableLiftPower(msg.enable);
  }
}


void RobotEventHandler::HandleEnableCliffSensor(const GameToEngineEvent& event)
{
  // TODO: get RobotID in a non-hack way
  RobotID_t robotID = 1;
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  if (nullptr != robot)
  {
    const ExternalInterface::EnableCliffSensor& msg = event.GetData().Get_EnableCliffSensor();
    robot->SetEnableCliffSensor(msg.enable);
  }
}
  
void RobotEventHandler::HandleDisplayProceduralFace(const GameToEngineEvent& event)
{
  const ExternalInterface::DisplayProceduralFace& msg = event.GetData().Get_DisplayProceduralFace();

  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  robot->GetAnimationStreamer().GetLastProceduralFace()->SetFromMessage(msg);
}
  
void RobotEventHandler::HandleForceDelocalizeRobot(const GameToEngineEvent& event)
{
  RobotID_t robotID = event.GetData().Get_ForceDelocalizeRobot().robotID;

  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
    
  // We need a robot
  if (nullptr == robot) {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleForceDelocalizeRobot.InvalidRobotID",
                        "Failed to find robot %d to delocalize.", robotID);
      
      
  } else {
    PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.ForceDelocalize",
                     "Forcibly delocalizing robot %d", robotID);
      
    robot->Delocalize();
  }
}
  
void RobotEventHandler::HandleBehaviorManagerEvent(const GameToEngineEvent& event)
{
  const auto& eventData = event.GetData();
  const auto& message = eventData.Get_BehaviorManagerMessage();
  const RobotID_t robotID = message.robotID;

  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleBehaviorManagerEvent.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetBehaviorManager().HandleMessage(message.BehaviorManagerMessageUnion);
  }
}

void RobotEventHandler::HandleSendAvailableObjects(const GameToEngineEvent& event)
{

  const auto& eventData = event.GetData();
  const auto& message = eventData.Get_SendAvailableObjects();
  const RobotID_t robotID = message.robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_WARNING("RobotEventHandler.HandleSendAvailableObjects.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->BroadcastAvailableObjects(message.enable);
  }

}
  
  void RobotEventHandler::HandleSaveCalibrationImage(const GameToEngineEvent& event)
  {
    
    const auto& eventData = event.GetData();
    const auto& message = eventData.Get_SaveCalibrationImage();
    const RobotID_t robotID = message.robotID;
    
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
  
  void RobotEventHandler::HandleClearCalibrationImages(const GameToEngineEvent& event)
  {
    
    const auto& eventData = event.GetData();
    const auto& message = eventData.Get_ClearCalibrationImages();
    const RobotID_t robotID = message.robotID;
    
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
  
  void RobotEventHandler::HandleComputeCameraCalibration(const GameToEngineEvent& event)
  {
    
    const auto& eventData = event.GetData();
    const auto& message = eventData.Get_ComputeCameraCalibration();
    const RobotID_t robotID = message.robotID;
    
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
  
  void RobotEventHandler::HandleCameraCalibration(const GameToEngineEvent& event)
  {
    // TODO: get RobotID in a non-hack way
    RobotID_t robotID = 1;
    Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
    
    // We need a robot
    if (nullptr == robot)
    {
      PRINT_NAMED_WARNING("RobotEventHandler.HandleCameraCalibration.InvalidRobotID", "Failed to find robot %u.", robotID);
    }
    else
    {
      CameraCalibration calib = event.GetData().Get_CameraCalibration();
      std::vector<u8> calibVec(calib.Size());
      calib.Pack(calibVec.data(), calib.Size());
      robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CameraCalibration, calibVec.data(), calibVec.size());
      
      PRINT_NAMED_INFO("RobotEventHandler.HandleCameraCalibration.SendingCalib",
                       "fx: %f, fy: %f, cx: %f, cy: %f, nrows %d, ncols %d",
                       calib.focalLength_x, calib.focalLength_y, calib.center_x, calib.center_y, calib.nrows, calib.ncols);
      
    }
  }

  
} // namespace Cozmo
} // namespace Anki
