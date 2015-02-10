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
#include "anki/cozmo/game/signals/cozmoGameSignals.h"

namespace Anki {
namespace Cozmo {
 
  /*
   *  Helper macro for generating a callback lambda that captures "this" and 
   *  calls the corresponding ProcessMessage method. For example, for a 
   *  MessageU2G_FooBar, use REGISTER_CALLBACK(FooBar) and the following code
   *  is generated:
   *
   *    auto cbFooBar = [this](const MessageU2G_FooBar& msg) {
   *      this->ProcessMessage(msg);
   *    };
   *    _uiMsgHandler.RegisterCallbackForMessageU2G_FooBar(cbFooBar);
   * 
   * NOTE: For compactness, the lambda is not actually created and stored in
   *   cbFooBar. Instead, it is simply created inline within the RegisterCallback call.
   */
#define REGISTER_CALLBACK(__MSG_TYPE__) \
_uiMsgHandler.RegisterCallbackForMessage##__MSG_TYPE__([this](const Message##__MSG_TYPE__& msg) { this->ProcessMessage(msg);});
  
  void CozmoGameHostImpl::RegisterCallbacksU2G()
  {
    REGISTER_CALLBACK(U2G_ConnectToRobot)
    REGISTER_CALLBACK(U2G_ConnectToUiDevice)
    REGISTER_CALLBACK(U2G_ForceAddRobot)
    REGISTER_CALLBACK(U2G_StartEngine)
    REGISTER_CALLBACK(U2G_DriveWheels)
    REGISTER_CALLBACK(U2G_MoveHead)
    REGISTER_CALLBACK(U2G_MoveLift)
    REGISTER_CALLBACK(U2G_SetLiftHeight)
    REGISTER_CALLBACK(U2G_SetHeadAngle)
    REGISTER_CALLBACK(U2G_StopAllMotors)
    REGISTER_CALLBACK(U2G_ImageRequest)
    REGISTER_CALLBACK(U2G_SetRobotImageSendMode)
    REGISTER_CALLBACK(U2G_SaveImages)
    REGISTER_CALLBACK(U2G_EnableDisplay)
    REGISTER_CALLBACK(U2G_SetHeadlights)
    REGISTER_CALLBACK(U2G_GotoPose)
    REGISTER_CALLBACK(U2G_PlaceObjectOnGround)
    REGISTER_CALLBACK(U2G_PlaceObjectOnGroundHere)
    REGISTER_CALLBACK(U2G_ExecuteTestPlan)
    REGISTER_CALLBACK(U2G_SelectNextObject)
    REGISTER_CALLBACK(U2G_PickAndPlaceObject)
    REGISTER_CALLBACK(U2G_TraverseObject)
    REGISTER_CALLBACK(U2G_ClearAllBlocks)
    REGISTER_CALLBACK(U2G_ExecuteBehavior)
    REGISTER_CALLBACK(U2G_SetBehaviorState)
    REGISTER_CALLBACK(U2G_AbortPath)
    REGISTER_CALLBACK(U2G_AbortAll)
    REGISTER_CALLBACK(U2G_DrawPoseMarker)
    REGISTER_CALLBACK(U2G_ErasePoseMarker)
    REGISTER_CALLBACK(U2G_SetHeadControllerGains)
    REGISTER_CALLBACK(U2G_SetLiftControllerGains)
    REGISTER_CALLBACK(U2G_SelectNextSoundScheme)
    REGISTER_CALLBACK(U2G_StartTestMode)
    REGISTER_CALLBACK(U2G_IMURequest)
    REGISTER_CALLBACK(U2G_PlayAnimation)
    REGISTER_CALLBACK(U2G_ReadAnimationFile)
    REGISTER_CALLBACK(U2G_StartFaceTracking)
    REGISTER_CALLBACK(U2G_StopFaceTracking)
    REGISTER_CALLBACK(U2G_StartLookingForMarkers)
    REGISTER_CALLBACK(U2G_StopLookingForMarkers)
    REGISTER_CALLBACK(U2G_SetVisionSystemParams)
    REGISTER_CALLBACK(U2G_SetFaceDetectParams)
    
  } // RegisterCallbacksU2G()
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ConnectToRobot const& msg)
  {
    // Tell the game to connect to a robot, using a signal
    CozmoGameSignals::ConnectToRobotSignal().emit(msg.robotID);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ConnectToUiDevice const& msg)
  {
    // Tell the game to connect to a UI device, using a signal
    CozmoGameSignals::ConnectToUiDeviceSignal().emit(msg.deviceID);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ForceAddRobot const& msg)
  {
    char ip[16];
    assert(msg.ipAddress.size() <= 16);
    std::copy(msg.ipAddress.begin(), msg.ipAddress.end(), ip);
    _cozmoEngine.ForceAddRobot(msg.robotID, ip, msg.isSimulated);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StartEngine const& msg)
  {
    // Populate the Json configuration from the message members:
    Json::Value config;
    
    // Viz Host IP:
    char ip[16];
    assert(msg.vizHostIP.size() <= 16);
    std::copy(msg.vizHostIP.begin(), msg.vizHostIP.end(), ip);
    config["VizHostIP"] = ip;
    
    // Start the engine with that configuration
    StartEngine(config);
    
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_DriveWheels const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_DriveWheels for null robot.\n");
    } else {
      robot->DriveWheels(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_MoveHead const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_MoveHead for null robot.\n");
    } else {
      robot->MoveHead(msg.speed_rad_per_sec);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_MoveLift const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_MoveLift for null robot.\n");
    } else {
      robot->MoveLift(msg.speed_rad_per_sec);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetHeadAngle const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_SetHeadAngle for null robot.\n");
    } else {
      robot->MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StopAllMotors const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);

    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_StopAllMotors for null robot.\n");
    } else {
      robot->StopAllMotors();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetLiftHeight const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_SetLiftHeight for null robot.\n");
      return;
    }
    
