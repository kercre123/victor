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
#include "anki/cozmo/basestation/actionInterface.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
static const ActionList::SlotHandle DriveAndManipulateSlot = 0;

RobotEventHandler::RobotEventHandler(RobotManager& manager, IExternalInterface* interface)
  : _robotManager(manager)
  , _externalInterface(interface)
{
  if (_externalInterface != nullptr)
  {
    // We'll use this callback for simple events we care about
    auto actionEventCallback = std::bind(&RobotEventHandler::HandleActionEvents, this, std::placeholders::_1);
    
    std::vector<ExternalInterface::MessageGameToEngineTag> tagList =
    {
      ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGround,
      ExternalInterface::MessageGameToEngineTag::PlaceObjectOnGroundHere,
      ExternalInterface::MessageGameToEngineTag::GotoPose,
      ExternalInterface::MessageGameToEngineTag::GotoObject,
      ExternalInterface::MessageGameToEngineTag::PickAndPlaceObject,
      ExternalInterface::MessageGameToEngineTag::RollObject,
      ExternalInterface::MessageGameToEngineTag::TraverseObject,
      ExternalInterface::MessageGameToEngineTag::PlayAnimation,
      ExternalInterface::MessageGameToEngineTag::FaceObject,
      ExternalInterface::MessageGameToEngineTag::FacePose,
      ExternalInterface::MessageGameToEngineTag::TurnInPlace,
    };
    
    // Subscribe to desired events
    for (auto tag : tagList)
    {
      _signalHandles.push_back(_externalInterface->Subscribe(tag, actionEventCallback));
    }
    
    // Custom handler for QueueSingleAction
    auto queueSingleActionCallback = std::bind(&RobotEventHandler::HandleQueueSingleAction, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::QueueSingleAction, queueSingleActionCallback));
    
    // Custom handler for QueueCompoundAction
    auto queueCompoundActionCallback = std::bind(&RobotEventHandler::HandleQueueCompoundAction, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::QueueCompoundAction, queueCompoundActionCallback));
    
    // Custom handler for SetLiftHeight
    auto setLiftHeightCallback = std::bind(&RobotEventHandler::HandleSetLiftHeight, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::SetLiftHeight, setLiftHeightCallback));
    
    // Custom handler for DisplayProceduralFace
    auto dispProcFaceCallback = std::bind(&RobotEventHandler::HandleDisplayProceduralFace, this, std::placeholders::_1);
    _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::DisplayProceduralFace, dispProcFaceCallback));
  }
}
  
IActionRunner* GetPlaceObjectOnGroundActionHelper(Robot& robot, const ExternalInterface::PlaceObjectOnGround& msg)
{
  // Create an action to drive to specied pose and then put down the carried
  // object.
  // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
  Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 22.f), robot.GetWorldOrigin());
  return new PlaceObjectOnGroundAtPoseAction(robot, targetPose, msg.useManualSpeed);
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
  return new DriveToPoseAction(targetPose, driveWithHeadDown, msg.useManualSpeed);
}
  
IActionRunner* GetPickAndPlaceActionHelper(Robot& robot, const ExternalInterface::PickAndPlaceObject& msg)
{
  ObjectID selectedObjectID;
  if(msg.objectID < 0) {
    selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    selectedObjectID = msg.objectID;
  }
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    return new DriveToPickAndPlaceObjectAction(selectedObjectID,
                                               msg.useManualSpeed,
                                               msg.placementOffsetX_mm,
                                               msg.placementOffsetY_mm,
                                               msg.placementOffsetAngle_rad,
                                               msg.placeOnGroundIfCarrying);
  } else {
    PickAndPlaceObjectAction* action = new PickAndPlaceObjectAction(selectedObjectID,
                                                                    msg.useManualSpeed,
                                                                    msg.placementOffsetX_mm,
                                                                    msg.placementOffsetY_mm,
                                                                    msg.placementOffsetAngle_rad,
                                                                    msg.placeOnGroundIfCarrying);
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
  
  return new DriveToObjectAction(selectedObjectID, msg.distance_mm, msg.useManualSpeed);
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
    return new DriveToRollObjectAction(selectedObjectID, msg.useManualSpeed);
  } else {
    RollObjectAction* action = new RollObjectAction(selectedObjectID, msg.useManualSpeed);
    action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
    return action;
  }
}
  
