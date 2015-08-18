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
    // Handled in CozmoEngine::HandleEvents
  }
  
  void CozmoGameImpl::Process_ConnectToUiDevice(ExternalInterface::ConnectToUiDevice const& msg)
  {
    // Handled in CozmoGameImpl::HandleEvents
  }
  
  void CozmoGameImpl::Process_DisconnectFromUiDevice(ExternalInterface::DisconnectFromUiDevice const& msg)
  {
    // Handled in CozmoGameImpl::HandleEvents
  }
  
  void CozmoGameImpl::Process_ForceAddRobot(ExternalInterface::ForceAddRobot const& msg)
  {
    // Handled in CozmoEngineHost:HandleEvents
  }
  
  void CozmoGameImpl::Process_StartEngine(ExternalInterface::StartEngine const& msg)
  {
    // Handled in CozmoGameImpl::HandleStartEngine
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
    // Handled in RobotEventHandler::HandleActionEvents
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
  
  void CozmoGameImpl::Process_FaceObject(ExternalInterface::FaceObject const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
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
  
  void CozmoGameImpl::Process_GotoPose(ExternalInterface::GotoPose const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGround(ExternalInterface::PlaceObjectOnGround const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
  }
  
  void CozmoGameImpl::Process_PlaceObjectOnGroundHere(ExternalInterface::PlaceObjectOnGroundHere const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
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
  
  void CozmoGameImpl::Process_PickAndPlaceObject(ExternalInterface::PickAndPlaceObject const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
  }
  
  void CozmoGameImpl::Process_GotoObject(ExternalInterface::GotoObject const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
  }
  
  void CozmoGameImpl::Process_RollObject(ExternalInterface::RollObject const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
  }
  
  void CozmoGameImpl::Process_TraverseObject(ExternalInterface::TraverseObject const& msg)
  {
    // Handled in RobotEventHandler::HandleActionEvents
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
    // Handled in RobotEventHandler::HandleActionEvents
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
    // Handled in CozmoEngine::HandleEvents
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
  
  void CozmoGameImpl::Process_QueueSingleAction(const ExternalInterface::QueueSingleAction &msg)
  {
    // Handled in RobotEventHandler::HandleQueueSingleAction
  }
  
  void CozmoGameImpl::Process_QueueCompoundAction(const ExternalInterface::QueueCompoundAction &msg)
  {
    // Handled in RobotEventHandler::HandleQueueCompoundAction
  }
  
}
}
