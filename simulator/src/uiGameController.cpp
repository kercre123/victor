/*
 * File:          UiGameController.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "uiGameController.h"

#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/vision/basestation/image.h"
#include "anki/cozmo/game/comms/gameMessageHandler.h"
#include "anki/cozmo/game/comms/gameComms.h"

#include <opencv2/imgproc/imgproc.hpp>

#include <stdio.h>
#include <string.h>

#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>


namespace Anki {
  namespace Cozmo {
    
      // Private memers:
      namespace {
        
        webots::Node* root_ = nullptr;
        
        struct {
          s32 family;
          s32 type;
          s32 id;
          f32 area;
          bool isActive;
          
          void Reset() {
            family = -1;
            type = -1;
            id = -1;
            area = 0;
            isActive = false;
          }
        } currentlyObservedObject;
        

        typedef enum {
          UI_WAITING_FOR_GAME = 0,
          UI_RUNNING
        } UI_State_t;
        
        UI_State_t uiState_;
        
        GameMessageHandler msgHandler_;
        GameComms *gameComms_ = nullptr;
        
        // For displaying cozmo's POV:
        webots::Display* cozmoCam_;
        webots::ImageRef* img_ = nullptr;
        
        Vision::ImageDeChunker _imageDeChunker;
        
        // Save robot image to file
        bool saveRobotImageToFile_ = false;

      } // private namespace

    
    // ======== Message handler callbacks =======
      
    // TODO: Update these not to need robotID
    
    void UiGameController::HandleRobotStateUpdateBase(ExternalInterface::RobotState const& msg)
    {
      _robotPose.SetTranslation({msg.pose_x, msg.pose_y, msg.pose_z});
      _robotPose.SetRotation(msg.headAngle_rad, Z_AXIS_3D());
      
      HandleRobotStateUpdate(msg);
    }
    
    void UiGameController::HandleRobotObservedObjectBase(ExternalInterface::RobotObservedObject const& msg)
    {
      if(cozmoCam_ == nullptr) {
        printf("RECEIVED OBJECT OBSERVED: objectID %d\n", msg.objectID);
      } else {
        // Draw a rectangle in red with the object ID as text in the center
        cozmoCam_->setColor(0x000000);
        
        //std::string dispStr(ObjectType::GetName(msg.objectType));
        //dispStr += " ";
        //dispStr += std::to_string(msg.objectID);
        std::string dispStr("Type=" + std::to_string(msg.objectType) + "\nID=" + std::to_string(msg.objectID));
        cozmoCam_->drawText(dispStr,
                            msg.img_topLeft_x + msg.img_width/4 + 1,
                            msg.img_topLeft_y + msg.img_height/2 + 1);
        
        cozmoCam_->setColor(0xff0000);          
        cozmoCam_->drawRectangle(msg.img_topLeft_x, msg.img_topLeft_y,
                                 msg.img_width, msg.img_height);
        cozmoCam_->drawText(dispStr,
                            msg.img_topLeft_x + msg.img_width/4,
                            msg.img_topLeft_y + msg.img_height/2);
        
        const f32 area = msg.img_width * msg.img_height;
        //if(area > currentlyObservedObject.area) {
          currentlyObservedObject.family = msg.objectFamily;
          currentlyObservedObject.type   = msg.objectType;
          currentlyObservedObject.id     = msg.objectID;
          currentlyObservedObject.isActive = msg.isActive;
          currentlyObservedObject.area   = area;
        //}
        
      }
      
      HandleRobotObservedObject(msg);
    }
    
    void UiGameController::HandleRobotObservedNothingBase(ExternalInterface::RobotObservedNothing const& msg)
    {
      currentlyObservedObject.Reset();
      
      HandleRobotObservedNothing(msg);
    }
    
    void UiGameController::HandleRobotDeletedObjectBase(ExternalInterface::RobotDeletedObject const& msg)
    {
      printf("Robot %d reported deleting object %d\n", msg.robotID, msg.objectID);
      
      HandleRobotDeletedObject(msg);
    }

    void UiGameController::HandleRobotConnectionBase(ExternalInterface::RobotAvailable const& msgIn)
    {
      // Just send a message back to the game to connect to any robot that's
      // advertising (since we don't have a selection mechanism here)
      printf("Sending message to command connection to robot %d.\n", msgIn.robotID);
      ExternalInterface::ConnectToRobot msgOut;
      msgOut.robotID = msgIn.robotID;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ConnectToRobot(msgOut);
      SendMessage(message);
      
      HandleRobotConnection(msgIn);
    }
    
    void UiGameController::HandleUiDeviceConnectionBase(ExternalInterface::UiDeviceAvailable const& msgIn)
    {
      // Just send a message back to the game to connect to any UI device that's
      // advertising (since we don't have a selection mechanism here)
      printf("Sending message to command connection to UI device %d.\n", msgIn.deviceID);
      ExternalInterface::ConnectToUiDevice msgOut;
      msgOut.deviceID = msgIn.deviceID;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ConnectToUiDevice(msgOut);
      SendMessage(message);
      
      HandleUiDeviceConnection(msgIn);
    }
    
    void UiGameController::HandleRobotConnectedBase(ExternalInterface::RobotConnected const &msg)
    {
      // Once robot connects, set resolution
      SendSetRobotImageSendMode(ISM_STREAM);
      
      HandleRobotConnected(msg);
    }
    
    void UiGameController::HandleRobotCompletedActionBase(ExternalInterface::RobotCompletedAction const& msg)
    {
      switch((RobotActionType)msg.actionType)
      {
        case RobotActionType::PICKUP_OBJECT_HIGH:
        case RobotActionType::PICKUP_OBJECT_LOW:
          printf("Robot %d %s picking up stack of %d objects with IDs: ",
                 msg.robotID, ActionResultToString(msg.result),
                 msg.completionInfo.numObjects);
          for(int i=0; i<msg.completionInfo.numObjects; ++i) {
            printf("%d ", msg.completionInfo.objectIDs[i]);
          }
          printf("[Tag=%d]\n", msg.idTag);
          break;
          
        case RobotActionType::PLACE_OBJECT_HIGH:
        case RobotActionType::PLACE_OBJECT_LOW:
          printf("Robot %d %s placing stack of %d objects with IDs: ",
                 msg.robotID, ActionResultToString(msg.result),
                 msg.completionInfo.numObjects);
          for(int i=0; i<msg.completionInfo.numObjects; ++i) {
            printf("%d ", msg.completionInfo.objectIDs[i]);
          }
          printf("[Tag=%d]\n", msg.idTag);
          break;

        case RobotActionType::PLAY_ANIMATION:
          printf("Robot %d finished playing animation %s. [Tag=%d]\n",
                 msg.robotID, msg.completionInfo.animName.c_str(), msg.idTag);
          break;
          
        default:
          printf("Robot %d completed action with type=%d and tag=%d: %s.\n",
                 msg.robotID, msg.actionType, msg.idTag, ActionResultToString(msg.result));
      }
      
      HandleRobotCompletedAction(msg);
    }
    
    // For processing image chunks arriving from robot.
    // Sends complete images to VizManager for visualization (and possible saving).
    void UiGameController::HandleImageChunkBase(ExternalInterface::ImageChunk const& msg)
    {
      const bool isImageReady = _imageDeChunker.AppendChunk(msg.imageId, msg.frameTimeStamp, msg.nrows, msg.ncols, (Vision::ImageEncoding_t)msg.imageEncoding, msg.imageChunkCount, msg.chunkId, msg.data, msg.chunkSize);
      
      
      if(isImageReady)
      {
        cv::Mat img = _imageDeChunker.GetImage();
        if(img.channels() == 1) {
          cvtColor(img, img, CV_GRAY2RGB);
        }
        
        for(s32 i=0; i<img.rows; ++i) {
          
          if(i % 2 == 0) {
            cv::Mat img_i = img.row(i);
            img_i.setTo(0);
          } else {
            u8* img_i = img.ptr(i);
            for(s32 j=0; j<img.cols; ++j) {
              img_i[3*j] = 0;
              img_i[3*j+2] /= 3;
              
              // [Optional] Add a bit of noise
              f32 noise = 20.f*static_cast<f32>(std::rand()) / static_cast<f32>(RAND_MAX) - 0.5f;
              img_i[3*j+1] = static_cast<u8>(std::max(0.f,std::min(255.f,static_cast<f32>(img_i[3*j+1]) + noise)));
              
            }
          }
        }
        
        // Delete existing image if there is one.
        if (img_ != nullptr) {
          cozmoCam_->imageDelete(img_);
        }
        
        img_ = cozmoCam_->imageNew(img.cols, img.rows, img.data, webots::Display::RGB);
        cozmoCam_->imagePaste(img_, 0, 0);
        
        // Save image to file
        if (saveRobotImageToFile_) {
          static u32 imgCnt = 0;
          char imgFileName[16];
          printf("SAVING IMAGE\n");
          sprintf(imgFileName, "robotImg_%d.jpg", imgCnt++);
          cozmoCam_->imageSave(img_, imgFileName);
          saveRobotImageToFile_ = false;
        }
        
      } // if(isImageReady)
      
      HandleImageChunk(msg);

    } // HandleImageChunk()
    
    
    void UiGameController::HandleActiveObjectMovedBase(ExternalInterface::ActiveObjectMoved const& msg)
    {
     PRINT_NAMED_INFO("HandleActiveObjectMoved", "Received message that object %d moved. Accel=(%f,%f,%f). UpAxis=%d\n",
                      msg.objectID, msg.xAccel, msg.yAccel, msg.zAccel, msg.upAxis);
      
      HandleActiveObjectMoved(msg);
    }
    
    void UiGameController::HandleActiveObjectStoppedMovingBase(ExternalInterface::ActiveObjectStoppedMoving const& msg)
    {
      PRINT_NAMED_INFO("HandleActiveObjectStoppedMoving", "Received message that object %d stopped moving%s. UpAxis=%d\n",
                       msg.objectID, (msg.rolled ? " and rolled" : ""), msg.upAxis);
      
      HandleActiveObjectStoppedMoving(msg);
    }
    
    void UiGameController::HandleActiveObjectTappedBase(ExternalInterface::ActiveObjectTapped const& msg)
    {
      PRINT_NAMED_INFO("HandleActiveObjectTapped", "Received message that object %d was tapped %d times.\n",
                       msg.objectID, msg.numTaps);
      
      HandleActiveObjectTapped(msg);
    }

    void UiGameController::HandleAnimationAvailableBase(ExternalInterface::AnimationAvailable const& msg)
    {
      PRINT_NAMED_INFO("HandleAnimationAvailable", "Animation available: %s", msg.animName.c_str());
      
      HandleAnimationAvailable(msg);
    }
    
    // ===== End of message handler callbacks ====
    
  
    UiGameController::UiGameController(s32 step_time_ms)

    :
    _stepTimeMS(step_time_ms)
    , _robotTransActual(nullptr)
    , _robotOrientationActual(nullptr)
    , _robotPose(0, Z_AXIS_3D(), {{0.f, 0.f, 0.f}}, nullptr, "robotPose")
    , _robotPoseActual(0, Z_AXIS_3D(), {{0.f, 0.f, 0.f}}, nullptr, "robotPoseActual")
    {
      _currentlyObservedObject.Reset();
    }
    
    void UiGameController::Init()
    {
      // Make root point to WebotsKeyBoardController node
      root_ = _supervisor.getSelf();
      
      // Set deviceID
      int deviceID = root_->getField("deviceID")->getSFInt32();
      
      // Get engine IP
      std::string engineIP = root_->getField("engineIP")->getSFString();
      
      // Startup comms with engine
      if (!gameComms_) {
        printf("Registering with advertising service at %s:%d", engineIP.c_str(), UI_ADVERTISEMENT_REGISTRATION_PORT);
        gameComms_ = new GameComms(deviceID, UI_MESSAGE_SERVER_LISTEN_PORT,
                                   engineIP.c_str(),
                                   UI_ADVERTISEMENT_REGISTRATION_PORT);
      }
      
      
      while(!gameComms_->IsInitialized()) {
        PRINT_NAMED_INFO("UiGameController.Init",
                         "Waiting for gameComms to initialize...");
        _supervisor.step(_stepTimeMS);
        gameComms_->Update();
      }
      msgHandler_.Init(gameComms_);
      
      // Register callbacks for incoming messages from game
      // TODO: Have CLAD generate this?
      msgHandler_.RegisterCallbackForMessage([this](const ExternalInterface::MessageEngineToGame& message) {
        switch (message.GetTag()) {
          case ExternalInterface::MessageEngineToGame::Tag::RobotConnected:
            HandleRobotConnectedBase(message.Get_RobotConnected());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::RobotState:
            HandleRobotStateUpdateBase(message.Get_RobotState());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::RobotObservedObject:
            HandleRobotObservedObjectBase(message.Get_RobotObservedObject());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::UiDeviceAvailable:
            HandleUiDeviceConnectionBase(message.Get_UiDeviceAvailable());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::RobotAvailable:
            HandleRobotConnectionBase(message.Get_RobotAvailable());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::ImageChunk:
            HandleImageChunkBase(message.Get_ImageChunk());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::RobotDeletedObject:
            HandleRobotDeletedObjectBase(message.Get_RobotDeletedObject());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::RobotCompletedAction:
            HandleRobotCompletedActionBase(message.Get_RobotCompletedAction());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::ActiveObjectMoved:
            HandleActiveObjectMovedBase(message.Get_ActiveObjectMoved());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::ActiveObjectStoppedMoving:
            HandleActiveObjectStoppedMovingBase(message.Get_ActiveObjectStoppedMoving());
            break;
          case ExternalInterface::MessageEngineToGame::Tag::ActiveObjectTapped:
            HandleActiveObjectTappedBase(message.Get_ActiveObjectTapped());
            break;
        case ExternalInterface::MessageEngineToGame::Tag::AnimationAvailable:
            HandleAnimationAvailableBase(message.Get_AnimationAvailable());
            break;
          default:
            // ignore
            break;
        }
      });
      
      cozmoCam_ = _supervisor.getDisplay("uiCamDisplay");

      uiState_ = UI_WAITING_FOR_GAME;
      
      InitInternal();
    }
    
  
    bool ForceAddRobotIfSpecified()
    {
      bool doForceAddRobot = false;
      bool forcedRobotIsSim = false;
      std::string forcedRobotIP;
      int  forcedRobotId = 1;
      
      webots::Field* forceAddRobotField = root_->getField("forceAddRobot");
      if(forceAddRobotField != nullptr) {
        doForceAddRobot = forceAddRobotField->getSFBool();
        if(doForceAddRobot) {
          webots::Field *forcedRobotIsSimField = root_->getField("forcedRobotIsSimulated");
          if(forcedRobotIsSimField == nullptr) {
            PRINT_NAMED_ERROR("KeyboardController.Update",
                              "Could not find 'forcedRobotIsSimulated' field.\n");
            doForceAddRobot = false;
          } else {
            forcedRobotIsSim = forcedRobotIsSimField->getSFBool();
          }
          
          webots::Field* forcedRobotIpField = root_->getField("forcedRobotIP");
          if(forcedRobotIpField == nullptr) {
            PRINT_NAMED_ERROR("KeyboardController.Update",
                              "Could not find 'forcedRobotIP' field.\n");
            doForceAddRobot = false;
          } else {
            forcedRobotIP = forcedRobotIpField->getSFString();
          }
          
          webots::Field* forcedRobotIdField = root_->getField("forcedRobotID");
          if(forcedRobotIdField == nullptr) {
            
          } else {
            forcedRobotId = forcedRobotIdField->getSFInt32();
          }
        } // if(doForceAddRobot)
      }
      
      if(doForceAddRobot) {
        ExternalInterface::ForceAddRobot msg;
        msg.isSimulated = forcedRobotIsSim;
        msg.robotID = forcedRobotId;
        
        std::fill(msg.ipAddress.begin(), msg.ipAddress.end(), '\0');
        std::copy(forcedRobotIP.begin(), forcedRobotIP.end(), msg.ipAddress.begin());
        
        ExternalInterface::MessageGameToEngine message;
        message.Set_ForceAddRobot(msg);
        msgHandler_.SendMessage(1, message); // TODO: don't hardcode ID here
      }
      
      return doForceAddRobot;
      
    } // ForceAddRobotIfSpecified()
  
    s32 UiGameController::Update()
    {
      s32 res = 0;
      
      if (_supervisor.step(_stepTimeMS) == -1) {
        PRINT_NAMED_INFO("UiGameController.Update.StepFailed", "");
        return -1;
      }
      
      gameComms_->Update();
      
      switch(uiState_) {
        case UI_WAITING_FOR_GAME:
        {
          if (!gameComms_->HasClient()) {
            return 0;
          } else {
            // Once gameComms has a client, tell the engine to start, force-add
            // robot if necessary, and switch states in the UI
            
            PRINT_NAMED_INFO("KeyboardController.Update", "Sending StartEngine message.\n");
            ExternalInterface::StartEngine msg;
            msg.asHost = 1; // TODO: Support running as client?
            std::string vizIpStr = "127.0.0.1";
            std::fill(msg.vizHostIP.begin(), msg.vizHostIP.end(), '\0'); // ensure null termination
            std::copy(vizIpStr.begin(), vizIpStr.end(), msg.vizHostIP.begin());
            ExternalInterface::MessageGameToEngine message;
            message.Set_StartEngine(msg);
            msgHandler_.SendMessage(1, message); // TODO: don't hardcode ID here
            
            bool didForceAdd = ForceAddRobotIfSpecified();
            
            if(didForceAdd) {
              PRINT_NAMED_INFO("KeyboardController.Update", "Sent force-add robot message.\n");
            }
            
            // Turn on image streaming to game/UI by default:
            SendImageRequest(ISM_STREAM, 1);
            
            uiState_ = UI_RUNNING;
          }
          break;
        }
          
        case UI_RUNNING:
        {
          SendPing();
          
          UpdateActualObjectPoses();
          
          msgHandler_.ProcessMessages();
          
          res = UpdateInternal();
          
          break;
        }
          
        default:
          PRINT_NAMED_ERROR("KeyboardController.Update", "Reached default switch case.\n");
          
      } // switch(uiState_)
      
      return res;
    }
    
    void UiGameController::UpdateActualObjectPoses()
    {
      if (_robotTransActual == nullptr) {
      
        webots::Field* rootChildren = GetSupervisor()->getRoot()->getField("children");
        int numRootChildren = rootChildren->getCount();
        for (int n = 0 ; n<numRootChildren; ++n) {
          webots::Node* nd = rootChildren->getMFNode(n);
          
          // Get the node name
          std::string nodeName = "";
          webots::Field* nameField = nd->getField("name");
          if (nameField) {
            nodeName = nameField->getSFString();
          }
          
          //printf(" Node %d: name \"%s\" typeName \"%s\" controllerName \"%s\"\n",
          //       n, nodeName.c_str(), nd->getTypeName().c_str(), controllerName.c_str());
          
          if (nd->getTypeName().find("Supervisor") != std::string::npos &&
              nodeName.find("CozmoBot") != std::string::npos) {
            
            printf("Found robot with name %s\n", nodeName.c_str());
            
            _robotTransActual = nd->getPosition();
            _robotOrientationActual = nd->getOrientation();
            
            break;
          }
        }
      }
      
      _robotPoseActual.SetTranslation( {static_cast<f32>(_robotTransActual[0]),
                                        static_cast<f32>(_robotTransActual[1]),
                                        static_cast<f32>(_robotTransActual[2])} );
      
      _robotPoseActual.SetRotation({static_cast<f32>(_robotOrientationActual[0]),
                                    static_cast<f32>(_robotOrientationActual[1]),
                                    static_cast<f32>(_robotOrientationActual[2]),
                                    static_cast<f32>(_robotOrientationActual[3]),
                                    static_cast<f32>(_robotOrientationActual[4]),
                                    static_cast<f32>(_robotOrientationActual[5]),
                                    static_cast<f32>(_robotOrientationActual[6]),
                                    static_cast<f32>(_robotOrientationActual[7]),
                                    static_cast<f32>(_robotOrientationActual[8])} );
      
    }

    void UiGameController::SendMessage(const ExternalInterface::MessageGameToEngine& msg)
    {
      UserDeviceID_t devID = 1; // TODO: Should this be a RobotID_t?
      msgHandler_.SendMessage(devID, msg); 
    }
    


    void UiGameController::SendPing()
    {
      static ExternalInterface::Ping m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_Ping(m);
      SendMessage(message);

      ++m.counter;
    }
    
    void UiGameController::SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps)
    {
      ExternalInterface::DriveWheels m;
      m.lwheel_speed_mmps = lwheel_speed_mmps;
      m.rwheel_speed_mmps = rwheel_speed_mmps;
      ExternalInterface::MessageGameToEngine message;
      message.Set_DriveWheels(m);
      SendMessage(message);
    }
    
    void UiGameController::SendTurnInPlace(const f32 angle_rad)
    {
      ExternalInterface::TurnInPlace m;
      m.robotID = 1;
      m.angle_rad = angle_rad;
      ExternalInterface::MessageGameToEngine message;
      message.Set_TurnInPlace(m);
      SendMessage(message);
    }

    void UiGameController::SendTurnInPlaceAtSpeed(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2)
    {
      ExternalInterface::TurnInPlaceAtSpeed m;
      m.robotID = 1;
      m.speed_rad_per_sec = speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      ExternalInterface::MessageGameToEngine message;
      message.Set_TurnInPlaceAtSpeed(m);
      SendMessage(message);
    }
    
    void UiGameController::SendMoveHead(const f32 speed_rad_per_sec)
    {
      ExternalInterface::MoveHead m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      ExternalInterface::MessageGameToEngine message;
      message.Set_MoveHead(m);
      SendMessage(message);
    }
    
    void UiGameController::SendMoveLift(const f32 speed_rad_per_sec)
    {
      ExternalInterface::MoveLift m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      ExternalInterface::MessageGameToEngine message;
      message.Set_MoveLift(m);
      SendMessage(message);
    }
    
    void UiGameController::SendMoveHeadToAngle(const f32 rad, const f32 speed, const f32 accel, const f32 duration_sec)
    {
      ExternalInterface::SetHeadAngle m;
      m.angle_rad = rad;
      m.max_speed_rad_per_sec = speed;
      m.accel_rad_per_sec2 = accel;
      m.duration_sec = duration_sec;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetHeadAngle(m);
      SendMessage(message);
    }
    
    void UiGameController::SendMoveLiftToHeight(const f32 mm, const f32 speed, const f32 accel, const f32 duration_sec)
    {
      ExternalInterface::SetLiftHeight m;
      m.height_mm = mm;
      m.max_speed_rad_per_sec = speed;
      m.accel_rad_per_sec2 = accel;
      m.duration_sec = duration_sec;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetLiftHeight(m);
      SendMessage(message);
    }
    
    void UiGameController::SendTapBlockOnGround(const u8 numTaps)
    {
      ExternalInterface::TapBlockOnGround m;
      m.numTaps = numTaps;
      ExternalInterface::MessageGameToEngine message;
      message.Set_TapBlockOnGround(m);
      SendMessage(message);
    }
    
    void UiGameController::SendStopAllMotors()
    {
      ExternalInterface::StopAllMotors m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_StopAllMotors(m);
      SendMessage(message);
    }
    
    void UiGameController::SendImageRequest(u8 mode, u8 robotID)
    {
      ExternalInterface::ImageRequest m;
      m.robotID = robotID;
      m.mode = mode;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ImageRequest(m);
      SendMessage(message);
    }
    
    void UiGameController::SendSetRobotImageSendMode(u8 mode)
    {
      // Determine resolution from "streamResolution" setting in the keyboard controller
      // node
      Vision::CameraResolution resolution = IMG_STREAM_RES;
      
      if (root_) {
        const std::string resString = root_->getField("streamResolution")->getSFString();
        printf("Attempting to switch robot to %s resolution.\n", resString.c_str());
        if(resString == "VGA") {
          resolution = Vision::CAMERA_RES_VGA;
        } else if(resString == "QVGA") {
          resolution = Vision::CAMERA_RES_QVGA;
        } else if(resString == "CVGA") {
          resolution = Vision::CAMERA_RES_CVGA;
        } else {
          printf("Unsupported streamResolution = %s\n", resString.c_str());
        }
      }
      
      ExternalInterface::SetRobotImageSendMode m;
      m.mode = mode;
      m.resolution = resolution;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetRobotImageSendMode(m);
      SendMessage(message);
    }
    
    void UiGameController::SendSaveImages(SaveMode_t mode, bool alsoSaveState)
    {
      ExternalInterface::SaveImages m;
      m.mode = mode;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SaveImages(m);
      SendMessage(message);
      
      if(alsoSaveState) {
        ExternalInterface::SaveRobotState msgSaveState;
        msgSaveState.mode = mode;
        ExternalInterface::MessageGameToEngine messageWrapper;
        messageWrapper.Set_SaveRobotState(msgSaveState);
        SendMessage(messageWrapper);
      }
    }
    
    void UiGameController::SendEnableDisplay(bool on)
    {
      ExternalInterface::EnableDisplay m;
      m.enable = on;
      ExternalInterface::MessageGameToEngine message;
      message.Set_EnableDisplay(m);
      SendMessage(message);
   }
    
    void UiGameController::SendSetHeadlights(u8 intensity)
    {
      ExternalInterface::SetHeadlights m;
      m.intensity = intensity;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetHeadlights(m);
      SendMessage(message);
    }
    
    void UiGameController::SendExecutePathToPose(const Pose3d& p, const bool useManualSpeed)
    {
      ExternalInterface::GotoPose m;
      m.x_mm = p.GetTranslation().x();
      m.y_mm = p.GetTranslation().y();
      m.rad = p.GetRotationAngle<'Z'>().ToFloat();
      m.level = 0;
      m.useManualSpeed = static_cast<u8>(useManualSpeed);
      ExternalInterface::MessageGameToEngine message;
      message.Set_GotoPose(m);
      SendMessage(message);
    }
    
    void UiGameController::SendPlaceObjectOnGroundSequence(const Pose3d& p, const bool useManualSpeed)
    {
      ExternalInterface::PlaceObjectOnGround m;
      m.x_mm = p.GetTranslation().x();
      m.y_mm = p.GetTranslation().y();
      m.rad = p.GetRotationAngle<'Z'>().ToFloat();
      m.level = 0;
      m.useManualSpeed = static_cast<u8>(useManualSpeed);
      ExternalInterface::MessageGameToEngine message;
      message.Set_PlaceObjectOnGround(m);
      SendMessage(message);
    }
    
    void UiGameController::SendExecuteTestPlan()
    {
      ExternalInterface::ExecuteTestPlan m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ExecuteTestPlan(m);
      SendMessage(message);
    }
    
    void UiGameController::SendClearAllBlocks()
    {
      ExternalInterface::ClearAllBlocks m;
      m.robotID = 1;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ClearAllBlocks(m);
      SendMessage(message);
    }
    
    void UiGameController::SendClearAllObjects()
    {
      ExternalInterface::ClearAllObjects m;
      m.robotID = 1;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ClearAllObjects(m);
      SendMessage(message);
    }
    
    void UiGameController::SendSelectNextObject()
    {
      ExternalInterface::SelectNextObject m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SelectNextObject(m);
      SendMessage(message);
    }
    
    void UiGameController::SendPickAndPlaceObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed)
    {
      ExternalInterface::PickAndPlaceObject m;
      m.usePreDockPose = static_cast<u8>(usePreDockPose);
      m.useManualSpeed = static_cast<u8>(useManualSpeed);
      m.objectID = -1;
      ExternalInterface::MessageGameToEngine message;
      message.Set_PickAndPlaceObject(m);
      SendMessage(message);
    }
    
    void UiGameController::SendPickAndPlaceSelectedObject(const bool usePreDockPose, const bool useManualSpeed)
    {
      SendPickAndPlaceObject(-1, usePreDockPose, useManualSpeed);
    }

    void UiGameController::SendRollObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed)
    {
      ExternalInterface::RollObject m;
      m.usePreDockPose = static_cast<u8>(usePreDockPose);
      m.useManualSpeed = static_cast<u8>(useManualSpeed);
      m.objectID = -1;
      ExternalInterface::MessageGameToEngine message;
      message.Set_RollObject(m);
      SendMessage(message);
    }
    
    void UiGameController::SendRollSelectedObject(const bool usePreDockPose, const bool useManualSpeed)
    {
      SendRollObject(-1, usePreDockPose, useManualSpeed);
    }

    
    void UiGameController::SendTraverseSelectedObject(const bool usePreDockPose, const bool useManualSpeed)
    {
      ExternalInterface::TraverseObject m;
      m.usePreDockPose = static_cast<u8>(usePreDockPose);
      m.useManualSpeed = static_cast<u8>(useManualSpeed);
      ExternalInterface::MessageGameToEngine message;
      message.Set_TraverseObject(m);
      SendMessage(message);
    }
    
    void UiGameController::SendExecuteBehavior(BehaviorManager::Mode mode)
    {
      ExternalInterface::ExecuteBehavior m;
      m.behaviorMode = mode;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ExecuteBehavior(m);
      SendMessage(message);
    }
  
    void UiGameController::SendSetNextBehaviorState(BehaviorManager::BehaviorState nextState)
    {
      ExternalInterface::SetBehaviorState m;
      m.behaviorState = nextState;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetBehaviorState(m);
      SendMessage(message);
    }
    
    void UiGameController::SendAbortPath()
    {
      ExternalInterface::AbortPath m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_AbortPath(m);
      SendMessage(message);
    }
    
    void UiGameController::SendAbortAll()
    {
      ExternalInterface::AbortAll m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_AbortAll(m);
      SendMessage(message);
    }
    
    void UiGameController::SendDrawPoseMarker(const Pose3d& p)
    {
      ExternalInterface::DrawPoseMarker m;
      m.x_mm = p.GetTranslation().x();
      m.y_mm = p.GetTranslation().y();
      m.rad = p.GetRotationAngle<'Z'>().ToFloat();
      m.level = 0;
      ExternalInterface::MessageGameToEngine message;
      message.Set_DrawPoseMarker(m);
      SendMessage(message);
    }
    
    void UiGameController::SendErasePoseMarker()
    {
      ExternalInterface::ErasePoseMarker m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ErasePoseMarker(m);
      SendMessage(message);
    }

    void UiGameController::SendWheelControllerGains(const f32 kpLeft, const f32 kiLeft, const f32 maxErrorSumLeft,
                                  const f32 kpRight, const f32 kiRight, const f32 maxErrorSumRight)
    {
      ExternalInterface::SetWheelControllerGains m;
      m.kpLeft = kpLeft;
      m.kiLeft = kiLeft;
      m.maxIntegralErrorLeft = maxErrorSumLeft;
      m.kpRight = kpRight;
      m.kiRight = kiRight;
      m.maxIntegralErrorRight = maxErrorSumRight;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetWheelControllerGains(m);
      SendMessage(message);
    }

    
    void UiGameController::SendHeadControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum)
    {
      ExternalInterface::SetHeadControllerGains m;
      m.kp = kp;
      m.ki = ki;
      m.kd = kd;
      m.maxIntegralError = maxErrorSum;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetHeadControllerGains(m);
      SendMessage(message);
    }
    
    void UiGameController::SendLiftControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum)
    {
      ExternalInterface::SetLiftControllerGains m;
      m.kp = kp;
      m.ki = ki;
      m.kd = kd;
      m.maxIntegralError = maxErrorSum;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetLiftControllerGains(m);
      SendMessage(message);
    }
    
    void UiGameController::SendSteeringControllerGains(const f32 k1, const f32 k2)
    {
      ExternalInterface::SetSteeringControllerGains m;
      m.k1 = k1;
      m.k2 = k2;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetSteeringControllerGains(m);
      SendMessage(message);
    }
    
    void UiGameController::SendSetRobotVolume(const f32 volume)
    {
      ExternalInterface::SetRobotVolume m;
      m.volume = volume;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetRobotVolume(m);
      SendMessage(message);
    }
    
    void UiGameController::SendStartTestMode(TestMode mode, s32 p1, s32 p2, s32 p3)
    {
      ExternalInterface::StartTestMode m;
      m.mode = mode;
      m.p1 = p1;
      m.p2 = p2;
      m.p3 = p3;
      ExternalInterface::MessageGameToEngine message;
      message.Set_StartTestMode(m);
      SendMessage(message);
    }
    
    void UiGameController::SendIMURequest(u32 length_ms)
    {
      ExternalInterface::IMURequest m;
      m.length_ms = length_ms;
      ExternalInterface::MessageGameToEngine message;
      message.Set_IMURequest(m);
      SendMessage(message);
    }
    
    void UiGameController::SendAnimation(const char* animName, u32 numLoops)
    {
      static double lastSendTime_sec = -1e6;
      
      // Don't send repeated animation commands within a half second
      if(_supervisor.getTime() > lastSendTime_sec + 0.5f)
      {
        PRINT_NAMED_INFO("SendAnimation", "sending %s", animName);
        ExternalInterface::PlayAnimation m;
        //m.animationID = animId;
        m.robotID = 1;
        m.animationName = animName;
        m.numLoops = numLoops;
        ExternalInterface::MessageGameToEngine message;
        message.Set_PlayAnimation(m);
        SendMessage(message);
        lastSendTime_sec = _supervisor.getTime();
      } else {
        PRINT_NAMED_INFO("SendAnimation", "Ignoring duplicate SendAnimation keystroke.");
      }
      
    }

    void UiGameController::SendReplayLastAnimation()
    {
      ExternalInterface::ReplayLastAnimation m;
      m.numLoops = 1;
      m.robotID = 1;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ReplayLastAnimation(m);
      SendMessage(message);
    }

    void UiGameController::SendReadAnimationFile()
    {
      ExternalInterface::ReadAnimationFile m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_ReadAnimationFile(m);
      SendMessage(message);
    }
    
    void UiGameController::SendSetIdleAnimation(const std::string &animName) {
      ExternalInterface::SetIdleAnimation msg;
      msg.robotID = 1;
      msg.animationName = animName;
      ExternalInterface::MessageGameToEngine message;
      message.Set_SetIdleAnimation(msg);
      SendMessage(message);
    }
    
    void UiGameController::SendQueuePlayAnimAction(const std::string &animName, u32 numLoops, QueueActionPosition pos) {
      ExternalInterface::QueueSingleAction msg;
      msg.robotID = 1;
      msg.inSlot = 1;
      msg.position = pos;
      msg.actionType = RobotActionType::PLAY_ANIMATION;
      msg.action.playAnimation.animationName = animName;
      msg.action.playAnimation.numLoops = numLoops;

      ExternalInterface::MessageGameToEngine message;
      message.Set_QueueSingleAction(msg);
      SendMessage(message);
    }
    
    void UiGameController::SendCancelAction() {
      ExternalInterface::CancelAction msg;
      msg.actionType = RobotActionType::UNKNOWN;
      msg.robotID = 1;
      ExternalInterface::MessageGameToEngine message;
      message.Set_CancelAction(msg);
      SendMessage(message);
    }
    
    void UiGameController::SendStartFaceTracking(u8 timeout_sec)
    {
      ExternalInterface::StartFaceTracking m;
      m.timeout_sec = timeout_sec;
      ExternalInterface::MessageGameToEngine message;
      message.Set_StartFaceTracking(m);
      SendMessage(message);
    }
    
    void UiGameController::SendStopFaceTracking()
    {
      ExternalInterface::StopFaceTracking m;
      ExternalInterface::MessageGameToEngine message;
      message.Set_StopFaceTracking(m);
      SendMessage(message);
      
      // For now, have to re-enable marker finding b/c turning on face
      // tracking will have stopped it:
      ExternalInterface::StartLookingForMarkers m2;
      message.Set_StartLookingForMarkers(m2);
      SendMessage(message);
    }

    void UiGameController::SendVisionSystemParams()
    {
      if (root_) {
         // Vision system params
         ExternalInterface::SetVisionSystemParams p;
         p.autoexposureOn = root_->getField("camera_autoexposureOn")->getSFInt32();
         p.exposureTime = root_->getField("camera_exposureTime")->getSFFloat();
         p.integerCountsIncrement = root_->getField("camera_integerCountsIncrement")->getSFInt32();
         p.minExposureTime = root_->getField("camera_minExposureTime")->getSFFloat();
         p.maxExposureTime = root_->getField("camera_maxExposureTime")->getSFFloat();
         p.highValue = root_->getField("camera_highValue")->getSFInt32();
         p.percentileToMakeHigh = root_->getField("camera_percentileToMakeHigh")->getSFFloat();
         p.limitFramerate = root_->getField("camera_limitFramerate")->getSFInt32();
         
         printf("New Camera params\n");
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetVisionSystemParams(p);
        SendMessage(message);
       }
    }

    void UiGameController::SendFaceDetectParams()
    {
      if (root_) {
        // Face Detect params
        ExternalInterface::SetFaceDetectParams p;
        p.scaleFactor = root_->getField("fd_scaleFactor")->getSFFloat();
        p.minNeighbors = root_->getField("fd_minNeighbors")->getSFInt32();
        p.minObjectHeight = root_->getField("fd_minObjectHeight")->getSFInt32();
        p.minObjectWidth = root_->getField("fd_minObjectWidth")->getSFInt32();
        p.maxObjectHeight = root_->getField("fd_maxObjectHeight")->getSFInt32();
        p.maxObjectWidth = root_->getField("fd_maxObjectWidth")->getSFInt32();
        
        printf("New Face detect params\n");
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetFaceDetectParams(p);
        SendMessage(message);
      }
    }
    
    void UiGameController::SendForceAddRobot()
    {
      if (root_) {
        ExternalInterface::ForceAddRobot msg;
        msg.isSimulated = false;
        msg.ipAddress.fill('\0'); // ensure null-termination after copy below
        
        webots::Field* ipField = root_->getField("forceAddIP");
        webots::Field* idField = root_->getField("forceAddID");
        
        if(ipField != nullptr && idField != nullptr) {
          const std::string ipStr = ipField->getSFString();
          std::copy(ipStr.begin(), ipStr.end(), msg.ipAddress.data());
          
          msg.robotID = static_cast<u8>(idField->getSFInt32());
          
          printf("Sending message to force-add robot %d at %s\n", msg.robotID, ipStr.c_str());
          ExternalInterface::MessageGameToEngine message;
          message.Set_ForceAddRobot(msg);
          SendMessage(message);
        } else {
          printf("ERROR: No 'forceAddIP' / 'forceAddID' field(s) found!\n");
        }
      }
    }
    
    void UiGameController::QuitWebots(s32 status)
    {
      _supervisor.simulationQuit(status);
    }
    
    
    s32 UiGameController::GetStepTimeMS()
    {
      return _stepTimeMS;
    }
    
    webots::Supervisor* UiGameController::GetSupervisor()
    {
      return &_supervisor;
    }
    
    const Pose3d& UiGameController::GetRobotPose()
    {
      return _robotPose;
    }
    
    const Pose3d& UiGameController::GetRobotPoseActual()
    {
      return _robotPoseActual;
    }
    
    const UiGameController::ObservedObject& UiGameController::GetCurrentlyObservedObject()
    {
      return _currentlyObservedObject;
    }

  } // namespace Cozmo
} // namespace Anki