IActionRunner* GetTraverseObjectActionHelper(Robot& robot, const ExternalInterface::TraverseObject& msg)
{
  ObjectID selectedObjectID = robot.GetBlockWorld().GetSelectedObject();
  
  if(static_cast<bool>(msg.usePreDockPose)) {
    return new DriveToAndTraverseObjectAction(selectedObjectID, msg.useManualSpeed);
  } else {
    return new TraverseObjectAction(selectedObjectID, msg.useManualSpeed);
  }
}
  
IActionRunner* GetFaceObjectActionHelper(Robot& robot, const ExternalInterface::FaceObject& msg)
{
  ObjectID objectID;
  if(msg.objectID == u32_MAX) {
    objectID = robot.GetBlockWorld().GetSelectedObject();
  } else {
    objectID = msg.objectID;
  }
  return new FaceObjectAction(objectID,
                              Radians(msg.turnAngleTol),
                              Radians(msg.maxTurnAngle),
                              msg.visuallyVerifyWhenDone,
                              msg.headTrackWhenDone);
}
  
IActionRunner* GetFacePoseActionHelper(Robot& robot, const ExternalInterface::FacePose& facePose)
{
  Pose3d pose(0, Z_AXIS_3D(), {facePose.world_x, facePose.world_y, facePose.world_z},
              robot.GetWorldOrigin());
  return new FacePoseAction(pose,
                            Radians(facePose.turnAngleTol),
                            Radians(facePose.maxTurnAngle));
}
  
IActionRunner* CreateNewActionByType(Robot& robot,
                                     const RobotActionType actionType,
                                     const ExternalInterface::RobotActionUnion& actionUnion)
{
  switch(actionType)
  {
    case RobotActionType::TURN_IN_PLACE:
      return new TurnInPlaceAction(actionUnion.turnInPlace.angle_rad, actionUnion.turnInPlace.isAbsolute);
      
    case RobotActionType::PLAY_ANIMATION:
      return new PlayAnimationAction(actionUnion.playAnimation.animationName, actionUnion.playAnimation.numLoops);
      
    case RobotActionType::PICK_AND_PLACE_OBJECT:
    case RobotActionType::PICKUP_OBJECT_HIGH:
    case RobotActionType::PICKUP_OBJECT_LOW:
    case RobotActionType::PLACE_OBJECT_HIGH:
    case RobotActionType::PLACE_OBJECT_LOW:
      return GetPickAndPlaceActionHelper(robot, actionUnion.pickAndPlaceObject);
      
    case RobotActionType::MOVE_HEAD_TO_ANGLE:
      // TODO: Provide a means to pass in the speed/acceleration values to the action
      return new MoveHeadToAngleAction(actionUnion.setHeadAngle.angle_rad);
      
    case RobotActionType::MOVE_LIFT_TO_HEIGHT:
      // TODO: Provide a means to pass in the speed/acceleration values to the action
      return new MoveLiftToHeightAction(actionUnion.setLiftHeight.height_mm);
      
    case RobotActionType::FACE_OBJECT:
      return GetFaceObjectActionHelper(robot, actionUnion.faceObject);
      
    case RobotActionType::FACE_POSE:
      return GetFacePoseActionHelper(robot, actionUnion.facePose);
      
    case RobotActionType::ROLL_OBJECT_LOW:
      return GetRollObjectActionHelper(robot, actionUnion.rollObject);
      
    case RobotActionType::DRIVE_TO_OBJECT:
      return GetDriveToObjectActionHelper(robot, actionUnion.goToObject);
      
    case RobotActionType::DRIVE_TO_POSE:
      return GetDriveToPoseActionHelper(robot, actionUnion.goToPose);
      
      // TODO: Add cases for other actions
      
    default:
      PRINT_NAMED_ERROR("RobotEventHandler.CreateNewActionByType.InvalidActionType",
                        "Failed to create an action for the given actionType.");
      return nullptr;
  }
}
  
