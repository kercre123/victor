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

#include "anki/cozmo/game/signals/cozmoGameSignals.h"


namespace Anki {
namespace Cozmo {
 
  static const ActionList::SlotHandle DriveAndManipulateSlot = 0;
  
  /*
   *  Helper macro for generating a callback lambda that captures "this" and 
   *  calls the corresponding ProcessMessage method. For example, for a 
   *  U2G::FooBar, use REGISTER_CALLBACK(FooBar) and the following code
   *  is generated:
   *
   *    auto cbFooBar = [this](const U2G::FooBar& msg) {
   *      this->ProcessMessage(msg);
   *    };
   *    _uiMsgHandler.RegisterCallbackForU2G_FooBar(cbFooBar);
   * 
   * NOTE: For compactness, the lambda is not actually created and stored in
   *   cbFooBar. Instead, it is simply created inline within the RegisterCallback call.
   */
  void CozmoGameImpl::RegisterCallbacksU2G()
  {
    _uiMsgHandler.RegisterCallbackForMessage([this](const U2G::Message& msg) {
#include "anki/cozmo/messageBuffers/game/UiMessagesU2G_switch.def"
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
  
  void CozmoGameImpl::ProcessBadTag_Message(U2G::Message::Tag tag)
  {
    PRINT_STREAM_WARNING("CozmoGameImpl.ProcessBadTag",
                        "Got unknown message with id " << U2G::MessageTagToString(tag) << ".");
  }
  
  void CozmoGameImpl::Process_Ping(U2G::Ping const& msg)
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
  
  void CozmoGameImpl::Process_ConnectToRobot(U2G::ConnectToRobot const& msg)
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
  
  void CozmoGameImpl::Process_ConnectToUiDevice(U2G::ConnectToUiDevice const& msg)
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
  
  void CozmoGameImpl::Process_DisconnectFromUiDevice(U2G::DisconnectFromUiDevice const& msg)
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
  
  void CozmoGameImpl::Process_ForceAddRobot(U2G::ForceAddRobot const& msg)
  {
    char ip[16];
    assert(msg.ipAddress.size() <= 16);
    std::copy(msg.ipAddress.begin(), msg.ipAddress.end(), ip);
    ForceAddRobot(msg.robotID, ip, msg.isSimulated);
  }
  
  void CozmoGameImpl::Process_StartEngine(U2G::StartEngine const& msg)
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
  
  void CozmoGameImpl::Process_DriveWheels(U2G::DriveWheels const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->AreWheelsLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_DriveWheels.WheelsLocked",
                         "Ignoring U2G::DriveWheels while wheels are locked.\n");
      } else {
        robot->DriveWheels(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
      }
    }
  }
  
  void CozmoGameImpl::Process_TurnInPlace(U2G::TurnInPlace const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new TurnInPlaceAction(msg.angle_rad));
    }
  }
  
