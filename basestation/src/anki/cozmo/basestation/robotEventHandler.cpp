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
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/progressionSystem/progressionManager.h"
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
  if (_context->GetExternalInterface() != nullptr)
  {
    // We'll use this callback for simple events we care about
    auto actionEventCallback = std::bind(&RobotEventHandler::HandleActionEvents, this, std::placeholders::_1);
    
    std::vector<ExternalInterface::MessageGameToEngineTag> tagList =
    {
      ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGround,
      ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGroundHere,
      ExternalInterface::MessageGameToEngineTag::GotoPose,
      ExternalInterface::MessageGameToEngineTag::GotoObject,
      ExternalInterface::MessageGameToEngineTag::AlignWithObject,
      ExternalInterface::MessageGameToEngineTag::PickupObject,
      ExternalInterface::MessageGameToEngineTag::PlaceOnObject,
      ExternalInterface::MessageGameToEngineTag::PlaceRelObject,
      ExternalInterface::MessageGameToEngineTag::RollObject,
      ExternalInterface::MessageGameToEngineTag::PopAWheelie,
      ExternalInterface::MessageGameToEngineTag::TraverseObject,
      ExternalInterface::MessageGameToEngineTag::MountCharger,
      ExternalInterface::MessageGameToEngineTag::PlayAnimation,
      ExternalInterface::MessageGameToEngineTag::TurnTowardsObject,
      ExternalInterface::MessageGameToEngineTag::TurnTowardsPose,
      ExternalInterface::MessageGameToEngineTag::TurnInPlace,
      ExternalInterface::MessageGameToEngineTag::TrackToObject,
      ExternalInterface::MessageGameToEngineTag::TrackToFace,
      ExternalInterface::MessageGameToEngineTag::SetHeadAngle,
      ExternalInterface::MessageGameToEngineTag::PanAndTilt,
      ExternalInterface::MessageGameToEngineTag::TurnTowardsLastFacePose,
    };
    
    // Subscribe to desired events
    for (auto tag : tagList)
    {
      _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(tag, actionEventCallback));
    }
    
    // Custom handler for QueueSingleAction
    auto queueSingleActionCallback = std::bind(&RobotEventHandler::HandleQueueSingleAction, this, std::placeholders::_1);
    _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::QueueSingleAction, queueSingleActionCallback));
    
    // Custom handler for QueueCompoundAction
    auto queueCompoundActionCallback = std::bind(&RobotEventHandler::HandleQueueCompoundAction, this, std::placeholders::_1);
    _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::QueueCompoundAction, queueCompoundActionCallback));
    
    // Custom handler for SetLiftHeight
    auto setLiftHeightCallback = std::bind(&RobotEventHandler::HandleSetLiftHeight, this, std::placeholders::_1);
    _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::SetLiftHeight, setLiftHeightCallback));
    
    // Custom handler for EnableLiftPower
    auto enableLiftPowerCallback = std::bind(&RobotEventHandler::HandleEnableLiftPower, this, std::placeholders::_1);
    _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::EnableLiftPower, enableLiftPowerCallback));
    
    // Custom handler for DisplayProceduralFace
    auto dispProcFaceCallback = std::bind(&RobotEventHandler::HandleDisplayProceduralFace, this, std::placeholders::_1);
    _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::DisplayProceduralFace, dispProcFaceCallback));
    
    // Custom handler for ForceDelocalizeRobot
    auto delocalizeCallabck = std::bind(&RobotEventHandler::HandleForceDelocalizeRobot, this, std::placeholders::_1);
    _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ForceDelocalizeRobot, delocalizeCallabck));
    
    // Custom handlers for Mood events
    {
      auto moodEventCallback = std::bind(&RobotEventHandler::HandleMoodEvent, this, std::placeholders::_1);
      _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::MoodMessage, moodEventCallback));
    }

    // Custom handlers for Progression events
    {
      auto progressionEventCallback = std::bind(&RobotEventHandler::HandleProgressionEvent, this, std::placeholders::_1);
      _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ProgressionMessage, progressionEventCallback));
    }

    // Custom handlers for BehaviorManager events
    {
      auto eventCallback = std::bind(&RobotEventHandler::HandleBehaviorManagerEvent, this, std::placeholders::_1);
      _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::BehaviorManagerMessage, eventCallback));
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
                                             msg.motionProf,
                                             msg.useExactRotation,
                                             msg.useManualSpeed);
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
  
  return new DriveToPoseAction(robot,
                               targetPose,
                               msg.motionProf,
                               driveWithHeadDown,
                               msg.useManualSpeed);
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
    return new DriveToPickupObjectAction(robot,
                                         selectedObjectID,
                                         msg.motionProf,
                                         msg.useApproachAngle,
                                         msg.approachAngle_rad,
                                         msg.useManualSpeed);
  } else {
    PickupObjectAction* action = new PickupObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2);
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
    return new DriveToPlaceRelObjectAction(robot,
                                           selectedObjectID,
                                           msg.motionProf,
                                           msg.placementOffsetX_mm,
                                           msg.useApproachAngle,
                                           msg.approachAngle_rad,
                                           msg.useManualSpeed);
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

    return new DriveToPlaceOnObjectAction(robot,
                                          selectedObjectID,
                                          msg.motionProf,
                                          msg.useApproachAngle,
                                          msg.approachAngle_rad,
                                          msg.useManualSpeed);
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
  
  return new DriveToObjectAction(robot,
                                 selectedObjectID,
                                 msg.distanceFromObjectOrigin_mm,
                                 msg.motionProf,
                                 msg.useManualSpeed);
}