void RobotEventHandler::QueueActionHelper(const QueueActionPosition position, const u32 idTag, const u32 inSlot,
                                          ActionList& actionList, IActionRunner* action, const u8 numRetries/* = 0*/)
{
  action->SetTag(idTag);
  
  QueueActionHelper(position, inSlot, actionList, action, numRetries);
}

void RobotEventHandler::QueueActionHelper(const QueueActionPosition position, const u32 inSlot,
                                          ActionList& actionList, IActionRunner* action, const u8 numRetries/* = 0*/)
{
  switch(position)
  {
    case QueueActionPosition::NOW:
    {
      actionList.QueueActionNow(inSlot, action, numRetries);
      break;
    }
    case QueueActionPosition::NOW_AND_CLEAR_REMAINING:
    {
      // Cancel all queued actions and make this action the next thing in it
      actionList.Cancel();
      actionList.QueueActionNext(inSlot, action, numRetries);
      break;
    }
    case QueueActionPosition::NEXT:
    {
      actionList.QueueActionNext(inSlot, action, numRetries);
      break;
    }
    case QueueActionPosition::AT_END:
    {
      actionList.QueueActionAtEnd(inSlot, action, numRetries);
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("CozmoGameImpl.QueueActionHelper.InvalidPosition",
                        "Unrecognized 'position' for queuing action.\n");
      return;
    }
  }
}
  
