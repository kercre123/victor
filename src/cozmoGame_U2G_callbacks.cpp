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
 
  /*
   *  Helper macro for generating a callback lambda that captures "this" and 
   *  calls the corresponding ProcessMessage method. For example, for a 
   *  U2G_FooBar, use REGISTER_CALLBACK(FooBar) and the following code
   *  is generated:
   *
   *    auto cbFooBar = [this](const U2G_FooBar& msg) {
   *      this->ProcessMessage(msg);
   *    };
   *    _uiMsgHandler.RegisterCallbackForU2G_FooBar(cbFooBar);
   * 
   * NOTE: For compactness, the lambda is not actually created and stored in
   *   cbFooBar. Instead, it is simply created inline within the RegisterCallback call.
   */
  void CozmoGameImpl::RegisterCallbacksU2G()
  {
    _uiMsgHandler.RegisterCallbackForMessage([this](const U2G_Message& msg) {
#include "comms/messaging/UiMessageDefinitionsU2G_switch.def"
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
  
  void CozmoGameImpl::ProcessBadType_U2G_Message(U2G_Message::Type type)
  {
    PRINT_NAMED_WARNING("CozmoGameImpl.ProcessBadType",
                        "Got unknown message with id %d.\n",
                        type);
  }
  
  void CozmoGameImpl::Process_Ping(U2G_Ping const& msg)
  {
    
    _lastPingTimeFromUI_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    // Check to see if the ping counter is sequential or if we've dropped packets/pings
    const u32 counterDiff = msg.counter - _lastPingCounterFromUI;
    _lastPingCounterFromUI = msg.counter;
    
    if(counterDiff > 1) {
      PRINT_NAMED_WARNING("CozmoGameImpl.Update",
                          "Counter difference > 1 betweeen last two pings from UI. (Difference was %d.)\n",
                          counterDiff);
      
      // TODO: Take action if we've dropped pings?
    }
    
  }
  
  void CozmoGameImpl::Process_ConnectToRobot(U2G_ConnectToRobot const& msg)
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
  
  void CozmoGameImpl::Process_ConnectToUiDevice(U2G_ConnectToUiDevice const& msg)
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
  
  void CozmoGameImpl::Process_DisconnectFromUiDevice(U2G_DisconnectFromUiDevice const& msg)
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
  
  void CozmoGameImpl::Process_ForceAddRobot(U2G_ForceAddRobot const& msg)
  {
    char ip[16];
    assert(msg.ipAddress.size() <= 16);
    std::copy(msg.ipAddress.begin(), msg.ipAddress.end(), ip);
    ForceAddRobot(msg.robotID, ip, msg.isSimulated);
  }
  
  void CozmoGameImpl::Process_StartEngine(U2G_StartEngine const& msg)
  {
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
  
  void CozmoGameImpl::Process_DriveWheels(U2G_DriveWheels const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->AreWheelsLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_DriveWheels.WheelsLocked",
                         "Ignoring U2G_DriveWheels while wheels are locked.\n");
      } else {
        robot->DriveWheels(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
      }
    }
  }
  
  void CozmoGameImpl::Process_TurnInPlace(U2G_TurnInPlace const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().AddAction(new TurnInPlaceAction(msg.angle_rad));
    }
  }
  
  void CozmoGameImpl::Process_MoveHead(U2G_MoveHead const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveHead.HeadLocked",
                         "Ignoring U2G_MoveHead while head is locked.\n");
      } else {
        robot->MoveHead(msg.speed_rad_per_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_MoveLift(U2G_MoveLift const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveLift.LiftLocked",
                         "Ignoring U2G_MoveLift while lift is locked.\n");
      } else {
        robot->MoveLift(msg.speed_rad_per_sec);
      }
    }
  }
  
  void CozmoGameImpl::Process_SetHeadAngle(U2G_SetHeadAngle const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_SetHeadAngle.HeadLocked",
                         "Ignoring U2G_SetHeadAngle while head is locked.\n");
      } else {
        robot->DisableTrackHeadToObject();
        robot->MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
      }
    }
  }
  
  void CozmoGameImpl::Process_TrackHeadToObject(U2G_TrackHeadToObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      if(robot->IsHeadLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_TrackHeadToObject.HeadLocked",
                         "Ignoring U2G_TrackHeadToObject while head is locked.\n");
      } else {
        robot->EnableTrackHeadToObject(msg.objectID);
      }
    }
  }
  
  
  void CozmoGameImpl::Process_FaceObject(U2G_FaceObject const& msg)
  {
    Robot* robot = GetRobotByID(msg.robotID);
    
    if(robot != nullptr) {
      ObjectID objectID;
      if(msg.objectID == u32_MAX) {
        objectID = robot->GetBlockWorld().GetSelectedObject();
      } else {
        objectID = msg.objectID;
      }
      robot->GetActionList().AddAction(new FaceObjectAction(objectID, msg.turnAngleTol,
                                                            msg.maxTurnAngle,
                                                            msg.headTrackWhenDone));
    }
  }
  
  void CozmoGameImpl::Process_StopAllMotors(U2G_StopAllMotors const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopAllMotors();
    }
  }
  
  void CozmoGameImpl::Process_SetLiftHeight(U2G_SetLiftHeight const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      if(robot->IsLiftLocked()) {
        PRINT_NAMED_INFO("CozmoGameImpl.Process_SetLiftHeight.LiftLocked",
                         "Ignoring U2G_SetLiftHeight while lift is locked.\n");
      } else {
        // Special case if commanding low dock height
        if (msg.height_mm == LIFT_HEIGHT_LOWDOCK) {
          if(robot->IsCarryingObject()) {
            // Put the block down right here
            U2G_PlaceObjectOnGroundHere m;
            Process_PlaceObjectOnGroundHere(m);
            return;
          }
        }
        
        robot->MoveLiftToHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
      }
    }
  }
  
  void CozmoGameImpl::Process_SetRobotImageSendMode(U2G_SetRobotImageSendMode const& msg)
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
  
  void CozmoGameImpl::Process_ImageRequest(U2G_ImageRequest const& msg)
  {
    SetImageSendMode(msg.robotID, static_cast<ImageSendMode_t>(msg.mode));
  }
  
  void CozmoGameImpl::Process_SaveImages(U2G_SaveImages const& msg)
  {
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      printf("Saving image mode %d\n", msg.mode);
      robot->SetSaveImageMode((SaveMode_t)msg.mode);
    }
  }
  
  void CozmoGameImpl::Process_SaveRobotState(U2G_SaveRobotState const& msg)
  {
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
    printf("Saving robot state: %d\n", msg.mode);
      robot->SetSaveStateMode((SaveMode_t)msg.mode);
    }
  }
  
  void CozmoGameImpl::Process_EnableDisplay(U2G_EnableDisplay const& msg)
  {
    VizManager::getInstance()->ShowObjects(msg.enable);
  }
  
  void CozmoGameImpl::Process_SetHeadlights(U2G_SetHeadlights const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetHeadlight(msg.intensity);
    }
  }
  
  void CozmoGameImpl::Process_GotoPose(U2G_GotoPose const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      // TODO: Add ability to indicate z too!
      // TODO: Better way to specify the target pose's parent
      Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0), robot->GetWorldOrigin());
      targetPose.SetName("GotoPoseTarget");
      robot->GetActionList().AddAction(new DriveToPoseAction(targetPose, msg.useManualSpeed));
    }
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGround(U2G_PlaceObjectOnGround const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      // Create an action to drive to specied pose and then put down the carried
      // object.
      // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
      Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 22.f), robot->GetWorldOrigin());
      robot->GetActionList().AddAction(new PlaceObjectOnGroundAtPoseAction(*robot, targetPose, msg.useManualSpeed));
    }
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGroundHere(U2G_PlaceObjectOnGroundHere const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetActionList().AddAction(new PlaceObjectOnGroundAction());
    }
  }
  
  void CozmoGameImpl::Process_ExecuteTestPlan(U2G_ExecuteTestPlan const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ExecuteTestPath();
    }
  }
  
  void CozmoGameImpl::Process_SetRobotCarryingObject(U2G_SetRobotCarryingObject const& msg)
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
  
  void CozmoGameImpl::Process_ClearAllBlocks(U2G_ClearAllBlocks const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::RAMPS);
    }
  }
  
  void CozmoGameImpl::Process_SelectNextObject(U2G_SelectNextObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->GetBlockWorld().CycleSelectedObject();
    }
  }
  
  void CozmoGameImpl::Process_PickAndPlaceObject(U2G_PickAndPlaceObject const& msg)
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
      
      if(static_cast<bool>(msg.usePreDockPose)) {
        robot->GetActionList().AddAction(new DriveToPickAndPlaceObjectAction(selectedObjectID, msg.useManualSpeed), numRetries);
      } else {
        PickAndPlaceObjectAction* action = new PickAndPlaceObjectAction(selectedObjectID, msg.useManualSpeed);
        action->SetPreActionPoseAngleTolerance(-1.f); // disable pre-action pose distance check
        robot->GetActionList().AddAction(action, numRetries);
      }
    }
  }
  
  void CozmoGameImpl::Process_TraverseObject(U2G_TraverseObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      
      const u8 numRetries = 0;
      
      ObjectID selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      
      if(static_cast<bool>(msg.usePreDockPose)) {
        robot->GetActionList().AddAction(new DriveToAndTraverseObjectAction(selectedObjectID, msg.useManualSpeed), numRetries);
      } else {
        TraverseObjectAction* action = new TraverseObjectAction(selectedObjectID, msg.useManualSpeed);
        robot->GetActionList().AddAction(action, numRetries);
      }
      
    }
  }
  
  
  void CozmoGameImpl::Process_ExecuteBehavior(U2G_ExecuteBehavior const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartBehaviorMode(static_cast<BehaviorManager::Mode>(msg.behaviorMode));
    }
  }
  
  void CozmoGameImpl::Process_SetBehaviorState(U2G_SetBehaviorState const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetBehaviorState(static_cast<BehaviorManager::BehaviorState>(msg.behaviorState));
    }
  }
  
  void CozmoGameImpl::Process_AbortPath(U2G_AbortPath const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ClearPath();
    }
  }
  
  void CozmoGameImpl::Process_AbortAll(U2G_AbortAll const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->AbortAll();
    }
  }
  
  void CozmoGameImpl::Process_DrawPoseMarker(U2G_DrawPoseMarker const& msg)
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
  
  void CozmoGameImpl::Process_ErasePoseMarker(U2G_ErasePoseMarker const& msg)
  {
    VizManager::getInstance()->EraseAllQuadsWithType(VIZ_QUAD_POSE_MARKER);
  }

  void CozmoGameImpl::Process_SetWheelControllerGains(U2G_SetWheelControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetWheelControllerGains(msg.kpLeft, msg.kiLeft, msg.maxIntegralErrorLeft,
                                     msg.kpRight, msg.kiRight, msg.maxIntegralErrorRight);
    }
  }

  
  void CozmoGameImpl::Process_SetHeadControllerGains(U2G_SetHeadControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetHeadControllerGains(msg.kp, msg.ki, msg.maxIntegralError);
    }
  }
  
  void CozmoGameImpl::Process_SetLiftControllerGains(U2G_SetLiftControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->SetLiftControllerGains(msg.kp, msg.ki, msg.maxIntegralError);
    }
  }
  
  void CozmoGameImpl::Process_SelectNextSoundScheme(U2G_SelectNextSoundScheme const& msg)
  {
    SoundSchemeID_t nextSoundScheme = (SoundSchemeID_t)(SoundManager::getInstance()->GetScheme() + 1);
    if (nextSoundScheme == NUM_SOUND_SCHEMES) {
      nextSoundScheme = SOUND_SCHEME_COZMO;
    }
    printf("Sound scheme: %d\n", nextSoundScheme);
    SoundManager::getInstance()->SetScheme(nextSoundScheme);
  }
  
  void CozmoGameImpl::Process_StartTestMode(U2G_StartTestMode const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartTestMode((TestMode)msg.mode, msg.p1, msg.p2, msg.p3);
    }
  }
  
  void CozmoGameImpl::Process_IMURequest(U2G_IMURequest const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->RequestIMU(msg.length_ms);
    }
  }
  
  void CozmoGameImpl::Process_PlayAnimation(U2G_PlayAnimation const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->PlayAnimation(&(msg.animationName[0]), msg.numLoops);
    }
  }
  
  void CozmoGameImpl::Process_ReadAnimationFile(U2G_ReadAnimationFile const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->ReadAnimationFile();
    }
  }
  
  void CozmoGameImpl::Process_StartFaceTracking(U2G_StartFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartFaceTracking(msg.timeout_sec);
    }
  }
  
  void CozmoGameImpl::Process_StopFaceTracking(U2G_StopFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopFaceTracking();
    }
  }
  
  void CozmoGameImpl::Process_SetVisionSystemParams(U2G_SetVisionSystemParams const& msg)
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
  
  void CozmoGameImpl::Process_SetFaceDetectParams(U2G_SetFaceDetectParams const& msg)
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
  
  void CozmoGameImpl::Process_StartLookingForMarkers(U2G_StartLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StartLookingForMarkers();
    }
  }
  
  void CozmoGameImpl::Process_StopLookingForMarkers(U2G_StopLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = GetRobotByID(robotID);
    
    if(robot != nullptr) {
      robot->StopLookingForMarkers();
    }
  }
  
  void CozmoGameImpl::Process_VisionWhileMoving(U2G_VisionWhileMoving const& msg)
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
  
  
}
}