IActionRunner* GetDriveToAlignWithObjectActionHelper(Robot& robot, const ExternalInterface::AlignWithObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  return new DriveToAlignWithObjectAction(robot,
                                          selectedObjectID,
                                          msg.distanceFromMarker_mm,
                                          msg.motionProf,
                                          msg.useApproachAngle,
                                          msg.approachAngle_rad,
                                          msg.useManualSpeed);
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
    return new DriveToRollObjectAction(robot,
                                       selectedObjectID,
                                       msg.motionProf,
                                       msg.useApproachAngle,
                                       msg.approachAngle_rad,
                                       msg.useManualSpeed);
  } else {
    RollObjectAction* action = new RollObjectAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2);
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
    return new DriveToPopAWheelieAction(robot,
                                        selectedObjectID,
                                        msg.motionProf,
                                        msg.useApproachAngle,
                                        msg.approachAngle_rad,
                                        msg.useManualSpeed);
  } else {
    PopAWheelieAction* action = new PopAWheelieAction(robot, selectedObjectID, msg.useManualSpeed);
    action->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2);
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}

  
IActionRunner* GetTraverseObjectActionHelper(Robot& robot, const ExternalInterface::TraverseObject& msg)
{
  ObjectID selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    return new DriveToAndTraverseObjectAction(robot,
                                              selectedObjectID,
                                              msg.motionProf,
                                              msg.useManualSpeed);
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
    return new DriveToAndMountChargerAction(robot,
                                            selectedObjectID,
                                            msg.motionProf,
                                            msg.useManualSpeed);
  } else {
    MountChargerAction* chargerAction = new MountChargerAction(robot, selectedObjectID, msg.useManualSpeed);
    chargerAction->SetSpeedAndAccel(msg.motionProf.dockSpeed_mmps, msg.motionProf.dockAccel_mmps2);
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

      // TODO: Add cases for other actions
      
    default:
      PRINT_NAMED_ERROR("RobotEventHandler.CreateNewActionByType.InvalidActionTag",
                        "Failed to create an action for the given actionTag.");
      return nullptr;
  }
}
  
void RobotEventHandler::HandleActionEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
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
    case ExternalInterface::MessageGameToEngineTag::TurnTowardsLastFacePose:
    {
      newAction = GetTurnTowardsLastFacePoseActionHelper(robot, event.GetData().Get_TurnTowardsLastFacePose());
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
  
void RobotEventHandler::HandleQueueSingleAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
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
  
void RobotEventHandler::HandleQueueCompoundAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
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
  
void RobotEventHandler::HandleSetLiftHeight(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
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

void RobotEventHandler::HandleEnableLiftPower(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
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

  
void RobotEventHandler::HandleDisplayProceduralFace(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
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
  
  void RobotEventHandler::HandleForceDelocalizeRobot(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
  {
    RobotID_t robotID = event.GetData().Get_ForceDelocalizeRobot().robotID;

    Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
    
    // We need a robot
    if (nullptr == robot) {
      PRINT_NAMED_ERROR("RobotEventHandler.HandleForceDelocalizeRobot.InvalidRobotID",
                        "Failed to find robot %d to delocalize.", robotID);
      
      
    } else {
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.ForceDelocalize",
                       "Forcibly delocalizing robot %d", robotID);
      
      robot->Delocalize();
    }
  }
  
void RobotEventHandler::HandleMoodEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& eventData = event.GetData();
  const RobotID_t robotID = eventData.Get_MoodMessage().robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_ERROR("RobotEventHandler.HandleMoodEvent.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetMoodManager().HandleEvent(event);
  }
}
  
void RobotEventHandler::HandleProgressionEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& eventData = event.GetData();
  const RobotID_t robotID = eventData.Get_ProgressionMessage().robotID;
  
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_ERROR("RobotEventHandler.HandleProgressionEvent.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetProgressionManager().HandleEvent(event);
  }
}
  
void RobotEventHandler::HandleBehaviorManagerEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& eventData = event.GetData();
  const auto& message = eventData.Get_BehaviorManagerMessage();
  const RobotID_t robotID = message.robotID;

  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    PRINT_NAMED_ERROR("RobotEventHandler.HandleBehaviorManagerEvent.InvalidRobotID", "Failed to find robot %u.", robotID);
  }
  else
  {
    robot->GetBehaviorManager().HandleMessage(message.BehaviorManagerMessageUnion);
  }
}
  

} // namespace Cozmo
} // namespace Anki
