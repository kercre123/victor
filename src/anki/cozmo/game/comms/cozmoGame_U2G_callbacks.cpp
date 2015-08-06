/**
 * File: cozmoGame_U2G_callbacks.cpp
 *
 * Author: Andrew Stein
 * Date:   2/9/2015
 *
 * Description: Implements callbacks for U2G messages coming into CozmoGame
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "cozmoGame_impl.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"


namespace Anki {
namespace Cozmo {
 
  static const ActionList::SlotHandle DriveAndManipulateSlot = 0;
  
  /*
   *  Helper macro for generating a callback lambda that captures "this" and 
   *  calls the corresponding ProcessMessage method. For example, for a 
   *  ExternalInterface::FooBar, use REGISTER_CALLBACK(FooBar) and the following code
   *  is generated:
   *
   *    auto cbFooBar = [this](const ExternalInterface::FooBar& msg) {
   *      this->ProcessMessage(msg);
   *    };
   *    _uiMsgHandler.RegisterCallbackForU2G_FooBar(cbFooBar);
   * 
   * NOTE: For compactness, the lambda is not actually created and stored in
   *   cbFooBar. Instead, it is simply created inline within the RegisterCallback call.
   */
  void CozmoGameImpl::RegisterCallbacksU2G()
  {
    _uiMsgHandler.RegisterCallbackForMessage([this](const ExternalInterface::MessageGameToEngine& msg) {
      _uiMsgHandler.Broadcast(msg);
#include "clad/externalInterface/messageGameToEngine_switch.def"
    });
  } // RegisterCallbacksU2G()
  
  Robot* CozmoGameImpl::GetRobotByID(const RobotID_t robotID)
  {
    Robot* robot = nullptr;
    
    if(_isHost) {
      CozmoEngineHost* cozmoEngineHost = reinterpret_cast<CozmoEngineHost*>(_cozmoEngine);
      
      if(cozmoEngineHost == nullptr) {
        PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage",
                          "Could not reinterpret cozmoEngine as a cozmoEngineHost.\n");
        return nullptr;
      }
      
      robot = cozmoEngineHost->GetRobotByID(robotID);
      
      if(robot == nullptr) {
        PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage",
                          "No robot with ID=%d found.\n", robotID);
      }
      
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.GetRobotByID",
                        "Cannot get robot ID for game running as client.\n");
    }
    
    return robot;
  }
  
  void CozmoGameImpl::ProcessBadTag_MessageGameToEngine(ExternalInterface::MessageGameToEngine::Tag tag)
  {
    PRINT_STREAM_WARNING("CozmoGameImpl.ProcessBadTag",
                        "Got unknown message with id " << ExternalInterface::MessageGameToEngineTagToString(tag) << ".");
  }
  
  void CozmoGameImpl::Process_Ping(ExternalInterface::Ping const& msg)
  {
    
    _lastPingTimeFromUI_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    
    if (_uiComms.GetNumConnectedDevices() > 1) {
      // The ping counter check doesn't work for more than 1 UI client
      return;
    }
    
    // Check to see if the ping counter is sequential or if we've dropped packets/pings
    const u32 counterDiff = msg.counter - _lastPingCounterFromUI;
    _lastPingCounterFromUI = msg.counter;
    
    if(counterDiff > 1) {
      PRINT_STREAM_WARNING("CozmoGameImpl.Update",
                          "Counter difference > 1 betweeen last two pings from UI. (Difference was " << counterDiff << ".)\n");
      
      // TODO: Take action if we've dropped pings?
    }
    
  }
  
  void CozmoGameImpl::Process_ConnectToRobot(ExternalInterface::ConnectToRobot const& msg)
  {
    // Tell the game to connect to a robot, using a signal
    // CozmoGameSignals::ConnectToRobotSignal().emit(msg.robotID);
    
    const bool success = ConnectToRobot(msg.robotID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameImpl.ProcessMessage", "Connected to robot %d!\n", msg.robotID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage", "Failed to connect to robot %d!\n", msg.robotID);
    }
  }
  
  void CozmoGameImpl::Process_ConnectToUiDevice(ExternalInterface::ConnectToUiDevice const& msg)
  {
    // Tell the game to connect to a UI device, using a signal?
    // CozmoGameSignals::ConnectToUiDeviceSignal().emit(msg.deviceID);
    
    const bool success = ConnectToUiDevice(msg.deviceID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameImpl.ProcessMessage", "Connected to UI device %d!\n", msg.deviceID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage", "Failed to connect to UI device %d!\n", msg.deviceID);
    }
  }
  
  void CozmoGameImpl::Process_DisconnectFromUiDevice(ExternalInterface::DisconnectFromUiDevice const& msg)
  {
    // Do this with a signal?
    _uiComms.DisconnectDeviceByID(msg.deviceID);
    PRINT_NAMED_INFO("CozmoGameImpl.ProcessMessage", "Disconnected from UI device %d!\n", msg.deviceID);
    
    if(_uiComms.GetNumConnectedDevices() == 0) {
      PRINT_NAMED_INFO("CozmoGameImpl.ProcessMessage",
                       "Last UI device just disconnected: forcing re-initialization.\n");
      Init(_config);
    }
  }
  
  void CozmoGameImpl::Process_ForceAddRobot(ExternalInterface::ForceAddRobot const& msg)
  {
    char ip[16];
    assert(msg.ipAddress.size() <= 16);
    std::copy(msg.ipAddress.begin(), msg.ipAddress.end(), ip);
    ForceAddRobot(msg.robotID, ip, msg.isSimulated);
  }
  
  void CozmoGameImpl::Process_StartEngine(ExternalInterface::StartEngine const& msg)
  {
    if (_isEngineStarted) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_StartEngine.AlreadyStarted", "");
      return;
    }
    
    // Populate the Json configuration from the message members:
    Json::Value config;
    
    // Viz Host IP:
    char ip[16];
    assert(msg.vizHostIP.size() <= 16);
    std::copy(msg.vizHostIP.begin(), msg.vizHostIP.end(), ip);
    config[AnkiUtil::kP_VIZ_HOST_IP] = ip;
    
    config[AnkiUtil::kP_AS_HOST] = msg.asHost;
    
    // Start the engine with that configuration
    StartEngine(config);
    
  }
  
  void CozmoGameImpl::Process_DriveWheels(ExternalInterface::DriveWheels const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->AreWheelsLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_DriveWheels.WheelsLocked",
                         "Ignoring ExternalInterface::DriveWheels while wheels are locked.\n");
      } else {
        robot->DriveWheels(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
      }
    }
  }
  
  void CozmoGameImpl::Process_TurnInPlace(ExternalInterface::TurnInPlace const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new TurnInPlaceAction(msg.angle_rad));
    }
  }

  void CozmoGameImpl::Process_TurnInPlaceAtSpeed(ExternalInterface::TurnInPlaceAtSpeed const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
        robot->TurnInPlaceAtSpeed(msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
    }
  }
  
  void CozmoGameImpl::Process_MoveHead(ExternalInterface::MoveHead const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveHead.HeadLocked",
                         "Ignoring ExternalInterface::MoveHead while head is locked.\n");
      } else {
        robot->MoveHead(msg.speed_rad_per_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_MoveLift(ExternalInterface::MoveLift const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveLift.LiftLocked",
                         "Ignoring ExternalInterface::MoveLift while lift is locked.\n");
      } else {
        robot->MoveLift(msg.speed_rad_per_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_SetHeadAngle(ExternalInterface::SetHeadAngle const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_SetHeadAngle.HeadLocked",
                         "Ignoring ExternalInterface::SetHeadAngle while head is locked.\n");
      } else {
        robot->DisableTrackToObject();
        robot->MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2, msg.duration_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_TrackToObject(ExternalInterface::TrackToObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_TrackHeadToObject.HeadLocked",
                         "Ignoring ExternalInterface::TrackHeadToObject while head is locked.\n");
      } else {
        
        if(msg.objectID == u32_MAX) {
          robot->DisableTrackToObject();
        } else {
          robot->EnableTrackToObject(msg.objectID, msg.headOnly);
        }
      }
    }
  }
  
  IActionRunner* GetFaceObjectActionHelper(Robot* robot, ExternalInterface::FaceObject const& msg)
  {
    ObjectID objectID;
    if(msg.objectID == u32_MAX) {
      objectID = robot->GetBlockWorld().GetSelectedObject();
    } else {
      objectID = msg.objectID;
    }
    return new FaceObjectAction(objectID,
                                Radians(msg.turnAngleTol),
                                Radians(msg.maxTurnAngle),
                                msg.headTrackWhenDone);
  }
  
  void CozmoGameImpl::Process_FaceObject(ExternalInterface::FaceObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, GetFaceObjectActionHelper(robot, msg));
    }
  }
  
  void CozmoGameImpl::Process_StopAllMotors(ExternalInterface::StopAllMotors const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopAllMotors();
    }
  }
  
  void CozmoGameImpl::Process_SetLiftHeight(ExternalInterface::SetLiftHeight const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_SetLiftHeight.LiftLocked",
                         "Ignoring ExternalInterface::SetLiftHeight while lift is locked.\n");
      } else {
        // Special case if commanding low dock height
        if (msg.height_mm == LIFT_HEIGHT_LOWDOCK) {
          if(robot->IsCarryingObject()) {
            // Put the block down right here
            ExternalInterface::PlaceObjectOnGroundHere m;
            Process_PlaceObjectOnGroundHere(m);
            return;
          }
        }
        
        robot->MoveLiftToHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2, msg.duration_sec);
      }
    }
  }

  void CozmoGameImpl::Process_TapBlockOnGround(ExternalInterface::TapBlockOnGround const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_TapBlockOnGround.LiftLocked",
                         "Ignoring ExternalInterface::TapBlockOnGround while lift is locked.\n");
      } else {
        robot->TapBlockOnGround(msg.numTaps);
      }
    }
  }
  
  void CozmoGameImpl::Process_SetRobotImageSendMode(ExternalInterface::SetRobotImageSendMode const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      if (msg.mode == ISM_OFF) {
        robot->GetBlockWorld().EnableDraw(false);
      } else if (msg.mode == ISM_STREAM) {
        robot->GetBlockWorld().EnableDraw(true);
      }
      
      robot->RequestImage((ImageSendMode_t)msg.mode,
                          (Vision::CameraResolution)msg.resolution);
    }
  }
  
  void CozmoGameImpl::Process_ImageRequest(ExternalInterface::ImageRequest const& msg)
  {
    SetImageSendMode(msg.robotID, static_cast<ImageSendMode_t>(msg.mode));
  }
  
  void CozmoGameImpl::Process_SaveImages(ExternalInterface::SaveImages const& msg)
  {
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      printf("Saving image mode %d\n", msg.mode);
      robot->SetSaveImageMode((SaveMode_t)msg.mode);
    }
  }
  
  void CozmoGameImpl::Process_SaveRobotState(ExternalInterface::SaveRobotState const& msg)
  {
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
    printf("Saving robot state: %d\n", msg.mode);
      robot->SetSaveStateMode((SaveMode_t)msg.mode);
    }
  }
  
  void CozmoGameImpl::Process_EnableDisplay(ExternalInterface::EnableDisplay const& msg)
  {
    VizManager::getInstance()->ShowObjects(msg.enable);
  }
  
  void CozmoGameImpl::Process_SetHeadlights(ExternalInterface::SetHeadlights const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetHeadlight(msg.intensity);
    }
  }
  
  IActionRunner* GetDriveToPoseActionHelper(Robot* robot, ExternalInterface::GotoPose const& msg)
  {
    // TODO: Add ability to indicate z too!
    // TODO: Better way to specify the target pose's parent
    Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0), robot->GetWorldOrigin());
    targetPose.SetName("GotoPoseTarget");
    
    // TODO: expose whether or not to drive with head down in message?
    // (For now it is hard-coded to true)
    const bool driveWithHeadDown = true;
    return new DriveToPoseAction(targetPose, driveWithHeadDown, msg.useManualSpeed);
  }
  
  void CozmoGameImpl::Process_GotoPose(ExternalInterface::GotoPose const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, GetDriveToPoseActionHelper(robot, msg));
    }
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGround(ExternalInterface::PlaceObjectOnGround const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 1;
      
      // Create an action to drive to specied pose and then put down the carried
      // object.
      // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
      Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 22.f), robot->GetWorldOrigin());
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new PlaceObjectOnGroundAtPoseAction(*robot, targetPose, msg.useManualSpeed), numRetries);
    }
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGroundHere(ExternalInterface::PlaceObjectOnGroundHere const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new PlaceObjectOnGroundAction());
    }
  }
  
  void CozmoGameImpl::Process_ExecuteTestPlan(ExternalInterface::ExecuteTestPlan const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ExecuteTestPath();
    }
  }
  
  void CozmoGameImpl::Process_SetRobotCarryingObject(ExternalInterface::SetRobotCarryingObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    if(robot != nullptr) {
      if(msg.objectID < 0) {
        robot->UnSetCarryingObject();
      } else {
        ObjectID whichObject;
        whichObject = msg.objectID;
        robot->SetCarryingObject(whichObject);
      }
    }
  }
  
  void CozmoGameImpl::Process_ClearAllBlocks(ExternalInterface::ClearAllBlocks const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::ACTIVE_BLOCKS);
    }
  }
  
  void CozmoGameImpl::Process_ClearAllObjects(ExternalInterface::ClearAllObjects const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearAllExistingObjects();
    }
  }
  
  void CozmoGameImpl::Process_SetObjectAdditionAndDeletion(ExternalInterface::SetObjectAdditionAndDeletion const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetBlockWorld().EnableObjectAddition(msg.enableAddition);
      robot->GetBlockWorld().EnableObjectDeletion(msg.enableDeletion);
    }
  }
  
  void CozmoGameImpl::Process_SelectNextObject(ExternalInterface::SelectNextObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetBlockWorld().CycleSelectedObject();
    }
  }
 
  IActionRunner* GetPickAndPlaceActionHelper(Robot* robot, ExternalInterface::PickAndPlaceObject const& msg)
  {
    ObjectID selectedObjectID;
    if(msg.objectID < 0) {
      selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
    } else {
      selectedObjectID = msg.objectID;
    }
    
    if(static_cast<bool>(msg.usePreDockPose)) {
      return new DriveToPickAndPlaceObjectAction(selectedObjectID, msg.useManualSpeed);
    } else {
      PickAndPlaceObjectAction* action = new PickAndPlaceObjectAction(selectedObjectID, msg.useManualSpeed);
      action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
      return action;
    }
  }
  
  void CozmoGameImpl::Process_PickAndPlaceObject(ExternalInterface::PickAndPlaceObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 1;
      
      IActionRunner* action = GetPickAndPlaceActionHelper(robot, msg);
      
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, action, numRetries);
    }
  }
  
  IActionRunner* GetDriveToObjectActionHelper(Robot* robot, ExternalInterface::GotoObject const& msg)
  {
    ObjectID selectedObjectID;
    if(msg.objectID < 0) {
      selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
    } else {
      selectedObjectID = msg.objectID;
    }
    
    return new DriveToObjectAction(selectedObjectID, msg.distance_mm, msg.useManualSpeed);
  }
  
  void CozmoGameImpl::Process_GotoObject(ExternalInterface::GotoObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 0;
      
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, GetDriveToObjectActionHelper(robot, msg), numRetries);
    }
  }

  IActionRunner* GetRollObjectActionHelper(Robot* robot, ExternalInterface::RollObject const& msg)
  {
    assert(robot != nullptr); // should've already been checked!
    
    ObjectID selectedObjectID;
    if(msg.objectID < 0) {
      selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
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
  
  
  void CozmoGameImpl::Process_RollObject(ExternalInterface::RollObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 1;
      
      IActionRunner* action = GetRollObjectActionHelper(robot, msg);
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, action, numRetries);
    }
  }
  
  void CozmoGameImpl::Process_TraverseObject(ExternalInterface::TraverseObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      const u8 numRetries = 0;
      
      ObjectID selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      
      if(static_cast<bool>(msg.usePreDockPose)) {
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new DriveToAndTraverseObjectAction(selectedObjectID, msg.useManualSpeed), numRetries);
      } else {
        TraverseObjectAction* action = new TraverseObjectAction(selectedObjectID, msg.useManualSpeed);
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, action, numRetries);
      }
      
    }
  }
  
  
  void CozmoGameImpl::Process_ExecuteBehavior(ExternalInterface::ExecuteBehavior const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartBehaviorMode(static_cast<BehaviorManager::Mode>(msg.behaviorMode));
    }
  }
  
  void CozmoGameImpl::Process_SetBehaviorState(ExternalInterface::SetBehaviorState const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetBehaviorState(static_cast<BehaviorManager::BehaviorState>(msg.behaviorState));
    }
  }
  
  void CozmoGameImpl::Process_AbortPath(ExternalInterface::AbortPath const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ClearPath();
    }
  }
  
  void CozmoGameImpl::Process_AbortAll(ExternalInterface::AbortAll const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->AbortAll();
    }
  }
  
  void CozmoGameImpl::Process_CancelAction(ExternalInterface::CancelAction const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().Cancel(-1, (RobotActionType)msg.actionType);
    }
  }
  
  void CozmoGameImpl::Process_DrawPoseMarker(ExternalInterface::DrawPoseMarker const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr && robot->IsCarryingObject()) {
      Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0));
      Quad2f objectFootprint = robot->GetBlockWorld().GetObjectByID(robot->GetCarryingObject())->GetBoundingQuadXY(targetPose);
      VizManager::getInstance()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
    }
  }
  
  void CozmoGameImpl::Process_ErasePoseMarker(ExternalInterface::ErasePoseMarker const& msg)
  {
    VizManager::getInstance()->EraseAllQuadsWithType(VIZ_QUAD_POSE_MARKER);
  }

  void CozmoGameImpl::Process_SetWheelControllerGains(ExternalInterface::SetWheelControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetWheelControllerGains(msg.kpLeft, msg.kiLeft, msg.maxIntegralErrorLeft,
                                     msg.kpRight, msg.kiRight, msg.maxIntegralErrorRight);
    }
  }

  
  void CozmoGameImpl::Process_SetHeadControllerGains(ExternalInterface::SetHeadControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetHeadControllerGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
    }
  }
  
  void CozmoGameImpl::Process_SetLiftControllerGains(ExternalInterface::SetLiftControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetLiftControllerGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
    }
  }

  void CozmoGameImpl::Process_SetSteeringControllerGains(ExternalInterface::SetSteeringControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetSteeringControllerGains(msg.k1, msg.k2);
    }
  }
  
  void CozmoGameImpl::Process_SetRobotVolume(ExternalInterface::SetRobotVolume const& msg)
  {
    SoundManager::getInstance()->SetRobotVolume(msg.volume);
  }
  
  void CozmoGameImpl::Process_StartTestMode(ExternalInterface::StartTestMode const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartTestMode((TestMode)msg.mode, msg.p1, msg.p2, msg.p3);
    }
  }
  
  void CozmoGameImpl::Process_IMURequest(ExternalInterface::IMURequest const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->RequestIMU(msg.length_ms);
    }
  }
  
  void CozmoGameImpl::Process_PlayAnimation(ExternalInterface::PlayAnimation const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    Robot* robot = GetRobotByID(msg.robotID);

    if(robot != nullptr) {
      //robot->PlayAnimation(&(msg.animationName[0]), msg.numLoops);
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot,
                                              new PlayAnimationAction(msg.animationName,
                                                                      msg.numLoops));
    }
  }
  
  void CozmoGameImpl::Process_SetIdleAnimation(ExternalInterface::SetIdleAnimation const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->SetIdleAnimation(msg.animationName);
    }
  }

  void CozmoGameImpl::Process_ReplayLastAnimation(ExternalInterface::ReplayLastAnimation const& msg)
  {
    PRINT_NAMED_INFO("CozmoGame.ReadAnimationFile", "replaying last animation");
    Robot* robot = GetRobotByID(msg.robotID);
    if (robot != nullptr) {
      robot->ReplayLastAnimation(msg.numLoops);
    }
  }

  void CozmoGameImpl::Process_ReadAnimationFile(ExternalInterface::ReadAnimationFile const& msg)
  {
    PRINT_NAMED_INFO("CozmoGame.ReadAnimationFile", "started animation tool");
    _cozmoEngine->StartAnimationTool();
  }
  
  void CozmoGameImpl::Process_StartFaceTracking(ExternalInterface::StartFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartFaceTracking(msg.timeout_sec);
    }
  }
  
  void CozmoGameImpl::Process_StopFaceTracking(ExternalInterface::StopFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopFaceTracking();
    }
  }
  
  void CozmoGameImpl::Process_SetVisionSystemParams(ExternalInterface::SetVisionSystemParams const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      VisionSystemParams_t p;
      p.autoexposureOn = msg.autoexposureOn;
      p.exposureTime = msg.exposureTime;
      p.integerCountsIncrement = msg.integerCountsIncrement;
      p.minExposureTime = msg.minExposureTime;
      p.maxExposureTime = msg.maxExposureTime;
      p.percentileToMakeHigh = msg.percentileToMakeHigh;
      p.highValue = msg.highValue;
      p.limitFramerate = msg.limitFramerate;
      
      robot->SendVisionSystemParams(p);
    }
  }
  
  void CozmoGameImpl::Process_SetFaceDetectParams(ExternalInterface::SetFaceDetectParams const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      FaceDetectParams_t p;
      p.scaleFactor = msg.scaleFactor;
      p.minNeighbors = msg.minNeighbors;
      p.minObjectHeight = msg.minObjectHeight;
      p.minObjectWidth = msg.minObjectWidth;
      p.maxObjectHeight = msg.maxObjectHeight;
      p.maxObjectWidth = msg.maxObjectWidth;
      
      robot->SendFaceDetectParams(p);
    }
  }
  
  void CozmoGameImpl::Process_StartLookingForMarkers(ExternalInterface::StartLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartLookingForMarkers();
    }
  }
  
  void CozmoGameImpl::Process_StopLookingForMarkers(ExternalInterface::StopLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopLookingForMarkers();
    }
  }
  
  
  void CozmoGameImpl::Process_SetActiveObjectLEDs(ExternalInterface::SetActiveObjectLEDs const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      assert(msg.objectID <= s32_MAX);
      ObjectID whichObject;
      whichObject = msg.objectID;
      
      MakeRelativeMode makeRelative = static_cast<MakeRelativeMode>(msg.makeRelative);
      robot->SetObjectLights(whichObject,
                             static_cast<WhichBlockLEDs>(msg.whichLEDs),
                             msg.onColor, msg.offColor, msg.onPeriod_ms, msg.offPeriod_ms,
                             msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                             msg.turnOffUnspecifiedLEDs,
                             makeRelative, Point2f(msg.relativeToX, msg.relativeToY));
      
      /*
      ActiveCube* activeCube = robot->GetActiveObject(whichObject);
      if(activeCube != nullptr) {
        activeCube->SetLEDs(static_cast<ActiveCube::WhichBlockLEDs>(msg.whichLEDs),
                            msg.color, msg.onPeriod_ms, msg.offPeriod_ms);
        
        if(msg.makeRelative) {
          activeCube->MakeStateRelativeToXY(Point2f(msg.relativeToX, msg.relativeToY));
        }
        
             }
       */
    }
  }
  
  void CozmoGameImpl::Process_SetAllActiveObjectLEDs(ExternalInterface::SetAllActiveObjectLEDs const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      assert(msg.objectID <= s32_MAX);
      ObjectID whichObject;
      whichObject = msg.objectID;
      MakeRelativeMode makeRelative = static_cast<MakeRelativeMode>(msg.makeRelative);
      robot->SetObjectLights(whichObject,
                             msg.onColor, msg.offColor,
                             msg.onPeriod_ms, msg.offPeriod_ms,
                             msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                             makeRelative, Point2f(msg.relativeToX, msg.relativeToY));
    }
  }

  void CozmoGameImpl::Process_VisionWhileMoving(ExternalInterface::VisionWhileMoving const& msg)
  {
    if(_isHost) {
      CozmoEngineHost* cozmoEngineHost = reinterpret_cast<CozmoEngineHost*>(_cozmoEngine);
      
      if(cozmoEngineHost == nullptr) {
        PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage",
                          "Could not reinterpret cozmoEngine as a cozmoEngineHost.\n");
      }
      
      const std::vector<RobotID_t>& robotIDs = cozmoEngineHost->GetRobotIDList();
      
      for(auto robotID : robotIDs) {
        Robot* robot = cozmoEngineHost->GetRobotByID(robotID);
        if(robot == nullptr) {
          PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage",
                            "No robot with ID=%d found, even though it is in the ID list.\n", robotID);
        } else {
          robot->EnableVisionWhileMoving(msg.enable);
        }
      }
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.ProcessMessage.VisionWhileMoving",
                        "Cannot process VisionWhileMoving message on a client engine.\n");
    }
  }
  
  void CozmoGameImpl::Process_SetBackpackLEDs(ExternalInterface::SetBackpackLEDs const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->SetBackpackLights(msg.onColor, msg.offColor,
                               msg.onPeriod_ms, msg.offPeriod_ms,
                               msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms);
    }
  }
  
  void CozmoGameImpl::Process_VisualizeQuad(ExternalInterface::VisualizeQuad const& msg)
  {
    const Quad3f quad({msg.xUpperLeft,  msg.yUpperLeft,  msg.zUpperLeft},
                      {msg.xUpperRight, msg.yUpperRight, msg.zUpperRight},
                      {msg.xLowerLeft,  msg.yLowerLeft,  msg.zLowerLeft},
                      {msg.xLowerRight, msg.yLowerRight, msg.zLowerRight});
    
    VizManager::getInstance()->DrawGenericQuad(msg.quadID, quad, msg.color);
  }
  
  void CozmoGameImpl::Process_EraseQuad(ExternalInterface::EraseQuad const& msg)
  {
    VizManager::getInstance()->EraseQuad(VIZ_QUAD_GENERIC_3D, msg.quadID);
  }
  
  
  static void QueueActionHelper(const QueueActionPosition position,
                                const u32 idTag, const u32 inSlot, const u8 numRetries,
                                ActionList& actionList, IActionRunner* action)
  {
    action->SetTag(idTag);
    
    switch(position)
    {
      case QueueActionPosition::NOW:
        actionList.QueueActionNow(inSlot, action, numRetries);
        break;
     
      case QueueActionPosition::NOW_AND_CLEAR_REMAINING:
        // Empty the queue entirely and make this action the next thing in it
        actionList.Clear();
        actionList.QueueActionNext(inSlot, action, numRetries);
        break;

      case QueueActionPosition::NEXT:
        actionList.QueueActionNext(inSlot, action, numRetries);
        break;
        
      case QueueActionPosition::AT_END:
        actionList.QueueActionAtEnd(inSlot, action, numRetries);
        break;
        
      default:
        PRINT_NAMED_ERROR("CozmoGameImpl.QueueActionHelper.InvalidPosition",
                          "Unrecognized 'position' for queuing action.\n");
        return;
    }
  }
  
  IActionRunner* CreateNewActionByType(Robot* robot,
                                       const RobotActionType actionType,
                                       const ExternalInterface::RobotActionUnion& actionUnion)
  {
    switch(actionType)
    {
      case RobotActionType::TURN_IN_PLACE:
        return new TurnInPlaceAction(actionUnion.turnInPlace.angle_rad);
        
      case RobotActionType::PLAY_ANIMATION:
        return new PlayAnimationAction(actionUnion.playAnimation.animationName);
        
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
        
      case RobotActionType::ROLL_OBJECT_LOW:
        return GetRollObjectActionHelper(robot, actionUnion.rollObject);
      
      case RobotActionType::DRIVE_TO_OBJECT:
        return GetDriveToObjectActionHelper(robot, actionUnion.goToObject);
        
      case RobotActionType::DRIVE_TO_POSE:
        return GetDriveToPoseActionHelper(robot, actionUnion.goToPose);
        
        // TODO: Add cases for other actions
        
      default:
        PRINT_NAMED_ERROR("CozmoGameImpl.CreateNewActionByType.InvalidActionType",
                          "Failed to create an action for the given actionType.\n");
        return nullptr;
    }
  }
  
  void CozmoGameImpl::Process_QueueSingleAction(const ExternalInterface::QueueSingleAction &msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      IActionRunner* action = CreateNewActionByType(robot, msg.actionType, msg.action);
      
      // Put the action in the given position of the specified queue:
      QueueActionHelper(msg.position, msg.idTag, msg.inSlot, msg.numRetries, robot->GetActionList(), action);
      
    }
    
  }
  
  void CozmoGameImpl::Process_QueueCompoundAction(const ExternalInterface::QueueCompoundAction &msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    if(robot != nullptr) {
      
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
        
        IActionRunner* action = CreateNewActionByType(robot, msg.actionTypes[iAction], msg.actions[iAction]);
        
        compoundAction->AddAction(action);
        
      } // for each action/actionType
      
      // Put the action in the given position of the specified queue:
      QueueActionHelper(msg.position, msg.idTag, msg.inSlot, msg.numRetries,
                        robot->GetActionList(), compoundAction);
      
    }
  }
  
}
}