    // Special case if commanding low dock height
    if (msg.height_mm == LIFT_HEIGHT_LOWDOCK) {
      if(robot->IsCarryingObject()) {
        // Put the block down right here
        robot->PlaceObjectOnGround();
        return;
      }
    }
    
    robot->MoveLiftToHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetRobotImageSendMode const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
   
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_SetRobotImageSendMode for null robot.\n");
    } else {
      
      if (msg.mode == ISM_OFF) {
        robot->GetBlockWorld().EnableDraw(false);
      } else if (msg.mode == ISM_STREAM) {
        robot->GetBlockWorld().EnableDraw(true);
      }
      
      robot->RequestImage((ImageSendMode_t)msg.mode,
                          (Vision::CameraResolution)msg.resolution);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ImageRequest const& msg)
  {
    SetImageSendMode(msg.robotID, static_cast<ImageSendMode_t>(msg.mode));
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SaveImages const& msg)
  {
    VizManager::getInstance()->SaveImages(msg.enableSave);
    printf("Saving images: %s\n", VizManager::getInstance()->IsSavingImages() ? "ON" : "OFF");
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_EnableDisplay const& msg)
  {
    VizManager::getInstance()->ShowObjects(msg.enable);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetHeadlights const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
   
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_SetHeadlights for null robot.\n");
    } else {
      robot->SetHeadlight(msg.intensity);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_GotoPose const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
   
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_GotoPose for null robot.\n");
    } else {
      // TODO: Add ability to indicate z too!
      // TODO: Better way to specify the target pose's parent
      Pose3d targetPose(msg.rad, Z_AXIS_3D, Vec3f(msg.x_mm, msg.y_mm, 0), robot->GetWorldOrigin());
      targetPose.SetName("GotoPoseTarget");
      robot->GetActionList().AddAction(new DriveToPoseAction(targetPose));
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_PlaceObjectOnGround const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
   
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_PlaceObjectOnGround for null robot.\n");
    } else {
      // Create an action to drive to specied pose and then put down the carried
      // object.
      // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
      Pose3d targetPose(msg.rad, Z_AXIS_3D, Vec3f(msg.x_mm, msg.y_mm, 22.f), robot->GetWorldOrigin());
      robot->GetActionList().AddAction(new PlaceObjectOnGroundAtPoseAction(*robot, targetPose));
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_PlaceObjectOnGroundHere const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_PlaceObjectOnGroundHere for null robot.\n");
    } else {
      robot->GetActionList().AddAction(new PlaceObjectOnGroundAction());
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ExecuteTestPlan const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_ExecuteTestPlan for null robot.\n");
    } else {
      robot->ExecuteTestPath();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ClearAllBlocks const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
   
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_ClearAllBlocks for null robot.\n");
    } else {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::RAMPS);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SelectNextObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_SelectNextObject for null robot.\n");
    } else {
      robot->GetBlockWorld().CycleSelectedObject();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_PickAndPlaceObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_PickAndPlaceObject for null robot.\n");
    } else {
      const u8 numRetries = 0;
      
      ObjectID selectedObjectID;
      if(msg.objectID < 0) {
        selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      } else {
        selectedObjectID = msg.objectID;
      }
      
      if(static_cast<bool>(msg.usePreDockPose)) {
        robot->GetActionList().AddAction(new DriveToPickAndPlaceObjectAction(selectedObjectID), numRetries);
      } else {
        PickAndPlaceObjectAction* action = new PickAndPlaceObjectAction(selectedObjectID);
        action->SetMaxPreActionPoseDistance(-1.f); // disable pre-action pose distance check
        robot->GetActionList().AddAction(action, numRetries);
      }
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_TraverseObject const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_TraverseObject for null robot.\n");
    } else {
      
      const u8 numRetries = 0;
      
      ObjectID selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      robot->GetActionList().AddAction(new DriveToAndTraverseObjectAction(selectedObjectID), numRetries);
      
    }
  }
  
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ExecuteBehavior const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_ExecuteBehavior for null robot.\n");
    } else {
      robot->StartBehaviorMode(static_cast<BehaviorManager::Mode>(msg.behaviorMode));
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetBehaviorState const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_SetBehaviorState for null robot.\n");
    } else {
      robot->SetBehaviorState(static_cast<BehaviorManager::BehaviorState>(msg.behaviorState));
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_AbortPath const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_AbortPath for null robot.\n");
    } else {
      robot->ClearPath();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_AbortAll const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage",
                        "Cannot process MessageU2G_AbortAll for null robot.\n");
    } else {
      robot->AbortAll();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_DrawPoseMarker const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_DrawPoseMarker for null robot.\n");
    } else if (robot->IsCarryingObject()) {
      Pose3d targetPose(msg.rad, Z_AXIS_3D, Vec3f(msg.x_mm, msg.y_mm, 0));
      Quad2f objectFootprint = robot->GetBlockWorld().GetObjectByID(robot->GetCarryingObject())->GetBoundingQuadXY(targetPose);
      VizManager::getInstance()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ErasePoseMarker const& msg)
  {
    VizManager::getInstance()->EraseAllQuadsWithType(VIZ_QUAD_POSE_MARKER);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetHeadControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_SetHeadControllerGains for null robot.\n");
    } else {
      robot->SetHeadControllerGains(msg.kp, msg.ki, msg.maxIntegralError);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetLiftControllerGains const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_SetLiftControllerGains for null robot.\n");
    } else {
      robot->SetLiftControllerGains(msg.kp, msg.ki, msg.maxIntegralError);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SelectNextSoundScheme const& msg)
  {
    SoundSchemeID_t nextSoundScheme = (SoundSchemeID_t)(SoundManager::getInstance()->GetScheme() + 1);
    if (nextSoundScheme == NUM_SOUND_SCHEMES) {
      nextSoundScheme = SOUND_SCHEME_COZMO;
    }
    printf("Sound scheme: %d\n", nextSoundScheme);
    SoundManager::getInstance()->SetScheme(nextSoundScheme);
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StartTestMode const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_StartTestMode for null robot.\n");
    } else {
      robot->StartTestMode((TestMode)msg.mode, msg.p1, msg.p2, msg.p3);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_IMURequest const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_IMURequest for null robot.\n");
    } else {
      robot->RequestIMU(msg.length_ms);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_PlayAnimation const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_PlayAnimation for null robot.\n");
    } else {
      robot->PlayAnimation(&(msg.animationName[0]), msg.numLoops);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_ReadAnimationFile const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_ReadAnimationFile for null robot.\n");
    } else {
      robot->ReadAnimationFile();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StartFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_StartFaceTracking for null robot.\n");
    } else {
      robot->StartFaceTracking(msg.timeout_sec);
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StopFaceTracking const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_StopFaceTracking for null robot.\n");
    } else {
      robot->StopFaceTracking();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetVisionSystemParams const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_SetVisionSystemParams for null robot.\n");
    } else {
      
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
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_SetFaceDetectParams const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_SetFaceDetectParams for null robot.\n");
    } else {
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
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StartLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_StartLookingForMarkers for null robot.\n");
    } else {
      robot->StartLookingForMarkers();
    }
  }
  
  void CozmoGameHostImpl::ProcessMessage(MessageU2G_StopLookingForMarkers const& msg)
  {
    // TODO: Get robot ID from message or the one corresponding to the UI that sent the message?
    const RobotID_t robotID = 1;
    Robot* robot = _cozmoEngine.GetRobotByID(robotID);
    
    if(robot == nullptr) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.ProcessMessage", "Cannot process MessageU2G_StopLookingForMarkers for null robot.\n");
    } else {
      robot->StopLookingForMarkers();
    }
  }
  
  
}
}