  void CozmoGameImpl::Process_MoveHead(U2G::MoveHead const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveHead.HeadLocked",
                         "Ignoring U2G::MoveHead while head is locked.\n");
      } else {
        robot->MoveHead(msg.speed_rad_per_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_MoveLift(U2G::MoveLift const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveLift.LiftLocked",
                         "Ignoring U2G::MoveLift while lift is locked.\n");
      } else {
        robot->MoveLift(msg.speed_rad_per_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_SetHeadAngle(U2G::SetHeadAngle const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_SetHeadAngle.HeadLocked",
                         "Ignoring U2G::SetHeadAngle while head is locked.\n");
      } else {
        robot->DisableTrackToObject();
        robot->MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
      }
    }
  }
  
  void CozmoGameImpl::Process_TrackToObject(U2G::TrackToObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_TrackHeadToObject.HeadLocked",
                         "Ignoring U2G::TrackHeadToObject while head is locked.\n");
      } else {
        
        if(msg.objectID == u32_MAX) {
          robot->DisableTrackToObject();
        } else {
          robot->EnableTrackToObject(msg.objectID, msg.headOnly);
        }
      }
    }
  }
  
  
  void CozmoGameImpl::Process_FaceObject(U2G::FaceObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      ObjectID objectID;
      if(msg.objectID == u32_MAX) {
        objectID = robot->GetBlockWorld().GetSelectedObject();
      } else {
        objectID = msg.objectID;
      }
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new FaceObjectAction(objectID,
                                                                                           Radians(msg.turnAngleTol),
                                                                                           Radians(msg.maxTurnAngle),
                                                                                           msg.headTrackWhenDone));
    }
  }
  
  void CozmoGameImpl::Process_StopAllMotors(U2G::StopAllMotors const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopAllMotors();
    }
  }
  
  void CozmoGameImpl::Process_SetLiftHeight(U2G::SetLiftHeight const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_SetLiftHeight.LiftLocked",
                         "Ignoring U2G::SetLiftHeight while lift is locked.\n");
      } else {
        // Special case if commanding low dock height
        if (msg.height_mm == LIFT_HEIGHT_LOWDOCK) {
          if(robot->IsCarryingObject()) {
            // Put the block down right here
            U2G::PlaceObjectOnGroundHere m;
            Process_PlaceObjectOnGroundHere(m);
            return;
          }
        }
        
        robot->MoveLiftToHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
      }
    }
  }

  void CozmoGameImpl::Process_TapBlockOnGround(U2G::TapBlockOnGround const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_TapBlockOnGround.LiftLocked",
                         "Ignoring U2G::TapBlockOnGround while lift is locked.\n");
      } else {
        robot->TapBlockOnGround(msg.numTaps);
      }
    }
  }
  
  void CozmoGameImpl::Process_SetRobotImageSendMode(U2G::SetRobotImageSendMode const& msg)
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
  
  void CozmoGameImpl::Process_ImageRequest(U2G::ImageRequest const& msg)
  {
    SetImageSendMode(msg.robotID, static_cast<ImageSendMode_t>(msg.mode));
  }
  
  void CozmoGameImpl::Process_SaveImages(U2G::SaveImages const& msg)
  {
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      printf("Saving image mode %d\n", msg.mode);
      robot->SetSaveImageMode((SaveMode_t)msg.mode);
    }
  }
  
  void CozmoGameImpl::Process_SaveRobotState(U2G::SaveRobotState const& msg)
  {
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
    printf("Saving robot state: %d\n", msg.mode);
      robot->SetSaveStateMode((SaveMode_t)msg.mode);
    }
  }
  
  void CozmoGameImpl::Process_EnableDisplay(U2G::EnableDisplay const& msg)
  {
    VizManager::getInstance()->ShowObjects(msg.enable);
  }
  
  void CozmoGameImpl::Process_SetHeadlights(U2G::SetHeadlights const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetHeadlight(msg.intensity);
    }
  }
  
  void CozmoGameImpl::Process_GotoPose(U2G::GotoPose const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      // TODO: Add ability to indicate z too!
      // TODO: Better way to specify the target pose's parent
      Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0), robot->GetWorldOrigin());
      targetPose.SetName("GotoPoseTarget");
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new DriveToPoseAction(targetPose, msg.useManualSpeed));
    }
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGround(U2G::PlaceObjectOnGround const& msg)
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
  
  void CozmoGameImpl::Process_PlaceObjectOnGroundHere(U2G::PlaceObjectOnGroundHere const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new PlaceObjectOnGroundAction());
    }
  }
  
  void CozmoGameImpl::Process_ExecuteTestPlan(U2G::ExecuteTestPlan const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ExecuteTestPath();
    }
  }
  
  void CozmoGameImpl::Process_SetRobotCarryingObject(U2G::SetRobotCarryingObject const& msg)
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
  
  void CozmoGameImpl::Process_ClearAllBlocks(U2G::ClearAllBlocks const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::ACTIVE_BLOCKS);
    }
  }
  
  void CozmoGameImpl::Process_ClearAllObjects(U2G::ClearAllObjects const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearAllExistingObjects();
    }
  }
  
  void CozmoGameImpl::Process_SetObjectAdditionAndDeletion(U2G::SetObjectAdditionAndDeletion const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetBlockWorld().EnableObjectAddition(msg.enableAddition);
      robot->GetBlockWorld().EnableObjectDeletion(msg.enableDeletion);
    }
  }
  
  void CozmoGameImpl::Process_SelectNextObject(U2G::SelectNextObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetBlockWorld().CycleSelectedObject();
    }
  }
  
  void CozmoGameImpl::Process_PickAndPlaceObject(U2G::PickAndPlaceObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 1;
      
      ObjectID selectedObjectID;
      if(msg.objectID < 0) {
        selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      } else {
        selectedObjectID = msg.objectID;
      }
      
      if(static_cast<bool>(msg.usePreDockPose)) {
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new DriveToPickAndPlaceObjectAction(selectedObjectID, msg.useManualSpeed), numRetries);
      } else {
        PickAndPlaceObjectAction* action = new PickAndPlaceObjectAction(selectedObjectID, msg.useManualSpeed);
        action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, action, numRetries);
      }
    }
  }
  
  void CozmoGameImpl::Process_GotoObject(U2G::GotoObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 0;
      
      ObjectID selectedObjectID;
      if(msg.objectID < 0) {
        selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      } else {
        selectedObjectID = msg.objectID;
      }
      
      DriveToObjectAction* action = new DriveToObjectAction(selectedObjectID, msg.distance_mm, msg.useManualSpeed);
      robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, action, numRetries);
      
    }
  }

  void CozmoGameImpl::Process_RollObject(U2G::RollObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      const u8 numRetries = 1;
      
      ObjectID selectedObjectID;
      if(msg.objectID < 0) {
        selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      } else {
        selectedObjectID = msg.objectID;
      }
      
      if(static_cast<bool>(msg.usePreDockPose)) {
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new DriveToRollObjectAction(selectedObjectID, msg.useManualSpeed), numRetries);
      } else {
        RollObjectAction* action = new RollObjectAction(selectedObjectID, msg.useManualSpeed);
        action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, action, numRetries);
      }
    }
  }
  
  void CozmoGameImpl::Process_TraverseObject(U2G::TraverseObject const& msg)
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
  
  
  void CozmoGameImpl::Process_ExecuteBehavior(U2G::ExecuteBehavior const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartBehaviorMode(static_cast<BehaviorManager::Mode>(msg.behaviorMode));
    }
  }
  
  void CozmoGameImpl::Process_SetBehaviorState(U2G::SetBehaviorState const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetBehaviorState(static_cast<BehaviorManager::BehaviorState>(msg.behaviorState));
    }
  }
  
  void CozmoGameImpl::Process_AbortPath(U2G::AbortPath const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ClearPath();
    }
  }
  
  void CozmoGameImpl::Process_AbortAll(U2G::AbortAll const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->AbortAll();
    }
  }
  
  void CozmoGameImpl::Process_CancelAction(U2G::CancelAction const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().Cancel(*robot, -1, (RobotActionType)msg.actionType);
    }
  }
  
  void CozmoGameImpl::Process_DrawPoseMarker(U2G::DrawPoseMarker const& msg)
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
  
  void CozmoGameImpl::Process_ErasePoseMarker(U2G::ErasePoseMarker const& msg)
  {
    VizManager::getInstance()->EraseAllQuadsWithType(VIZ_QUAD_POSE_MARKER);
  }

  void CozmoGameImpl::Process_SetWheelControllerGains(U2G::SetWheelControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetWheelControllerGains(msg.kpLeft, msg.kiLeft, msg.maxIntegralErrorLeft,
                                     msg.kpRight, msg.kiRight, msg.maxIntegralErrorRight);
    }
  }

  
  void CozmoGameImpl::Process_SetHeadControllerGains(U2G::SetHeadControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetHeadControllerGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
    }
  }
  
  void CozmoGameImpl::Process_SetLiftControllerGains(U2G::SetLiftControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetLiftControllerGains(msg.kp, msg.ki, msg.kd, msg.maxIntegralError);
    }
  }

  void CozmoGameImpl::Process_SetSteeringControllerGains(U2G::SetSteeringControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetSteeringControllerGains(msg.k1, msg.k2);
    }
  }
  
  void CozmoGameImpl::Process_SelectNextSoundScheme(U2G::SelectNextSoundScheme const& msg)
  {
    SoundSchemeID_t nextSoundScheme = (SoundSchemeID_t)(SoundManager::getInstance()->GetScheme() + 1);
    if (nextSoundScheme == NUM_SOUND_SCHEMES) {
      nextSoundScheme = SOUND_SCHEME_COZMO;
    }
    printf("Sound scheme: %d\n", nextSoundScheme);
    SoundManager::getInstance()->SetScheme(nextSoundScheme);
  }
  
  void CozmoGameImpl::Process_StartTestMode(U2G::StartTestMode const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartTestMode((TestMode)msg.mode, msg.p1, msg.p2, msg.p3);
    }
  }
  
  void CozmoGameImpl::Process_IMURequest(U2G::IMURequest const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->RequestIMU(msg.length_ms);
    }
  }
  
  void CozmoGameImpl::Process_PlayAnimation(U2G::PlayAnimation const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->PlayAnimation(&(msg.animationName[0]), msg.numLoops);
    }
  }

  void CozmoGameImpl::Process_ReplayLastAnimation(U2G::ReplayLastAnimation const& msg)
  {
    PRINT_NAMED_INFO("CozmoGame.ReadAnimationFile", "replaying last animation");
    Robot* robot = _cozmoEngine->GetFirstRobot();
    if (robot != nullptr) {
      robot->ReplayLastAnimation(msg.numLoops);
    }
  }

  void CozmoGameImpl::Process_ReadAnimationFile(U2G::ReadAnimationFile const& msg)
  {
    PRINT_NAMED_INFO("CozmoGame.ReadAnimationFile", "started animation tool");
    _cozmoEngine->StartAnimationTool();
  }
  
  void CozmoGameImpl::Process_StartFaceTracking(U2G::StartFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartFaceTracking(msg.timeout_sec);
    }
  }
  
  void CozmoGameImpl::Process_StopFaceTracking(U2G::StopFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopFaceTracking();
    }
  }
  
  void CozmoGameImpl::Process_SetVisionSystemParams(U2G::SetVisionSystemParams const& msg)
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
  
  void CozmoGameImpl::Process_SetFaceDetectParams(U2G::SetFaceDetectParams const& msg)
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
  
  void CozmoGameImpl::Process_StartLookingForMarkers(U2G::StartLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartLookingForMarkers();
    }
  }
  
  void CozmoGameImpl::Process_StopLookingForMarkers(U2G::StopLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopLookingForMarkers();
    }
  }
  
  
  void CozmoGameImpl::Process_SetActiveObjectLEDs(U2G::SetActiveObjectLEDs const& msg)
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
  
  void CozmoGameImpl::Process_SetAllActiveObjectLEDs(U2G::SetAllActiveObjectLEDs const& msg)
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

  void CozmoGameImpl::Process_VisionWhileMoving(U2G::VisionWhileMoving const& msg)
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
  
  void CozmoGameImpl::Process_SetBackpackLEDs(U2G::SetBackpackLEDs const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->SetBackpackLights(msg.onColor, msg.offColor,
                               msg.onPeriod_ms, msg.offPeriod_ms,
                               msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms);
    }
  }
  
  void CozmoGameImpl::Process_VisualizeQuad(U2G::VisualizeQuad const& msg)
  {
    const Quad3f quad({msg.xUpperLeft,  msg.yUpperLeft,  msg.zUpperLeft},
                      {msg.xUpperRight, msg.yUpperRight, msg.zUpperRight},
                      {msg.xLowerLeft,  msg.yLowerLeft,  msg.zLowerLeft},
                      {msg.xLowerRight, msg.yLowerRight, msg.zLowerRight});
    
    VizManager::getInstance()->DrawGenericQuad(msg.quadID, quad, msg.color);
  }
  
  void CozmoGameImpl::Process_EraseQuad(U2G::EraseQuad const& msg)
  {
    VizManager::getInstance()->EraseQuad(VIZ_QUAD_GENERIC_3D, msg.quadID);
  }
  
}
}