void RobotEventHandler::HandleActionEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  RobotID_t robotID = 1; // We init the robotID to 1
  Robot* robotPointer = _robotManager.GetRobotByID(robotID);
  
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
      newAction = new PlaceObjectOnGroundAction();
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::GotoPose:
    {
      newAction = GetDriveToPoseActionHelper(robot, event.GetData().Get_GotoPose());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::GotoObject:
    {
      newAction = GetDriveToObjectActionHelper(robot, event.GetData().Get_GotoObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PickAndPlaceObject:
    {
      numRetries = 1;
      newAction = GetPickAndPlaceActionHelper(robot, event.GetData().Get_PickAndPlaceObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::RollObject:
    {
      numRetries = 1;
      newAction = GetRollObjectActionHelper(robot, event.GetData().Get_RollObject());
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
      newAction = new PlayAnimationAction(msg.animationName, msg.numLoops);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::FaceObject:
    {
      newAction = GetFaceObjectActionHelper(robot, event.GetData().Get_FaceObject());
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::FacePose:
    {
      const ExternalInterface::FacePose& facePose = event.GetData().Get_FacePose();
      newAction = GetFacePoseActionHelper(robot, facePose);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::TurnInPlace:
    {
      newAction = new TurnInPlaceAction(event.GetData().Get_TurnInPlace().angle_rad,
                                        event.GetData().Get_TurnInPlace().isAbsolute);
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
  QueueActionHelper(QueueActionPosition::AT_END, DriveAndManipulateSlot, robot.GetActionList(), newAction, numRetries);
}
  
void RobotEventHandler::HandleQueueSingleAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const ExternalInterface::QueueSingleAction& msg = event.GetData().Get_QueueSingleAction();
  
  // Can't queue actions for nonexistent robots...
  Robot* robot = _robotManager.GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  IActionRunner* action = CreateNewActionByType(*robot, msg.actionType, msg.action);
  
  // Put the action in the given position of the specified queue:
  QueueActionHelper(msg.position, msg.idTag, msg.inSlot, robot->GetActionList(), action, msg.numRetries);
}
  
void RobotEventHandler::HandleQueueCompoundAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const ExternalInterface::QueueCompoundAction& msg = event.GetData().Get_QueueCompoundAction();
  
  // Can't queue actions for nonexistent robots...
  Robot* robot = _robotManager.GetRobotByID(msg.robotID);
  if (nullptr == robot)
  {
    return;
  }
  
  // Create an empty parallel or sequential compound action:
  ICompoundAction* compoundAction = nullptr;
  if(msg.parallel) {
    compoundAction = new CompoundActionParallel();
  } else {
    compoundAction = new CompoundActionSequential();
  }
  
  // Make sure sizes match
  if(msg.actions.size() != msg.actionTypes.size()) {
    PRINT_NAMED_ERROR("CozmoGameImpl.Process_QueueCompoundAction.MismatchedSizes",
                      "Number of actions (%lu) and actionTypes (%lu) should match!\n",
                      msg.actions.size(), msg.actionTypes.size());
    return;
  }
  
  // Add all the actions in the message to the compound action, according
  // to their type
  for(size_t iAction=0; iAction < msg.actions.size(); ++iAction) {
    
    IActionRunner* action = CreateNewActionByType(*robot, msg.actionTypes[iAction], msg.actions[iAction]);
    
    compoundAction->AddAction(action);
    
  } // for each action/actionType
  
  // Put the action in the given position of the specified queue:
  QueueActionHelper(msg.position, msg.idTag, msg.inSlot, robot->GetActionList(),
                    compoundAction, msg.numRetries);
}
  
void RobotEventHandler::HandleSetLiftHeight(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  // TODO: get RobotID in a non-hack way
  RobotID_t robotID = 1;
  Robot* robot = _robotManager.GetRobotByID(robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  if(robot->IsLiftLocked()) {
    PRINT_NAMED_INFO("RobotEventHandler.HandleSetLiftHeight.LiftLocked",
                     "Ignoring ExternalInterface::SetLiftHeight while lift is locked.");
  } else {
    const ExternalInterface::SetLiftHeight& msg = event.GetData().Get_SetLiftHeight();
    
    // Special case if commanding low dock height
    if (msg.height_mm == LIFT_HEIGHT_LOWDOCK && robot->IsCarryingObject()) {
      
      // Put the block down right here
      IActionRunner* newAction = new PlaceObjectOnGroundAction();
      QueueActionHelper(QueueActionPosition::AT_END, DriveAndManipulateSlot, robot->GetActionList(), newAction);
    }
    else {
      // In the normal case directly set the lift height
      robot->MoveLiftToHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2, msg.duration_sec);
    }
  }
}
  
void RobotEventHandler::HandleDisplayProceduralFace(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const ExternalInterface::DisplayProceduralFace& msg = event.GetData().Get_DisplayProceduralFace();

  Robot* robot = _robotManager.GetRobotByID(msg.robotID);
  
  // We need a robot
  if (nullptr == robot)
  {
    return;
  }
  
  ProceduralFace procFace;
  using Param = ProceduralFace::Parameter;
  const size_t N = static_cast<size_t>(Param::NumParameters);
  if(msg.leftEye.size() < N || msg.rightEye.size() < N) {
    PRINT_NAMED_ERROR("RobotEventHandler.HandleDisplayProceduralFace.WrongArrayLength",
                      "Expecting leftEye / rightEye array lengths to be %lu, not %lu / %lu.",
                      N, msg.leftEye.size(), msg.rightEye.size());
    return;
  }
    
  for(int iParam = 0; iParam < N; ++iParam) {
    procFace.SetParameter(ProceduralFace::Left,  static_cast<Param>(iParam), msg.leftEye[iParam]);
    procFace.SetParameter(ProceduralFace::Right, static_cast<Param>(iParam), msg.rightEye[iParam]);
  }
  
  procFace.SetFaceAngle(msg.faceAngle);
  procFace.SetTimeStamp(robot->GetLastMsgTimestamp());
  
  robot->SetProceduralFace(procFace);
}

} // namespace Cozmo
} // namespace Anki