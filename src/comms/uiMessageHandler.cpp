/**
 * File: uiMessageHandler.cpp
 *
 * Author: Kevin Yoon
 * Date:   7/11/2014
 *
 * Description: Handles messages between UI and basestation just as
 *              RobotMessageHandler handles messages between basestation and robot.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/basestation/utils/logging/logging.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/uiMessageHandler.h"
#include "anki/cozmo/game/signals/cozmoGameSignals.h"
#include "anki/cozmo/basestation/soundManager.h"

#include "behaviorManager.h"
#include "cozmoActions.h"

#include "vizManager.h"

#if(RUN_UI_MESSAGE_TCP_SERVER)
#include "anki/cozmo/shared/cozmoConfig.h"
#else
#include "anki/cozmo/basestation/ui/messaging/messageQueue.h"
#endif

namespace Anki {
  namespace Cozmo {

    UiMessageHandler::UiMessageHandler()
    : comms_(NULL), cozmoEngine_(NULL), isInitialized_(false)
    {
      
    }
    Result UiMessageHandler::Init(Comms::IComms*   comms,
                                  CozmoEngineHost*  cozmoEngine)
    {
      Result retVal = RESULT_FAIL;
      
      if(comms != nullptr && cozmoEngine != nullptr) {
        comms_ = comms;
        cozmoEngine_ = cozmoEngine;
        
        isInitialized_ = true;
        retVal = RESULT_OK;
      }
      
      return retVal;
    }
    

    Result UiMessageHandler::SendMessage(const UserDeviceID_t devID, const UiMessage& msg)
    {
      #if(RUN_UI_MESSAGE_TCP_SERVER)
      
      Comms::MsgPacket p;
      p.data[0] = msg.GetID();
      msg.GetBytes(p.data+1);
      p.dataLen = msg.GetSize() + 1;
      p.destId = devID;
      
      return comms_->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
      
      #else
      
      //MessageQueue::getInstance()->AddMessageForUi(msg);
      
      #endif
      
      return RESULT_OK;
    }

  
    Result UiMessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      Result retVal = RESULT_FAIL;
      
      if(cozmoEngine_ == NULL) {
        PRINT_NAMED_ERROR("UiMessageHandler.NullCozmoEngine",
                          "CozmoEngine NULL when MessageHandler::ProcessPacket() called.\n");
      }
      else {
        const u8 msgID = packet.data[0];
        
        if(lookupTable_[msgID].size != packet.dataLen-1) {
          PRINT_NAMED_ERROR("UiMessageHandler.MessageBufferWrongSize",
                            "Buffer's size does not match expected size for this message ID. (Msg %d, expected %d, recvd %d)\n",
                            msgID,
                            lookupTable_[msgID].size,
                            packet.dataLen - 1
                            );
        }
        else {
          
          /*
          if(cozmoEngine_->GetNumRobots() == 0) {
            PRINT_NAMED_ERROR("UiMessageHandler.NoRobotsToControl", "\n");
          }
          else {
           */
            // TODO: Look up the robotID associated with this UI device
            const RobotID_t robotID = 1;
            Robot* robot = cozmoEngine_->GetRobotByID(robotID);

            // TODO: Move robot pointer stuff above into the ProcessMessage methods below
            // For now, note that robot could be a nullptr!
            /*
            if(robot == nullptr) {
              PRINT_NAMED_ERROR("UiMessageHandler.ProcessPacket",
                                "RobotManager could not find robot with ID=%d.\n", robotID);
              retVal = RESULT_FAIL;
            } else {
             */
              // This calls the (macro-generated) ProcessPacketAs_MessageX() method
              // indicated by the lookup table, which will cast the buffer as the
              // correct message type and call the specified robot's ProcessMessage(MessageX)
              // method.
              retVal = (*this.*lookupTable_[msgID].ProcessPacketAs)(robot, packet.data+1);
            //}
          //}
        }
      } // if(robotMgr_ != NULL)
      
      return retVal;
    } // ProcessBuffer()
    
    Result UiMessageHandler::ProcessMessages()
    {
      Result retVal = RESULT_FAIL;
      
      if(isInitialized_) {
        retVal = RESULT_OK;
        
        while(comms_->GetNumPendingMsgPackets() > 0)
        {
          Comms::MsgPacket packet;
          comms_->GetNextMsgPacket(packet);
          
          if(ProcessPacket(packet) != RESULT_OK) {
            retVal = RESULT_FAIL;
          }
        } // while messages are still available from comms
      }
      
      return retVal;
    } // ProcessMessages()
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ConnectToRobot const& msg)
    {
      // Tell the game to connect to a robot, using an event
      CozmoGameSignals::GetConnectToRobotSignal().emit(msg.robotID);
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ConnectToUiDevice const& msg)
    {
      // Tell the game to connect to a UI device, using an event
      CozmoGameSignals::GetConnectToUiDeviceSignal().emit(msg.deviceID);
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_DriveWheels const& msg)
    {
      return robot->DriveWheels(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_MoveHead const& msg)
    {
      return robot->MoveHead(msg.speed_rad_per_sec);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_MoveLift const& msg)
    {
      return robot->MoveLift(msg.speed_rad_per_sec);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetHeadAngle const& msg)
    {
      return robot->MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_StopAllMotors const& msg)
    {
      return robot->StopAllMotors();
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetLiftHeight const& msg)
    {
      // Special case if commanding low dock height
      if (msg.height_mm == LIFT_HEIGHT_LOWDOCK) {
        if(robot->IsCarryingObject()) {
          // Put the block down right here
          return robot->PlaceObjectOnGround();
        }
      }
      
      return robot->MoveLiftToHeight(msg.height_mm, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ImageRequest const& msg)
    {
      if (msg.mode == ISM_OFF) {
        robot->GetBlockWorld().EnableDraw(false);
      } else if (msg.mode == ISM_STREAM) {
        robot->GetBlockWorld().EnableDraw(true);
      }

      return robot->RequestImage((ImageSendMode_t)msg.mode,
                                 (Vision::CameraResolution)msg.resolution);
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SaveImages const& msg)
    {
      VizManager::getInstance()->SaveImages(msg.enableSave);
      printf("Saving images: %s\n", VizManager::getInstance()->IsSavingImages() ? "ON" : "OFF");
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_EnableDisplay const& msg)
    {
      VizManager::getInstance()->ShowObjects(msg.enable);
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetHeadlights const& msg)
    {
      return robot->SetHeadlight(msg.intensity);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_GotoPose const& msg)
    {
      // TODO: Add ability to indicate z too!
      // TODO: Better way to specify the target pose's parent
      Pose3d targetPose(msg.rad, Z_AXIS_3D, Vec3f(msg.x_mm, msg.y_mm, 0), robot->GetWorldOrigin());
      targetPose.SetName("GotoPoseTarget");
      robot->GetActionList().AddAction(new DriveToPoseAction(targetPose));
      return RESULT_OK;
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_PlaceObjectOnGround const& msg)
    {
      // Create an action to drive to specied pose and then put down the carried
      // object.
      // TODO: Better way to set the object's z height and parent? (This assumes object's origin is 22mm off the ground!)
      Pose3d targetPose(msg.rad, Z_AXIS_3D, Vec3f(msg.x_mm, msg.y_mm, 22.f), robot->GetWorldOrigin());
      robot->GetActionList().AddAction(new PlaceObjectOnGroundAtPoseAction(*robot, targetPose));

      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_PlaceObjectOnGroundHere const& msg)
    {
      robot->GetActionList().AddAction(new PlaceObjectOnGroundAction());
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ExecuteTestPlan const& msg)
    {
      robot->ExecuteTestPath();
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ClearAllBlocks const& msg)
    {
      VizManager::getInstance()->EraseAllVizObjects();
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
      robot->GetBlockWorld().ClearObjectsByFamily(BlockWorld::ObjectFamily::RAMPS);
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SelectNextObject const& msg)
    {
      robot->GetBlockWorld().CycleSelectedObject();
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_PickAndPlaceObject const& msg)
    {
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
      
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_TraverseObject const& msg)
    {
      const u8 numRetries = 0;
      
      ObjectID selectedObjectID = robot->GetBlockWorld().GetSelectedObject();
      robot->GetActionList().AddAction(new DriveToAndTraverseObjectAction(selectedObjectID), numRetries);
      
      return RESULT_OK;
    }
    
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ExecuteBehavior const& msg)
    {
      robot->StartBehaviorMode(static_cast<BehaviorManager::Mode>(msg.behaviorMode));
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetBehaviorState const& msg)
    {
      robot->SetBehaviorState(static_cast<BehaviorManager::BehaviorState>(msg.behaviorState));
      return RESULT_OK;
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_AbortPath const& msg)
    {
      robot->ClearPath();
      return RESULT_OK;
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_AbortAll const& msg)
    {
      return robot->AbortAll();
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_DrawPoseMarker const& msg)
    {
      if (robot->IsCarryingObject()) {
        Pose3d targetPose(msg.rad, Z_AXIS_3D, Vec3f(msg.x_mm, msg.y_mm, 0));
        Quad2f objectFootprint = robot->GetBlockWorld().GetObjectByID(robot->GetCarryingObject())->GetBoundingQuadXY(targetPose);
        VizManager::getInstance()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
      }

      return RESULT_OK;
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ErasePoseMarker const& msg)
    {
      VizManager::getInstance()->EraseAllQuadsWithType(VIZ_QUAD_POSE_MARKER);
      return RESULT_OK;
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetHeadControllerGains const& msg)
    {
      return robot->SetHeadControllerGains(msg.kp, msg.ki, msg.maxIntegralError);
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetLiftControllerGains const& msg)
    {
      return robot->SetLiftControllerGains(msg.kp, msg.ki, msg.maxIntegralError);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SelectNextSoundScheme const& msg)
    {
      SoundSchemeID_t nextSoundScheme = (SoundSchemeID_t)(SoundManager::getInstance()->GetScheme() + 1);
      if (nextSoundScheme == NUM_SOUND_SCHEMES) {
        nextSoundScheme = SOUND_SCHEME_COZMO;
      }
      printf("Sound scheme: %d\n", nextSoundScheme);
      SoundManager::getInstance()->SetScheme(nextSoundScheme);
      return RESULT_OK;
    }

    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_StartTestMode const& msg)
    {
      return robot->StartTestMode((TestMode)msg.mode);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_IMURequest const& msg)
    {
      return robot->RequestIMU(msg.length_ms);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_PlayAnimation const& msg)
    {
      return robot->PlayAnimation(&(msg.animationName[0]), msg.numLoops);
    }
  
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_ReadAnimationFile const& msg)
    {
      return robot->ReadAnimationFile();
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_StartFaceTracking const& msg)
    {
      return robot->StartFaceTracking(msg.timeout_sec);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_StopFaceTracking const& msg)
    {
      return robot->StopFaceTracking();
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetVisionSystemParams const& msg)
    {
      VisionSystemParams_t p;
      p.autoexposureOn = msg.autoexposureOn;
      p.exposureTime = msg.exposureTime;
      p.integerCountsIncrement = msg.integerCountsIncrement;
      p.minExposureTime = msg.minExposureTime;
      p.maxExposureTime = msg.maxExposureTime;
      p.percentileToMakeHigh = msg.percentileToMakeHigh;
      p.highValue = msg.highValue;
      return robot->SendVisionSystemParams(p);
    }
   
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_SetFaceDetectParams const& msg)
    {
      FaceDetectParams_t p;
      p.scaleFactor = msg.scaleFactor;
      p.minNeighbors = msg.minNeighbors;
      p.minObjectHeight = msg.minObjectHeight;
      p.minObjectWidth = msg.minObjectWidth;
      p.maxObjectHeight = msg.maxObjectHeight;
      p.maxObjectWidth = msg.maxObjectWidth;
      return robot->SendFaceDetectParams(p);
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_StartLookingForMarkers const& msg)
    {
      return robot->StartLookingForMarkers();
    }
    
    Result UiMessageHandler::ProcessMessage(Robot* robot, MessageU2G_StopLookingForMarkers const& msg)
    {
      return robot->StopLookingForMarkers();
    }
    
  } // namespace Cozmo
} // namespace Anki
