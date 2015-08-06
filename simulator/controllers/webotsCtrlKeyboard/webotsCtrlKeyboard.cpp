/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <opencv2/imgproc/imgproc.hpp>

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/ledTypes.h"
#include "anki/cozmo/shared/activeBlockTypes.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/point_impl.h"

#include "anki/vision/basestation/image.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/messaging/shared/TcpClient.h"
#include "anki/cozmo/game/comms/gameMessageHandler.h"
#include "anki/cozmo/game/comms/gameComms.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/block.h"
#include "clad/types/actionTypes.h"

#include <stdio.h>
#include <string.h>
#include <fstream>

// Webots includes
#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <webots/GPS.hpp>
#include <webots/Compass.hpp>

// SDL for gamepad control (specifically Logitech Rumblepad F510)
// Gamepad should be in Direct mode (switch on back)
#define ENABLE_GAMEPAD_SUPPORT 0
#if(ENABLE_GAMEPAD_SUPPORT)
#include <SDL.h>
#define DEBUG_GAMEPAD 0
#endif

namespace Anki {
  namespace Cozmo {
    
    // Slow down keyboard polling to avoid duplicate commands?
    const s32 KB_TIME_STEP = BS_TIME_STEP;

    namespace WebotsKeyboardController {
      
      webots::Supervisor inputController;
      
      // Private memers:
      namespace {
        // Constants for Webots:
        ExternalInterface::Ping _pingMsg;
        
        std::set<int> lastKeysPressed_;
        
        bool wasMovingWheels_ = false;
        bool wasMovingHead_   = false;
        bool wasMovingLift_   = false;
        
        webots::GPS* gps_;
        webots::Compass* compass_;
        
        webots::Node* root_ = nullptr;
        
        u8 poseMarkerMode_ = 0;
        Anki::Pose3d prevPoseMarkerPose_;
        Anki::Pose3d poseMarkerPose_;
        webots::Field* poseMarkerDiffuseColor_ = nullptr;
        double poseMarkerColor_[2][3] = { {0.1, 0.8, 0.1} // Goto pose color
          ,{0.8, 0.1, 0.1} // Place object color
        };
        
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
        
        BehaviorManager::Mode behaviorMode_ = BehaviorManager::None;
        
        // Socket connection to basestation
        TcpClient bsClient;
        char sendBuf[128];
        
        double lastKeyPressTime_;
        
        #if(ENABLE_GAMEPAD_SUPPORT)
        SDL_GameController* js_;
        
        //Event handler
        SDL_Event sdlEvent_;
        
        //Normalized direction
        typedef enum {
          GC_ANALOG_LEFT_X,
          GC_ANALOG_LEFT_Y,
          GC_ANALOG_RIGHT_X,
          GC_ANALOG_RIGHT_Y,
          GC_LT_BUTTON,
          GC_RT_BUTTON,
          GC_NUM_AXES
        } GameControllerAxis_t;
        
        s16 gc_axisValues_[GC_NUM_AXES] = {0};
        
        typedef enum {
          GC_BUTTON_A = 0,
          GC_BUTTON_B,
          GC_BUTTON_X,
          GC_BUTTON_Y,
          GC_BUTTON_BACK,
          
          GC_BUTTON_START = 6,
          GC_BUTTON_ANALOG_LEFT,
          GC_BUTTON_ANALOG_RIGHT,
          
          GC_BUTTON_LB = 9,
          GC_BUTTON_RB,
          
          GC_BUTTON_DIR_UP = 11,
          GC_BUTTON_DIR_DOWN,
          GC_BUTTON_DIR_LEFT,
          GC_BUTTON_DIR_RIGHT,
          
          GC_NUM_BUTTONS
        } GameControllerButton_t;
        
        bool gc_buttonPressed_[GC_NUM_BUTTONS];
        
        const s16 ANALOG_INPUT_DEAD_ZONE_THRESH = 1000;
        const f32 ANALOG_INPUT_MAX_DRIVE_SPEED = 100; // mm/s
        const f32 MAX_ANALOG_RADIUS = 300;
        #endif // ENABLE_GAMEPAD_SUPPORT

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
        
        Point2f robotPosition_;
        
        /*
        u32 imgID_ = 0;
        u8 imgData_[3*640*480];
        u32 imgBytes_ = 0;
        u32 imgWidth_, imgHeight_ = 0;
        */
        Vision::ImageDeChunker _imageDeChunker;
        
        // Save robot image to file
        bool saveRobotImageToFile_ = false;
        
      } // private namespace
      
      // Forward declarations
      void ProcessKeystroke(void);
      void ProcessJoystick(void);
      void PrintHelp(void);
      void SendMessage(const ExternalInterface::MessageGameToEngine& msg);
      void SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps);
      void SendTurnInPlace(const f32 angle_rad);
      void SendTurnInPlaceAtSpeed(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2);
      void SendMoveHead(const f32 speed_rad_per_sec);
      void SendMoveLift(const f32 speed_rad_per_sec);
      void SendMoveHeadToAngle(const f32 rad, const f32 speed, const f32 accel, const f32 duration_sec = 0.f);
      void SendMoveLiftToHeight(const f32 mm, const f32 speed, const f32 accel, const f32 duration_sec = 0.f);
      void SendTapBlockOnGround(const u8 numTaps);
      void SendStopAllMotors();
      void SendImageRequest(u8 mode, u8 robotID);
      void SendSetRobotImageSendMode(u8 mode);
      void SendSaveImages(SaveMode_t mode, bool alsoSaveState=false);
      void SendEnableDisplay(bool on);
      void SendSetHeadlights(u8 intensity);
      void SendExecutePathToPose(const Pose3d& p, const bool useManualSpeed);
      void SendPlaceObjectOnGroundSequence(const Pose3d& p, const bool useManualSpeed);
      void SendPickAndPlaceObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed);
      void SendPickAndPlaceSelectedObject(const bool usePreDockPose, const bool useManualSpeed);
      void SendRollObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed);
      void SendRollSelectedObject(const bool usePreDockPose, const bool useManualSpeed);
      void SendTraverseSelectedObject(const bool usePreDockPose, const bool useManualSpeed);
      void SendExecuteTestPlan();
      void SendClearAllBlocks();
      void SendClearAllObjects();
      void SendSelectNextObject();
      void SendExecuteBehavior(BehaviorManager::Mode mode);
      void SendSetNextBehaviorState(BehaviorManager::BehaviorState nextState);
      void SendAbortPath();
      void SendAbortAll();
      void SendDrawPoseMarker(const Pose3d& p);
      void SendErasePoseMarker();
      void SendWheelControllerGains(const f32 kpLeft, const f32 kiLeft, const f32 maxErrorSumLeft,
                                    const f32 kpRight, const f32 kiRight, const f32 maxErrorSumRight);
      void SendHeadControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum);
      void SendLiftControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum);
      void SendSteeringControllerGains(const f32 k1, const f32 k2);
      void SendSetRobotVolume(const f32 volume);
      void SendStartTestMode(TestMode mode, s32 p1 = 0, s32 p2 = 0, s32 p3 = 0);
      void SendIMURequest(u32 length_ms);
      void SendAnimation(const char* animName, u32 numLoops);
      void SendReplayLastAnimation();
      void SendReadAnimationFile();
      void SendStartFaceTracking(u8 timeout_sec);
      void SendStopFaceTracking();
      void SendVisionSystemParams();
      void SendFaceDetectParams();
      void SendForceAddRobot();
      
      
      // ======== Message handler callbacks =======
      
      // TODO: Update these not to need robotID
      
      void HandleRobotStateUpdate(ExternalInterface::RobotState const& msg)
      {
        robotPosition_ = {msg.pose_x, msg.pose_y};
      }
      
      void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg)
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
      }
      
      void HandleRobotObservedNothing(ExternalInterface::RobotObservedNothing const& msg)
      {
        currentlyObservedObject.Reset();
      }
      
      void HandleRobotDeletedObject(ExternalInterface::RobotDeletedObject const& msg)
      {
        printf("Robot %d reported deleting object %d\n", msg.robotID, msg.objectID);
      }

      void HandleRobotConnection(const ExternalInterface::RobotAvailable& msgIn)
      {
        // Just send a message back to the game to connect to any robot that's
        // advertising (since we don't have a selection mechanism here)
        printf("Sending message to command connection to robot %d.\n", msgIn.robotID);
        ExternalInterface::ConnectToRobot msgOut;
        msgOut.robotID = msgIn.robotID;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ConnectToRobot(msgOut);
        SendMessage(message);
      }
      
      void HandleUiDeviceConnection(const ExternalInterface::UiDeviceAvailable& msgIn)
      {
        // Just send a message back to the game to connect to any UI device that's
        // advertising (since we don't have a selection mechanism here)
        printf("Sending message to command connection to UI device %d.\n", msgIn.deviceID);
        ExternalInterface::ConnectToUiDevice msgOut;
        msgOut.deviceID = msgIn.deviceID;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ConnectToUiDevice(msgOut);
        SendMessage(message);
      }
      
      void HandleRobotConnected(ExternalInterface::RobotConnected const &msg)
      {
        // Once robot connects, set resolution
        SendSetRobotImageSendMode(ISM_STREAM);
      }
      
      void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
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
        
      }
      
      // For processing image chunks arriving from robot.
      // Sends complete images to VizManager for visualization (and possible saving).
      void HandleImageChunk(ExternalInterface::ImageChunk const& msg)
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

      } // HandleImageChunk()
      
      
      void HandleActiveObjectMoved(ExternalInterface::ActiveObjectMoved const& msg)
      {
       PRINT_NAMED_INFO("HandleActiveObjectMoved", "Received message that object %d moved. Accel=(%f,%f,%f). UpAxis=%d\n",
                        msg.objectID, msg.xAccel, msg.yAccel, msg.zAccel, msg.upAxis);
      }
      
      void HandleActiveObjectStoppedMoving(ExternalInterface::ActiveObjectStoppedMoving const& msg)
      {
        PRINT_NAMED_INFO("HandleActiveObjectStoppedMoving", "Received message that object %d stopped moving%s. UpAxis=%d\n",
                         msg.objectID, (msg.rolled ? " and rolled" : ""), msg.upAxis);
      }
      
      void HandleActiveObjectTapped(ExternalInterface::ActiveObjectTapped const& msg)
      {
        PRINT_NAMED_INFO("HandleActiveObjectTapped", "Received message that object %d was tapped %d times.\n",
                         msg.objectID, msg.numTaps);
      }

      void HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg)
      {
        printf("Animation available: %s\n", msg.animName.c_str());
      }
      
      // ===== End of message handler callbacks ====
      
      
      void Init()
      {
        memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
        
        // Make root point to WebotsKeyBoardController node
        root_ = inputController.getSelf();
        
        // Set deviceID
        int deviceID = root_->getField("deviceID")->getSFInt32();
        
        // Get engine IP
        std::string engineIP = root_->getField("engineIP")->getSFString();
        
        // Startup comms with engine
        if (!gameComms_) {
          printf("Registering with advertising service at %s:%d\n", engineIP.c_str(), UI_ADVERTISEMENT_REGISTRATION_PORT);
          gameComms_ = new GameComms(deviceID, UI_MESSAGE_SERVER_LISTEN_PORT,
                                     engineIP.c_str(),
                                     UI_ADVERTISEMENT_REGISTRATION_PORT);
        }
        
        
        while(!gameComms_->IsInitialized()) {
          PRINT_NAMED_INFO("KeyboardController.Init",
                           "Waiting for gameComms to initialize...\n");
          inputController.step(KB_TIME_STEP);
          gameComms_->Update();
        }
        msgHandler_.Init(gameComms_);
        
        // Register callbacks for incoming messages from game
        // TODO: Have CLAD generate this?
        msgHandler_.RegisterCallbackForMessage([](const ExternalInterface::MessageEngineToGame& message) {
          switch (message.GetTag()) {
            case ExternalInterface::MessageEngineToGame::Tag::RobotConnected:
              HandleRobotConnected(message.Get_RobotConnected());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::RobotState:
              HandleRobotStateUpdate(message.Get_RobotState());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::RobotObservedObject:
              HandleRobotObservedObject(message.Get_RobotObservedObject());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::UiDeviceAvailable:
              HandleUiDeviceConnection(message.Get_UiDeviceAvailable());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::RobotAvailable:
              HandleRobotConnection(message.Get_RobotAvailable());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::ImageChunk:
              HandleImageChunk(message.Get_ImageChunk());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::RobotDeletedObject:
              HandleRobotDeletedObject(message.Get_RobotDeletedObject());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::RobotCompletedAction:
              HandleRobotCompletedAction(message.Get_RobotCompletedAction());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::ActiveObjectMoved:
              HandleActiveObjectMoved(message.Get_ActiveObjectMoved());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::ActiveObjectStoppedMoving:
              HandleActiveObjectStoppedMoving(message.Get_ActiveObjectStoppedMoving());
              break;
            case ExternalInterface::MessageEngineToGame::Tag::ActiveObjectTapped:
              HandleActiveObjectTapped(message.Get_ActiveObjectTapped());
              break;
          case ExternalInterface::MessageEngineToGame::Tag::AnimationAvailable:
              HandleAnimationAvailable(message.Get_AnimationAvailable());
              break;
            default:
              // ignore
              break;
          }
        });
        
        inputController.keyboardEnable(BS_TIME_STEP);
        
        gps_ = inputController.getGPS("gps");
        compass_ = inputController.getCompass("compass");
        
        gps_->enable(BS_TIME_STEP);
        compass_->enable(BS_TIME_STEP);
        
        poseMarkerDiffuseColor_ = root_->getField("poseMarkerDiffuseColor");
        
        #if(ENABLE_GAMEPAD_SUPPORT)
        // Look for gamepad
        if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
          printf("ERROR: Failed to init SDL\n");
          return;
        }
        if (SDL_NumJoysticks() > 0) {
          // Open first joystick. Assuming it's the Logitech Rumble Gamepad F510.
          js_ = SDL_GameControllerOpen(0);
          if (js_ == NULL) {
            printf("ERROR: Unable to open gamepad\n");
          }
        }
        #endif

        cozmoCam_ = inputController.getDisplay("uiCamDisplay");

        uiState_ = UI_WAITING_FOR_GAME;
      }
      
      void PrintHelp()
      {
        printf("\nBasestation keyboard control\n");
        printf("===============================\n");
        printf("                           Drive:  arrows  (Hold shift for slower speeds)\n");
        printf("               Move lift up/down:  a/z\n");
        printf("               Move head up/down:  s/x\n");
        printf("             Lift low/high/carry:  1/2/3\n");
        printf("            Head down/forward/up:  4/5/6\n");
        printf("                Toggle headlight:  h\n");
        printf("            Request *game* image:  i\n");
        printf("           Request *robot* image:  Alt+i\n");
        printf("      Toggle *game* image stream:  Shift+i\n");
        printf("     Toggle *robot* image stream:  Alt+Shift+i\n");
        printf("              Toggle save images:  e\n");
        printf("        Toggle VizObject display:  d\n");
        printf("   Toggle addition/deletion mode:  Shift+d\n");
        printf("Goto/place object at pose marker:  g\n");
        printf("         Toggle pose marker mode:  Shift+g\n");
        printf("              Cycle block select:  .\n");
        printf("              Clear known blocks:  c\n");
        printf("         Clear all known objects:  Alt+c\n");
        printf("                      Creep mode:  Shift+c\n");
        printf("          Dock to selected block:  p\n");
        printf("          Dock from current pose:  Shift+p\n");
        printf("    Travel up/down selected ramp:  r\n");
        printf("       Start June 2014 dice demo:  j\n");
        printf("              Abort current path:  q\n");
        printf("                Abort everything:  Shift+q\n");
        printf("         Update controller gains:  k\n");
        printf("             Cycle sound schemes:  m\n");
        printf("                 Request IMU log:  o\n");
        printf("            Toggle face tracking:  f (Shift+f)\n");
        printf("                      Test modes:  Alt + Testmode#\n");
        printf("                Follow test plan:  t\n");
        printf("        Force-add specifed robot:  Shift+r\n");
        printf("                      Print help:  ?\n");
        printf("\n");
      }
      
      //Check the keyboard keys and issue robot commands
      void ProcessKeystroke()
      {
        bool movingHead   = false;
        bool movingLift   = false;
        bool movingWheels = false;
        s8 steeringDir = 0;  // -1 = left, 0 = straight, 1 = right
        s8 throttleDir = 0;  // -1 = reverse, 0 = stop, 1 = forward
        
        f32 leftSpeed = 0.f;
        f32 rightSpeed = 0.f;
        
        f32 commandedLiftSpeed = 0.f;
        f32 commandedHeadSpeed = 0.f;
        
        f32 wheelSpeed = root_->getField("driveSpeedNormal")->getSFFloat();
        
        f32 steeringCurvature = root_->getField("steeringCurvature")->getSFFloat();
        
        static bool keyboardRestart = false;
        if (keyboardRestart) {
          inputController.keyboardDisable();
          inputController.keyboardEnable(BS_TIME_STEP);
          keyboardRestart = false;
        }
        
        // Get all keys pressed this tic
        std::set<int> keysPressed;
        int key;
        while((key = inputController.keyboardGetKey()) != 0) {
          keysPressed.insert(key);
        }
        
        // If exact same keys were pressed last tic, do nothing.
        if (lastKeysPressed_ == keysPressed) {
          return;
        }
        lastKeysPressed_ = keysPressed;
        
        
        for(auto key : keysPressed)
        {
          // Extract modifier key(s)
          int modifier_key = key & ~webots::Supervisor::KEYBOARD_KEY;
          
          // Set key to its modifier-less self
          key &= webots::Supervisor::KEYBOARD_KEY;
          
          lastKeyPressTime_ = inputController.getTime();
          
          // DEBUG: Display modifier key information
          /*
          printf("Key = '%c'", char(key));
          if(modifier_key) {
            printf(", with modifier keys: ");
            if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
              printf("ALT ");
            }
            if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
              printf("SHIFT ");
            }
            if(modifier_key & webots::Supervisor::KEYBOARD_CONTROL) {
              printf("CTRL/CMD ");
            }
           
          }
          printf("\n");
          */
          
          // Use slow motor speeds if SHIFT is pressed
          f32 liftSpeed = DEG_TO_RAD_F32(root_->getField("liftSpeedDegPerSec")->getSFFloat());
          f32 liftAccel = DEG_TO_RAD_F32(root_->getField("liftAccelDegPerSec2")->getSFFloat());
          f32 liftDurationSec = root_->getField("liftDurationSec")->getSFFloat();
          f32 headSpeed = DEG_TO_RAD_F32(root_->getField("headSpeedDegPerSec")->getSFFloat());
          f32 headAccel = DEG_TO_RAD_F32(root_->getField("headAccelDegPerSec2")->getSFFloat());
          f32 headDurationSec = root_->getField("headDurationSec")->getSFFloat();
          if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
            wheelSpeed = root_->getField("driveSpeedSlow")->getSFFloat();
            liftSpeed *= 0.5;
            headSpeed *= 0.5;
          } else if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
            wheelSpeed = root_->getField("driveSpeedTurbo")->getSFFloat();
          }
          
          // Speed of point turns (when no target angle specified). See SendTurnInPlaceAtSpeed().
          f32 pointTurnSpeed = std::fabs(root_->getField("pointTurnSpeed_degPerSec")->getSFFloat());
          
          
          //printf("keypressed: %d, modifier %d, orig_key %d, prev_key %d\n",
          //       key, modifier_key, key | modifier_key, lastKeyPressed_);
          
          // Check for test mode (alt + key)
          bool testMode = false;
          if (modifier_key & webots::Supervisor::KEYBOARD_ALT) {
            if (key >= '0' && key <= '9') {
              if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                // Hold shift down too to add 10 to the pressed key
                key += 10;
              }
              
              TestMode m = TestMode(key - '0');

              // Set parameters for special test cases
              s32 p1 = 0, p2 = 0, p3 = 0;
              switch(m) {
                case TM_DIRECT_DRIVE:
                  // p1: flags (See DriveTestFlags)
                  // p2: wheelPowerStepPercent (only applies if DTF_ENABLE_DIRECT_HAL_TEST is set)
                  // p3: wheelSpeed_mmps (only applies if DTF_ENABLE_DIRECT_HAL_TEST is not set)
                  //p1 = DTF_ENABLE_DIRECT_HAL_TEST | DTF_ENABLE_CYCLE_SPEEDS_TEST;
                  //p2 = 5;
                  //p3 = 20;
                  p1 = root_->getField("driveTest_flags")->getSFInt32();
                  p2 = 10;
                  p3 = root_->getField("driveTest_wheel_power")->getSFInt32();
                  break;
                case TM_LIFT:
                  p1 = root_->getField("liftTest_flags")->getSFInt32();
                  p2 = root_->getField("liftTest_nodCycleTimeMS")->getSFInt32();  // Nodding cycle time in ms (if LiftTF_NODDING flag is set)
                  p3 = 250;
                  break;
                case TM_HEAD:
                  p1 = root_->getField("headTest_flags")->getSFInt32();
                  p2 = root_->getField("headTest_nodCycleTimeMS")->getSFInt32();  // Nodding cycle time in ms (if HTF_NODDING flag is set)
                  p3 = 250;
                  break;
                case TM_LIGHTS:
                  // p1: flags (See LightTestFlags)
                  // p2: The LED channel to activate (applies if LTF_CYCLE_ALL not enabled)
                  // p3: The color to set it to (applies if LTF_CYCLE_ALL not enabled)
                  p1 = LTF_CYCLE_ALL;
                  p2 = LED_BACKPACK_RIGHT;
                  p3 = LED_GREEN;
                  break;
                default:
                  break;
              }
              
              printf("Sending test mode %d\n", m);
              SendStartTestMode(m,p1,p2,p3);

              testMode = true;
            }
          }
          
          if(!testMode) {
            // Check for (mostly) single key commands
            switch (key)
            {
              case webots::Robot::KEYBOARD_UP:
              {
                ++throttleDir;
                break;
              }
                
              case webots::Robot::KEYBOARD_DOWN:
              {
                --throttleDir;
                break;
              }
                
              case webots::Robot::KEYBOARD_LEFT:
              {
                --steeringDir;
                break;
              }
                
              case webots::Robot::KEYBOARD_RIGHT:
              {
                ++steeringDir;
                break;
              }
                
              case (s32)'<':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  SendTurnInPlaceAtSpeed(DEG_TO_RAD(pointTurnSpeed), DEG_TO_RAD(1440));
                } else {
                  SendTurnInPlace(DEG_TO_RAD(45));
                }
                break;
              }
                
              case (s32)'>':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  SendTurnInPlaceAtSpeed(DEG_TO_RAD(-pointTurnSpeed), DEG_TO_RAD(1440));
                } else {
                  SendTurnInPlace(DEG_TO_RAD(-45));
                }
                break;
              }
                
              case (s32)'S':
              {
                if(modifier_key == webots::Supervisor::KEYBOARD_ALT) {
                  // Re-read animations and send them to physical robot
                  SendReplayLastAnimation();
                } else {
                  commandedHeadSpeed += headSpeed;
                  movingHead = true;
                }
                break;
              }
                
              case (s32)'X':
              {
                commandedHeadSpeed -= headSpeed;
                movingHead = true;
                break;
              }
                
              case (s32)'A':
              {
                if(modifier_key == webots::Supervisor::KEYBOARD_ALT) {
                  // Re-read animations and send them to physical robot
                  SendReadAnimationFile();
                } else {
                  commandedLiftSpeed += liftSpeed;
                  movingLift = true;
                }
                break;
              }
                
              case (s32)'Z':
              {
                if(modifier_key == webots::Supervisor::KEYBOARD_ALT) {
                  // Tap carried block on the ground
                  SendTapBlockOnGround(1);
                } else {
                  commandedLiftSpeed -= liftSpeed;
                  movingLift = true;
                }
                break;
              }
                
              case '1': //set lift to low dock height
              {
                SendMoveLiftToHeight(LIFT_HEIGHT_LOWDOCK, liftSpeed, liftAccel, liftDurationSec);
                break;
              }
                
              case '2': //set lift to high dock height
              {
                SendMoveLiftToHeight(LIFT_HEIGHT_HIGHDOCK, liftSpeed, liftAccel, liftDurationSec);
                break;
              }
                
              case '3': //set lift to carry height
              {
                SendMoveLiftToHeight(LIFT_HEIGHT_CARRY, liftSpeed, liftAccel, liftDurationSec);
                break;
              }
                
              case '4': //set head to look all the way down
              {
                SendMoveHeadToAngle(MIN_HEAD_ANGLE, headSpeed, headAccel, headDurationSec);
                break;
              }
                
              case '5': //set head to straight ahead
              {
                SendMoveHeadToAngle(0, headSpeed, headAccel, headDurationSec);
                break;
              }
                
              case '6': //set head to look all the way up
              {
                SendMoveHeadToAngle(MAX_HEAD_ANGLE, headSpeed, headAccel, headDurationSec);
                break;
              }
                
              case (s32)' ': // (space bar)
              {
                SendStopAllMotors();
                break;
              }
                
              case (s32)'I':
              {
                //if(modifier_key & webots::Supervisor::KEYBOARD_CONTROL) {
                // CTRL/CMD+I - Tell physical robot to send a single image
                ImageSendMode_t mode = ISM_SINGLE_SHOT;
                
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  // CTRL/CMD+SHIFT+I - Toggle physical robot image streaming
                  static bool streamOn = false;
                  if (streamOn) {
                    mode = ISM_OFF;
                    printf("Turning robot image streaming OFF.\n");
                  } else {
                    mode = ISM_STREAM;
                    printf("Turning robot image streaming ON.\n");
                  }
                  streamOn = !streamOn;
                } else {
                  printf("Requesting single robot image.\n");
                }
                
                SendSetRobotImageSendMode(mode);

                break;
              }
                
              case (s32)'U':
              {
                // TODO: How to choose which robot
                const RobotID_t robotID = 1;
                
                // U - Request a single image from the game for a specified robot
                ImageSendMode_t mode = ISM_SINGLE_SHOT;
                
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  // SHIFT+I - Toggle image streaming from the game
                  static bool streamOn = true;
                  if (streamOn) {
                    mode = ISM_OFF;
                    printf("Turning game image streaming OFF.\n");
                  } else {
                    mode = ISM_STREAM;
                    printf("Turning game image streaming ON.\n");
                  }
                  streamOn = !streamOn;
                } else {
                  printf("Requesting single game image.\n");
                }
                
                SendImageRequest(mode, robotID);
                
                break;
              }
                
              case (s32)'E':
              {
                // Toggle saving of images to pgm
                SaveMode_t mode = SAVE_ONE_SHOT;
                
                const bool alsoSaveState = modifier_key & webots::Supervisor::KEYBOARD_ALT;
                
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  static bool streamOn = false;
                  if (streamOn) {
                    mode = SAVE_OFF;
                    printf("Saving robot image/state stream OFF.\n");
                  } else {
                    mode = SAVE_CONTINUOUS;
                    printf("Saving robot image %sstream ON.\n", alsoSaveState ? "and state " : "");
                  }
                  streamOn = !streamOn;
                } else {
                  printf("Saving single robot image%s.\n", alsoSaveState ? " and state message" : "");
                }
                
                SendSaveImages(mode, alsoSaveState);
                break;
              }
                
              case (s32)'D':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  
                  static const std::array<std::pair<bool,bool>,4> enableModes = {{
                    {false, false}, {false, true}, {true, false}, {true, true}
                  }};
                  static auto enableModeIter = enableModes.begin();
                  
                  printf("Setting addition/deletion mode to %s/%s.\n",
                         enableModeIter->first ? "TRUE" : "FALSE",
                         enableModeIter->second ? "TRUE" : "FALSE");
                  ExternalInterface::SetObjectAdditionAndDeletion msg;
                  msg.robotID = 1;
                  msg.enableAddition = enableModeIter->first;
                  msg.enableDeletion = enableModeIter->second;
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_SetObjectAdditionAndDeletion(msg);
                  SendMessage(msgWrapper);
                  
                  ++enableModeIter;
                  if(enableModeIter == enableModes.end()) {
                    enableModeIter = enableModes.begin();
                  }
                  
                } else {
                  static bool showObjects = false;
                  SendEnableDisplay(showObjects);
                  showObjects = !showObjects;
                }
                break;
              }
                
              case (s32)'H':
              {
                static bool headlightsOn = false;
                headlightsOn = !headlightsOn;
                SendSetHeadlights(headlightsOn ? 128 : 0);
                break;
              }
                
              case (s32)'G':
              {
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  poseMarkerMode_ = !poseMarkerMode_;
                  printf("Pose marker mode: %d\n", poseMarkerMode_);
                  poseMarkerDiffuseColor_->setSFColor(poseMarkerColor_[poseMarkerMode_]);
                  SendErasePoseMarker();
                  break;
                }
                
                bool useManualSpeed = (modifier_key & webots::Supervisor::KEYBOARD_ALT);
                  
                if (poseMarkerMode_ == 0) {
                  // Execute path to pose
                  printf("Going to pose marker at x=%f y=%f angle=%f (useManualSpeed: %d)\n",
                         poseMarkerPose_.GetTranslation().x(),
                         poseMarkerPose_.GetTranslation().y(),
                         poseMarkerPose_.GetRotationAngle<'Z'>().ToFloat(),
                         useManualSpeed);
                  SendExecutePathToPose(poseMarkerPose_, useManualSpeed);
                  SendMoveHeadToAngle(-0.26, headSpeed, headAccel);
                } else {
                  SendPlaceObjectOnGroundSequence(poseMarkerPose_, useManualSpeed);
                  // Make sure head is tilted down so that it can localize well
                  SendMoveHeadToAngle(-0.26, headSpeed, headAccel);
                  
                }
                break;
              }
                
              case (s32)'L':
              {
                static bool backpackLightsOn = false;
                
                ExternalInterface::SetBackpackLEDs msg;
                msg.robotID = 1;
                for(s32 i=0; i<NUM_BACKPACK_LEDS; ++i)
                {
                  msg.onColor[i] = 0;
                  msg.offColor[i] = 0;
                  msg.onPeriod_ms[i] = 1000;
                  msg.offPeriod_ms[i] = 2000;
                  msg.transitionOnPeriod_ms[i] = 500;
                  msg.transitionOffPeriod_ms[i] = 500;
                }
                
                if(!backpackLightsOn) {
                  msg.onColor[LED_BACKPACK_RIGHT]  = NamedColors::GREEN;
                  msg.onColor[LED_BACKPACK_LEFT]   = NamedColors::RED;
                  msg.onColor[LED_BACKPACK_BACK]   = NamedColors::BLUE;
                  msg.onColor[LED_BACKPACK_MIDDLE] = NamedColors::CYAN;
                  msg.onColor[LED_BACKPACK_FRONT]  = NamedColors::YELLOW;
                }
                
                ExternalInterface::MessageGameToEngine msgWrapper;
                msgWrapper.Set_SetBackpackLEDs(msg);
                msgHandler_.SendMessage(1, msgWrapper);

                backpackLightsOn = !backpackLightsOn;
                
                break;
              }
                
              case (s32)'T':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  static bool trackingObject = false;
                  
                  trackingObject = !trackingObject;

                  ExternalInterface::TrackToObject m;
                  m.robotID = 1;

                  if(trackingObject) {
                    const bool headOnly = modifier_key & webots::Supervisor::KEYBOARD_ALT;
                    
                    printf("Telling robot to track %sto the selected object %d\n",
                           headOnly ? "its head " : "",
                           currentlyObservedObject.id);
                    
                    m.objectID = currentlyObservedObject.id;
                    m.headOnly = headOnly;
                  } else {
                    // Disable tracking
                    m.objectID = u32_MAX;
                  }
                    
                  ExternalInterface::MessageGameToEngine message;
                  message.Set_TrackToObject(m);
                  SendMessage(message);

                } else {
                  SendExecuteTestPlan();
                }
                break;
              }
                
              case (s32)'.':
              {
                SendSelectNextObject();
                break;
              }
                
              case (s32)'C':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  if(BehaviorManager::CREEP == behaviorMode_) {
                    behaviorMode_ = BehaviorManager::None;
                  } else {
                    behaviorMode_ = BehaviorManager::CREEP;
                  }
                  
                  SendExecuteBehavior(behaviorMode_);
                }
                else if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  SendClearAllObjects();
                }
                else {
                  // 'c' without SHIFT
                  SendClearAllBlocks();
                }
                break;
              }
                
              case (s32)'P':
              {
                bool usePreDockPose = !(modifier_key & webots::Supervisor::KEYBOARD_SHIFT);
                bool useManualSpeed = (modifier_key & webots::Supervisor::KEYBOARD_ALT);
                
                SendPickAndPlaceSelectedObject(usePreDockPose, useManualSpeed);
                break;
              }
                
              case (s32)'R':
              {
                bool usePreDockPose = !(modifier_key & webots::Supervisor::KEYBOARD_SHIFT);
                bool useManualSpeed = (modifier_key & webots::Supervisor::KEYBOARD_ALT);
                
                SendTraverseSelectedObject(usePreDockPose, useManualSpeed);
                break;
              }
                
              case (s32)'W':
              {
                bool usePreDockPose = !(modifier_key & webots::Supervisor::KEYBOARD_SHIFT);
                bool useManualSpeed = (modifier_key & webots::Supervisor::KEYBOARD_ALT);
                
                SendRollSelectedObject(usePreDockPose, useManualSpeed);
                break;
              }

                
              case (s32)'J':
              {
                if (behaviorMode_ == BehaviorManager::June2014DiceDemo) {
                  SendExecuteBehavior(BehaviorManager::None);
                } else {
                  SendExecuteBehavior(BehaviorManager::June2014DiceDemo);
                }
                break;
              }
                
              case (s32)'Q':
              {
                /* Debugging U2G message for drawing quad
                ExternalInterface::VisualizeQuad msg;
                msg.xLowerRight = 250.f;
                msg.yLowerRight = -0250.f;
                msg.zLowerRight = 10.f*static_cast<f32>(std::rand())/static_cast<f32>(RAND_MAX) + 10.f;
                msg.xUpperLeft = -250.f;
                msg.yUpperLeft = 250.f;
                msg.zUpperLeft = 10.f*static_cast<f32>(std::rand())/static_cast<f32>(RAND_MAX) + 10.f;
                msg.xLowerLeft = -250.f;
                msg.yLowerLeft = -250.f;
                msg.zLowerLeft = 10.f*static_cast<f32>(std::rand())/static_cast<f32>(RAND_MAX) + 10.f;
                msg.xUpperRight = 250.f;
                msg.yUpperRight = 250.f;
                msg.zUpperRight = 10.f*static_cast<f32>(std::rand())/static_cast<f32>(RAND_MAX) + 10.f;
                msg.quadID = 998;
                msg.color = NamedColors::ORANGE;
                
                ExternalInterface::MessageGameToEngine msgWrapper;
                msgWrapper.Set_VisualizeQuad(msg);
                msgHandler_.SendMessage(1, msgWrapper);
                */
                
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  // SHIFT + Q: Cancel everything (paths, animations, docking, etc.)
                  SendAbortAll();
                } else if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  // ALT + Q: Cancel action
                  ExternalInterface::CancelAction msg;
                  msg.actionType = RobotActionType::UNKNOWN;
                  msg.robotID = 1;
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_CancelAction(msg);
                  msgHandler_.SendMessage(1, msgWrapper);
                  
                } else {
                  // Just Q: Just cancel path
                  SendAbortPath();
                }
                
                break;
              }
                
              case (s32)'K':
              {
                if (root_) {
                  
                  if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                    f32 steer_k1 = root_->getField("steerK1")->getSFFloat();
                    f32 steer_k2 = root_->getField("steerK2")->getSFFloat();
                    printf("New steering gains: k1 %f, k2 %f\n", steer_k1, steer_k2);
                    SendSteeringControllerGains(steer_k1, steer_k2);
                  } else {
                    
                    // Wheel gains
                    f32 kpLeft = root_->getField("lWheelKp")->getSFFloat();
                    f32 kiLeft = root_->getField("lWheelKi")->getSFFloat();
                    f32 maxErrorSumLeft = root_->getField("lWheelMaxErrorSum")->getSFFloat();
                    f32 kpRight = root_->getField("rWheelKp")->getSFFloat();
                    f32 kiRight = root_->getField("rWheelKi")->getSFFloat();
                    f32 maxErrorSumRight = root_->getField("rWheelMaxErrorSum")->getSFFloat();
                    printf("New wheel gains: left %f %f %f, right %f %f %f\n", kpLeft, kiLeft, maxErrorSumLeft, kpRight, kiRight, maxErrorSumRight);
                    SendWheelControllerGains(kpLeft, kiLeft, maxErrorSumLeft, kpRight, kiRight, maxErrorSumRight);
                    
                    // Head and lift gains
                    f32 kp = root_->getField("headKp")->getSFFloat();
                    f32 ki = root_->getField("headKi")->getSFFloat();
                    f32 kd = root_->getField("headKd")->getSFFloat();
                    f32 maxErrorSum = root_->getField("headMaxErrorSum")->getSFFloat();
                    printf("New head gains: kp=%f ki=%f kd=%f maxErrorSum=%f\n", kp, ki, kd, maxErrorSum);
                    SendHeadControllerGains(kp, ki, kd, maxErrorSum);
                    
                    kp = root_->getField("liftKp")->getSFFloat();
                    ki = root_->getField("liftKi")->getSFFloat();
                    kd = root_->getField("liftKd")->getSFFloat();
                    maxErrorSum = root_->getField("liftMaxErrorSum")->getSFFloat();
                    printf("New lift gains: kp=%f ki=%f kd=%f maxErrorSum=%f\n", kp, ki, kd, maxErrorSum);
                    SendLiftControllerGains(kp, ki, kd, maxErrorSum);
                  }
                } else {
                  printf("No WebotsKeyboardController was found in world\n");
                }
                break;
              }
                
              case (s32)'V':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  static bool visionWhileMovingEnabled = false;
                  visionWhileMovingEnabled = !visionWhileMovingEnabled;
                  printf("%s vision while moving.\n", (visionWhileMovingEnabled ? "Enabling" : "Disabling"));
                  ExternalInterface::VisionWhileMoving msg;
                  msg.enable = visionWhileMovingEnabled;
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_VisionWhileMoving(msg);
                  SendMessage(msgWrapper);
                } else if (modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  SendVisionSystemParams();
                  SendFaceDetectParams();
                } else {
                  f32 robotVolume = root_->getField("robotVolume")->getSFFloat();
                  SendSetRobotVolume(robotVolume);
                }
                break;
              }
                
              case (s32)'B':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_ALT &&
                   modifier_key & webots::Supervisor::KEYBOARD_SHIFT)
                {
                  ExternalInterface::SetAllActiveObjectLEDs msg;
                  static int jsonMsgCtr = 0;
                  Json::Value jsonMsg;
                  Json::Reader reader;
                  std::string jsonFilename("../webotsCtrlGameEngine/SetBlockLights_" + std::to_string(jsonMsgCtr++) + ".json");
                  std::ifstream jsonFile(jsonFilename);
                  
                  if(jsonFile.fail()) {
                    jsonMsgCtr = 0;
                    jsonFilename = "../webotsCtrlGameEngine/SetBlockLights_" + std::to_string(jsonMsgCtr++) + ".json";
                    jsonFile.open(jsonFilename);
                  }
                  
                  printf("Sending message from: %s\n", jsonFilename.c_str());
                  
                  reader.parse(jsonFile, jsonMsg);
                  jsonFile.close();
                  //ExternalInterface::SetActiveObjectLEDs msg(jsonMsg);
                  msg.robotID = 1;
                  msg.makeRelative = RELATIVE_LED_MODE_OFF;
                  msg.objectID = jsonMsg["objectID"].asUInt();
                  for(s32 iLED = 0; iLED<NUM_BLOCK_LEDS; ++iLED) {
                    msg.onColor[iLED]  = jsonMsg["onColor"][iLED].asUInt();
                    msg.offColor[iLED]  = jsonMsg["offColor"][iLED].asUInt();
                    msg.onPeriod_ms[iLED]  = jsonMsg["onPeriod_ms"][iLED].asUInt();
                    msg.offPeriod_ms[iLED]  = jsonMsg["offPeriod_ms"][iLED].asUInt();
                    msg.transitionOnPeriod_ms[iLED]  = jsonMsg["transitionOnPeriod_ms"][iLED].asUInt();
                    msg.transitionOffPeriod_ms[iLED]  = jsonMsg["transitionOffPeriod_ms"][iLED].asUInt();
                  }
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_SetAllActiveObjectLEDs(msg);
                  SendMessage(msgWrapper);
                }
                else if(currentlyObservedObject.id >= 0 && currentlyObservedObject.isActive)
                {
                  // Proof of concept: cycle colors
                  const s32 NUM_COLORS = 4;
                  const ColorRGBA colorList[NUM_COLORS] = {
                    NamedColors::RED, NamedColors::GREEN, NamedColors::BLUE,
                    NamedColors::BLACK
                  };

                  static s32 colorIndex = 0;

                  ExternalInterface::SetActiveObjectLEDs msg;
                  msg.objectID = currentlyObservedObject.id;
                  msg.robotID = 1;
                  msg.onPeriod_ms = 250;
                  msg.offPeriod_ms = 250;
                  msg.transitionOnPeriod_ms = 500;
                  msg.transitionOffPeriod_ms = 100;
                  msg.turnOffUnspecifiedLEDs = 1;
                  if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                    printf("Updating active block edge\n");
                    msg.onColor = NamedColors::RED;
                    msg.offColor = NamedColors::BLACK;
                    msg.whichLEDs = static_cast<u8>(WhichBlockLEDs::FRONT);
                    msg.makeRelative = 2;
                    msg.relativeToX = robotPosition_.x();
                    msg.relativeToY = robotPosition_.y();
                    
                  } else if( modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                    static s32 edgeIndex = 0;
                    
                    printf("Turning edge %d new color %d (%x)\n",
                           edgeIndex, colorIndex, u32(colorList[colorIndex]));
                    
                    msg.whichLEDs = (1 << edgeIndex);
                    msg.onColor = colorList[colorIndex];
                    msg.offColor = 0;
                    msg.turnOffUnspecifiedLEDs = 0;
                    msg.makeRelative = 2;
                    msg.relativeToX = robotPosition_.x();
                    msg.relativeToY = robotPosition_.y();
                    
                    ++edgeIndex;
                    if(edgeIndex == NUM_BLOCK_LEDS) {
                      edgeIndex = 0;
                      ++colorIndex;
                    }
                    
                  } else {
                    printf("Cycling active block %d color from (%d,%d,%d) to (%d,%d,%d)\n",
                           msg.objectID,
                           colorList[colorIndex==0 ? NUM_COLORS-1 : colorIndex-1].r(),
                           colorList[colorIndex==0 ? NUM_COLORS-1 : colorIndex-1].g(),
                           colorList[colorIndex==0 ? NUM_COLORS-1 : colorIndex-1].b(),
                           colorList[colorIndex].r(),
                           colorList[colorIndex].g(),
                           colorList[colorIndex].b());
                    msg.onColor = colorList[colorIndex++];
                    msg.offColor = NamedColors::BLACK;
                    msg.whichLEDs = static_cast<u8>(WhichBlockLEDs::FRONT);
                    msg.makeRelative = 0;
                    msg.turnOffUnspecifiedLEDs = 1;
                  }
                  
                  if(colorIndex == NUM_COLORS) {
                    colorIndex = 0;
                  }
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_SetActiveObjectLEDs(msg);
                  SendMessage(msgWrapper);
                }
                break;
              }
                
              case (s32)'O':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  ExternalInterface::FaceObject msg;
                  msg.robotID = 1;
                  msg.objectID = u32_MAX; // HACK to tell game to use blockworld's "selected" object
                  msg.turnAngleTol = DEG_TO_RAD(5);
                  msg.maxTurnAngle = DEG_TO_RAD(90);
                  msg.headTrackWhenDone = 0;
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_FaceObject(msg);
                  SendMessage(msgWrapper);
                } else if (modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  ExternalInterface::GotoObject msg;
                  msg.objectID = -1; // tell game to use blockworld's "selected" object
                  msg.distance_mm = sqrtf(2.f)*44.f;
                  msg.useManualSpeed = 0;
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_GotoObject(msg);
                  SendMessage(msgWrapper);
                } else {
                  SendIMURequest(2000);
                }
                break;
              }

              case (s32)'@':
              {
                //SendAnimation("ANIM_BACK_AND_FORTH_EXCITED", 3);
                SendAnimation("ANIM_TEST", 1);
                
                {
                  ExternalInterface::SetIdleAnimation msg;
                  msg.robotID = 1;
                  msg.animationName = "ANIM_IDLE";
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_SetIdleAnimation(msg);
                  SendMessage(msgWrapper);
                }
                
                break;
              }
              case (s32)'#':
              {
                ExternalInterface::QueueSingleAction msg1;
                msg1.robotID = 1;
                msg1.inSlot = 1;
                msg1.position = QueueActionPosition::NEXT;
                msg1.actionType = RobotActionType::PLAY_ANIMATION;
                msg1.action.playAnimation.animationName = "ANIM_TEST";
                msg1.action.playAnimation.numLoops = 1;
                
                ExternalInterface::QueueSingleAction msg2;
                msg2.robotID = 1;
                msg2.inSlot = 1;
                msg2.position = QueueActionPosition::NEXT;
                msg2.actionType = RobotActionType::PLAY_ANIMATION;
                msg2.action.playAnimation.animationName = "ANIM_TEST";
                msg2.action.playAnimation.numLoops = 1;
                
                ExternalInterface::MessageGameToEngine msgWrapper;
                msgWrapper.Set_QueueSingleAction(msg1);
                SendMessage(msgWrapper);
                
                msgWrapper.Set_QueueSingleAction(msg2);
                SendMessage(msgWrapper);
                
                //SendAnimation("ANIM_BLINK", 0);
                break;
              }
              case (s32)'$':
              {
                SendAnimation("ANIM_LIFT_NOD", 1);
                break;
              }
              case (s32)'%':
              {
                SendAnimation("ANIM_ALERT", 1);
                break;
              }
              case (s32)'^':
              {
                // Send whatever animation is specified in the animationToSendName field
                webots::Field* animToSendNameField = root_->getField("animationToSendName");
                if (animToSendNameField == nullptr) {
                  printf("ERROR: No animationToSendName field found in WebotsKeyboardController.proto\n");
                  break;
                }
                std::string animToSendName = animToSendNameField->getSFString();
                if (animToSendName.empty()) {
                  printf("ERROR: animationToSendName field is empty\n");
                  break;
                }
                
                webots::Field* animNumLoopsField = root_->getField("animationNumLoops");
                u32 animNumLoops = 1;
                if (animNumLoopsField && (animNumLoopsField->getSFInt32() > 0)) {
                  animNumLoops = animNumLoopsField->getSFInt32();
                }

                SendAnimation(animToSendName.c_str(), animNumLoops);
                break;
              }
              case (s32)'~':
              {
                //const s32 NUM_ANIM_TESTS = 4;
                //const AnimationID_t testAnims[NUM_ANIM_TESTS] = {ANIM_HEAD_NOD, ANIM_HEAD_NOD_SLOW, ANIM_BLINK, ANIM_UPDOWNLEFTRIGHT};
                
                const s32 NUM_ANIM_TESTS = 4;
                const char* testAnims[NUM_ANIM_TESTS] = {"ANIM_BLINK", "ANIM_HEAD_NOD", "ANIM_HEAD_NOD_SLOW", "ANIM_LIFT_NOD"};
                
                static s32 iAnimTest = 0;
                
                SendAnimation(testAnims[iAnimTest++], 1);
                
                if(iAnimTest == NUM_ANIM_TESTS) {
                  iAnimTest = 0;
                }
                break;
              }

              //
              // CREEP Behavior States:
              //
              case (s32)'0':
              {
                SendSetNextBehaviorState(BehaviorManager::DANCE_WITH_BLOCK);
                break;
              }
              case (s32)'7':
              {
                SendSetNextBehaviorState(BehaviorManager::EXCITABLE_CHASE);
                break;
              }
              case (s32)'8':
              {
                SendSetNextBehaviorState(BehaviorManager::SCARED_FLEE);
                break;
              }
              case (s32)'-':
              {
                SendSetNextBehaviorState(BehaviorManager::HELP_ME_STATE);
                break;
              }
              case (s32)'=':
              {
                SendSetNextBehaviorState(BehaviorManager::WHAT_NEXT);
                break;
              }
              case (s32)'9':
              {
                SendSetNextBehaviorState(BehaviorManager::SCAN);
                break;
              }
              case (s32)'&':
              {
                SendSetNextBehaviorState(BehaviorManager::IDLE);
                break;
              }
              case (s32)'!':
              {
                SendSetNextBehaviorState(BehaviorManager::ACKNOWLEDGEMENT_NOD);
                break;
              }
                
              case (s32)'/':
              {
                PrintHelp();
                break;
              }
                
              case (s32)'F':
              {
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  SendStopFaceTracking();
                } else {
                  SendStartFaceTracking(5);
                }
                break;
              }
              
              default:
              {
                // Unsupported key: ignore.
                break;
              }
                
            } // switch
          } // if/else testMode
          
        } // for(auto key : keysPressed_)
        
        movingWheels = throttleDir || steeringDir;

        if(movingWheels) {
          
          // Set wheel speeds based on drive commands
          if (throttleDir > 0) {
            leftSpeed = wheelSpeed + steeringDir * wheelSpeed * steeringCurvature;
            rightSpeed = wheelSpeed - steeringDir * wheelSpeed * steeringCurvature;
          } else if (throttleDir < 0) {
            leftSpeed = -wheelSpeed - steeringDir * wheelSpeed * steeringCurvature;
            rightSpeed = -wheelSpeed + steeringDir * wheelSpeed * steeringCurvature;
          } else {
            leftSpeed = steeringDir * wheelSpeed;
            rightSpeed = -steeringDir * wheelSpeed;
          }
          
          SendDriveWheels(leftSpeed, rightSpeed);
          wasMovingWheels_ = true;
        } else if(wasMovingWheels_ && !movingWheels) {
          // If we just stopped moving the wheels:
          SendDriveWheels(0, 0);
          wasMovingWheels_ = false;
        }
        
        // If the last key pressed was a move lift key then stop it.
        if(movingLift) {
          SendMoveLift(commandedLiftSpeed);
          wasMovingLift_ = true;
        } else if (wasMovingLift_ && !movingLift) {
          // If we just stopped moving the lift:
          SendMoveLift(0);
          wasMovingLift_ = false;
        }
        
        if(movingHead) {
          SendMoveHead(commandedHeadSpeed);
          wasMovingHead_ = true;
        } else if (wasMovingHead_ && !movingHead) {
          // If we just stopped moving the head:
          SendMoveHead(0);
          wasMovingHead_ = false;
        }
       
        
      } // BSKeyboardController::ProcessKeyStroke()
      
      void ProcessJoystick()
      {
#if(ENABLE_GAMEPAD_SUPPORT)
        //Handle events on queue
        while( SDL_PollEvent( &sdlEvent_ ) != 0 )
        {
          switch(sdlEvent_.type) {
              
            case SDL_CONTROLLERAXISMOTION:
              
              if (sdlEvent_.caxis.which == 0) {
              
                if (sdlEvent_.caxis.which >= GC_NUM_AXES) {
                  printf("WARN: Invalid GameController axis event detected\n");
                  break;
                }

                // Update value
                gc_axisValues_[sdlEvent_.caxis.axis] = sdlEvent_.caxis.value;

                // Send message depending on axis that was activated
                switch(sdlEvent_.caxis.axis)
                {
                  case GC_ANALOG_LEFT_X:
                  case GC_ANALOG_LEFT_Y:
                  {
                    #if(DEBUG_GAMEPAD)
                    printf("AnalogLeft X %d  Y %d\n", gc_axisValues_[GC_ANALOG_LEFT_X], gc_axisValues_[GC_ANALOG_LEFT_Y]);
                    #endif
                    
                    if (ABS(gc_axisValues_[GC_ANALOG_LEFT_X]) < ANALOG_INPUT_DEAD_ZONE_THRESH) {
                      gc_axisValues_[GC_ANALOG_LEFT_X] = 0;
                    }
                    if (ABS(gc_axisValues_[GC_ANALOG_LEFT_Y]) < ANALOG_INPUT_DEAD_ZONE_THRESH) {
                      gc_axisValues_[GC_ANALOG_LEFT_Y] = 0;
                    }
                    
                    // Compute speed
                    f32 xyMag = MIN(1.0,
                                    sqrtf( gc_axisValues_[GC_ANALOG_LEFT_X]*gc_axisValues_[GC_ANALOG_LEFT_X] + gc_axisValues_[GC_ANALOG_LEFT_Y]*gc_axisValues_[GC_ANALOG_LEFT_Y]) / (f32)s16_MAX
                                    );
                    
                    // Stop wheels if magnitude of input is low
                    if (xyMag < 0.01f) {
                      SendDriveWheels(0,0);
                      break;
                    }
                    
                    // Driving forward?
                    f32 fwd = gc_axisValues_[GC_ANALOG_LEFT_Y] < 0 ? 1 : -1;
                    
                    // Curving right?
                    f32 right = gc_axisValues_[GC_ANALOG_LEFT_X] > 0 ? 1 : -1;
                  
                    // Base wheel speed based on magnitude of input and whether or not robot is driving forward
                    f32 baseWheelSpeed = ANALOG_INPUT_MAX_DRIVE_SPEED * xyMag * fwd;
                  
            
                    // Angle of xy input used to determine curvature
                    f32 xyAngle = fabsf(atanf(-(f32)gc_axisValues_[GC_ANALOG_LEFT_Y] / (f32)gc_axisValues_[GC_ANALOG_LEFT_X])) * (right);
                    
                    // Compute radius of curvature
                    f32 roc = (xyAngle / PIDIV2_F) * MAX_ANALOG_RADIUS;
                    
                    
                    // Compute individual wheel speeds
                    f32 lwheel_speed_mmps = 0;
                    f32 rwheel_speed_mmps = 0;
                    if (fabsf(xyAngle) > PIDIV2_F - 0.1f) {
                      // Straight fwd/back
                      lwheel_speed_mmps = baseWheelSpeed;
                      rwheel_speed_mmps = baseWheelSpeed;
                    } else if (fabsf(xyAngle) < 0.1f) {
                      // Turn in place
                      lwheel_speed_mmps = right * xyMag * ANALOG_INPUT_MAX_DRIVE_SPEED;
                      rwheel_speed_mmps = -right * xyMag * ANALOG_INPUT_MAX_DRIVE_SPEED;
                    } else {
                      
                      lwheel_speed_mmps = baseWheelSpeed * (roc + (right * WHEEL_DIST_HALF_MM)) / roc;
                      rwheel_speed_mmps = baseWheelSpeed * (roc - (right * WHEEL_DIST_HALF_MM)) / roc;
                      
                      // Swap wheel speeds
                      if (fwd * right < 0) {
                        f32 temp = lwheel_speed_mmps;
                        lwheel_speed_mmps = rwheel_speed_mmps;
                        rwheel_speed_mmps = temp;
                      }
                    }
                    
                    
                    // Cap wheel speed at max
                    if (fabsf(lwheel_speed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
                      f32 correction = lwheel_speed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (lwheel_speed_mmps >= 0 ? 1 : -1));
                      f32 correctionFactor = 1.f - fabsf(correction / lwheel_speed_mmps);
                      lwheel_speed_mmps *= correctionFactor;
                      rwheel_speed_mmps *= correctionFactor;
                      //printf("lcorrectionFactor: %f\n", correctionFactor);
                    }
                    if (fabsf(rwheel_speed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
                      f32 correction = rwheel_speed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (rwheel_speed_mmps >= 0 ? 1 : -1));
                      f32 correctionFactor = 1.f - fabsf(correction / rwheel_speed_mmps);
                      lwheel_speed_mmps *= correctionFactor;
                      rwheel_speed_mmps *= correctionFactor;
                      //printf("rcorrectionFactor: %f\n", correctionFactor);
                    }
                    
                    #if(DEBUG_GAMEPAD)
                    printf("AnalogLeft: xyMag %f, xyAngle %f, radius %f, fwd %f, right %f, lwheel %f, rwheel %f\n", xyMag, xyAngle, roc, fwd, right, lwheel_speed_mmps, rwheel_speed_mmps );
                    #endif
                    
                    SendDriveWheels(lwheel_speed_mmps, rwheel_speed_mmps);
                    
                    break;
                  }
                  case GC_ANALOG_RIGHT_X:
                  case GC_ANALOG_RIGHT_Y:
                  {
                    #if(DEBUG_GAMEPAD)
                    printf("AnalogRight X %d  Y %d\n", gc_axisValues_[GC_ANALOG_RIGHT_X], gc_axisValues_[GC_ANALOG_RIGHT_Y]);
                    #endif
                    
                    // Compute head speed
                    f32 speed_rad_per_s = headSpeed * (-(f32)gc_axisValues_[GC_ANALOG_RIGHT_Y] / s16_MAX);
                    
                    SendMoveHead(speed_rad_per_s);
                    
                    break;
                  }
                  case GC_LT_BUTTON:
                  case GC_RT_BUTTON:
                    #if(DEBUG_GAMEPAD)
                    printf("Top buttons: LT %d  RT %d\n", gc_axisValues_[GC_LT_BUTTON], gc_axisValues_[GC_RT_BUTTON]);
                    #endif
                    break;
                  default:
                    printf("WARNING: Unrecognized axis %d\n", sdlEvent_.caxis.axis);
                    break;
                }
                
              } else {
                printf("WARNING: Unrecognized source of SDL event (%d)\n", sdlEvent_.caxis.which);
              }
              break;
              
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            {
              u8 buttonID = sdlEvent_.jbutton.button;
              gc_buttonPressed_[sdlEvent_.jbutton.button] = sdlEvent_.jbutton.state;
              
              bool pressed = sdlEvent_.jbutton.state; // If false, then this is a release event
              
              bool LT_held = gc_axisValues_[GC_LT_BUTTON];
              //bool RT_held = gc_axisValues_[GC_RT_BUTTON];
              
              // Process buttons that matter only when pressed
              if (pressed) {
                switch(buttonID) {
                  case GC_BUTTON_A:
                    SendSetNextBehaviorState(LT_held ? BehaviorManager::DANCE_WITH_BLOCK : BehaviorManager::EXCITABLE_CHASE);
                    break;
                  case GC_BUTTON_B:
                    SendSetNextBehaviorState(BehaviorManager::SCARED_FLEE);
                    break;
                  case GC_BUTTON_X:
                    SendSetNextBehaviorState(LT_held ? BehaviorManager::SCAN : BehaviorManager::IDLE);
                    break;
                  case GC_BUTTON_Y:
                    SendSetNextBehaviorState(LT_held ? BehaviorManager::WHAT_NEXT : BehaviorManager::HELP_ME_STATE);
                    break;
                  case GC_BUTTON_BACK:
                    break;
                    
                  case GC_BUTTON_START:
                    // Toggle CREEP mode
                    if(BehaviorManager::CREEP == behaviorMode_) {
                      behaviorMode_ = BehaviorManager::None;
                    } else {
                      behaviorMode_ = BehaviorManager::CREEP;
                    }
                    SendExecuteBehavior(behaviorMode_);
                    break;
                  default:
                    break;
                }
              }
              
              // Process buttons that matter both when pressed and released.
              switch(buttonID) {
                      
                case GC_BUTTON_ANALOG_LEFT:
                  break;
                case GC_BUTTON_ANALOG_RIGHT:
                  break;
                  
                case GC_BUTTON_LB:
                  if (LT_held) {
                    SendClearAllBlocks();
                  } else {
                    SendSelectNextObject();
                  }
                  break;
                case GC_BUTTON_RB:
                {
                  bool usePreDockPose = !LT_held;
                  SendPickAndPlaceSelectedObject(usePreDockPose, false);
                  break;
                }
                case GC_BUTTON_DIR_UP:
                case GC_BUTTON_DIR_DOWN:
                {
                  // Move lift up/down. Open-loop velocity commands.
                  f32 speed_rad_per_sec = 0;
                  if (pressed) {
                    speed_rad_per_sec = (LT_held ? 0.4f : 1) * (buttonID == GC_BUTTON_DIR_UP ? 1 : -1) * liftSpeed;
                  }
                  SendMoveLift(speed_rad_per_sec);
                  break;
                }
                case GC_BUTTON_DIR_LEFT:
                case GC_BUTTON_DIR_RIGHT:
                {
                  // Move lift to fixed position.
                  if (pressed) {
                    
                    // Figure out appropriate fixed position
                    f32 height_mm = LIFT_HEIGHT_LOWDOCK;
                    if (buttonID == GC_BUTTON_DIR_RIGHT) {
                      height_mm = LT_held ? LIFT_HEIGHT_HIGHDOCK : LIFT_HEIGHT_CARRY;
                    }
                    
                    SendMoveLiftToHeight(height_mm, liftSpeed, headAccel);
                  }
                  break;
                }
                default:
                  //printf("WARNING: Unrecognized button %d\n", sdlEvent_.jbutton.button);
                  break;
              }
              
              #if(DEBUG_GAMEPAD)
              printf("Button: %d  State %d, button %d, padding %d %d\n", sdlEvent_.jbutton.type, sdlEvent_.jbutton.state, sdlEvent_.jbutton.button, sdlEvent_.jbutton.padding1, sdlEvent_.jbutton.padding2);
              #endif
              break;
            }
            default:
              break;
          } // end switch
        } // end while
#endif
      }
      
      
      void TestLightCube()
      {
        static std::vector<ColorRGBA> colors = {{
          NamedColors::RED, NamedColors::GREEN, NamedColors::BLUE,
          NamedColors::CYAN, NamedColors::ORANGE, NamedColors::YELLOW
        }};
        static std::vector<WhichBlockLEDs> leds = {{
          WhichBlockLEDs::BACK,
          WhichBlockLEDs::LEFT,
          WhichBlockLEDs::FRONT,
          WhichBlockLEDs::RIGHT
        }};
        
        static auto colorIter = colors.begin();
        static auto ledIter = leds.begin();
        static s32 counter = 0;
        
        if(counter++ == 30) {
          counter = 0;
          
          ExternalInterface::SetActiveObjectLEDs msg;
          msg.objectID = currentlyObservedObject.id;
          msg.robotID = 1;
          msg.onPeriod_ms = 100;
          msg.offPeriod_ms = 100;
          msg.transitionOnPeriod_ms = 50;
          msg.transitionOffPeriod_ms = 50;
          msg.turnOffUnspecifiedLEDs = 1;
          msg.onColor = *colorIter;
          msg.offColor = 0;
          msg.whichLEDs = static_cast<u8>(*ledIter);
          msg.makeRelative = 0;
          
          ++ledIter;
          if(ledIter==leds.end()) {
            ledIter = leds.begin();
            ++colorIter;
            if(colorIter == colors.end()) {
              colorIter = colors.begin();
            }
          }
          
          ExternalInterface::MessageGameToEngine message;
          message.Set_SetActiveObjectLEDs(msg);
          SendMessage(message);
        }
      } // TestLightCube()
      
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
      void Update()
      {
        gameComms_->Update();
        
        switch(uiState_) {
          case UI_WAITING_FOR_GAME:
          {
            if (!gameComms_->HasClient()) {
              return;
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
            // Send ping to engine
            ExternalInterface::MessageGameToEngine message;
            message.Set_Ping(_pingMsg);
            msgHandler_.SendMessage(1, message); // TODO: don't hardcode ID here
            ++_pingMsg.counter;
            
            msgHandler_.ProcessMessages();
            
            // Update poseMarker pose
            const double* trans = gps_->getValues();
            const double* northVec = compass_->getValues();
            
            // Convert to mm
            Vec3f transVec;
            transVec.x() = trans[0] * 1000;
            transVec.y() = trans[1] * 1000;
            transVec.z() = trans[2] * 1000;
            
            // Compute orientation from north vector
            f32 angle = atan2f(-northVec[1], northVec[0]);
            
            poseMarkerPose_.SetTranslation(transVec);
            poseMarkerPose_.SetRotation(angle, Z_AXIS_3D());
            
            // Update pose marker if different from last time
            if (!(prevPoseMarkerPose_ == poseMarkerPose_)) {
              if (poseMarkerMode_ != 0) {
                // Place object mode
                SendDrawPoseMarker(poseMarkerPose_);
              }
            }
            
            ProcessKeystroke();
            ProcessJoystick();
            
            /*
             // DEBUG!!!!!
             ExternalInterface::SetRobotCarryingObject m;
             m.objectID = 500;
             m.robotID = 1;
             message.Set_SetRobotCarryingObject(m);
             SendMessage(message);
             */
        
            
            //TestLightCube();
            
            prevPoseMarkerPose_ = poseMarkerPose_;
            break;
          }
            
          default:
            PRINT_NAMED_ERROR("KeyboardController.Update", "Reached default switch case.\n");
            
        } // switch(uiState_)
        
      }
      
      void SendMessage(const ExternalInterface::MessageGameToEngine& msg)
      {
        UserDeviceID_t devID = 1; // TODO: Should this be a RobotID_t?
        msgHandler_.SendMessage(devID, msg); 
      }
      
      void SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps)
      {
        ExternalInterface::DriveWheels m;
        m.lwheel_speed_mmps = lwheel_speed_mmps;
        m.rwheel_speed_mmps = rwheel_speed_mmps;
        ExternalInterface::MessageGameToEngine message;
        message.Set_DriveWheels(m);
        SendMessage(message);
      }
      
      void SendTurnInPlace(const f32 angle_rad)
      {
        ExternalInterface::TurnInPlace m;
        m.robotID = 1;
        m.angle_rad = angle_rad;
        ExternalInterface::MessageGameToEngine message;
        message.Set_TurnInPlace(m);
        SendMessage(message);
      }

      void SendTurnInPlaceAtSpeed(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2)
      {
        ExternalInterface::TurnInPlaceAtSpeed m;
        m.robotID = 1;
        m.speed_rad_per_sec = speed_rad_per_sec;
        m.accel_rad_per_sec2 = accel_rad_per_sec2;
        ExternalInterface::MessageGameToEngine message;
        message.Set_TurnInPlaceAtSpeed(m);
        SendMessage(message);
      }
      
      void SendMoveHead(const f32 speed_rad_per_sec)
      {
        ExternalInterface::MoveHead m;
        m.speed_rad_per_sec = speed_rad_per_sec;
        ExternalInterface::MessageGameToEngine message;
        message.Set_MoveHead(m);
        SendMessage(message);
      }
      
      void SendMoveLift(const f32 speed_rad_per_sec)
      {
        ExternalInterface::MoveLift m;
        m.speed_rad_per_sec = speed_rad_per_sec;
        ExternalInterface::MessageGameToEngine message;
        message.Set_MoveLift(m);
        SendMessage(message);
      }
      
      void SendMoveHeadToAngle(const f32 rad, const f32 speed, const f32 accel, const f32 duration_sec)
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
      
      void SendMoveLiftToHeight(const f32 mm, const f32 speed, const f32 accel, const f32 duration_sec)
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
      
      void SendTapBlockOnGround(const u8 numTaps)
      {
        ExternalInterface::TapBlockOnGround m;
        m.numTaps = numTaps;
        ExternalInterface::MessageGameToEngine message;
        message.Set_TapBlockOnGround(m);
        SendMessage(message);
      }
      
      void SendStopAllMotors()
      {
        ExternalInterface::StopAllMotors m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_StopAllMotors(m);
        SendMessage(message);
      }
      
      void SendImageRequest(u8 mode, u8 robotID)
      {
        ExternalInterface::ImageRequest m;
        m.robotID = robotID;
        m.mode = mode;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ImageRequest(m);
        SendMessage(message);
      }
      
      void SendSetRobotImageSendMode(u8 mode)
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
      
      void SendSaveImages(SaveMode_t mode, bool alsoSaveState)
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
      
      void SendEnableDisplay(bool on)
      {
        ExternalInterface::EnableDisplay m;
        m.enable = on;
        ExternalInterface::MessageGameToEngine message;
        message.Set_EnableDisplay(m);
        SendMessage(message);
     }
      
      void SendSetHeadlights(u8 intensity)
      {
        ExternalInterface::SetHeadlights m;
        m.intensity = intensity;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetHeadlights(m);
        SendMessage(message);
      }
      
      void SendExecutePathToPose(const Pose3d& p, const bool useManualSpeed)
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
      
      void SendPlaceObjectOnGroundSequence(const Pose3d& p, const bool useManualSpeed)
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
      
      void SendExecuteTestPlan()
      {
        ExternalInterface::ExecuteTestPlan m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ExecuteTestPlan(m);
        SendMessage(message);
      }
      
      void SendClearAllBlocks()
      {
        ExternalInterface::ClearAllBlocks m;
        m.robotID = 1;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ClearAllBlocks(m);
        SendMessage(message);
      }
      
      void SendClearAllObjects()
      {
        ExternalInterface::ClearAllObjects m;
        m.robotID = 1;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ClearAllObjects(m);
        SendMessage(message);
      }
      
      void SendSelectNextObject()
      {
        ExternalInterface::SelectNextObject m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SelectNextObject(m);
        SendMessage(message);
      }
      
      void SendPickAndPlaceObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed)
      {
        ExternalInterface::PickAndPlaceObject m;
        m.usePreDockPose = static_cast<u8>(usePreDockPose);
        m.useManualSpeed = static_cast<u8>(useManualSpeed);
        m.objectID = -1;
        ExternalInterface::MessageGameToEngine message;
        message.Set_PickAndPlaceObject(m);
        SendMessage(message);
      }
      
      void SendPickAndPlaceSelectedObject(const bool usePreDockPose, const bool useManualSpeed)
      {
        SendPickAndPlaceObject(-1, usePreDockPose, useManualSpeed);
      }

      void SendRollObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed)
      {
        ExternalInterface::RollObject m;
        m.usePreDockPose = static_cast<u8>(usePreDockPose);
        m.useManualSpeed = static_cast<u8>(useManualSpeed);
        m.objectID = -1;
        ExternalInterface::MessageGameToEngine message;
        message.Set_RollObject(m);
        SendMessage(message);
      }
      
      void SendRollSelectedObject(const bool usePreDockPose, const bool useManualSpeed)
      {
        SendRollObject(-1, usePreDockPose, useManualSpeed);
      }

      
      void SendTraverseSelectedObject(const bool usePreDockPose, const bool useManualSpeed)
      {
        ExternalInterface::TraverseObject m;
        m.usePreDockPose = static_cast<u8>(usePreDockPose);
        m.useManualSpeed = static_cast<u8>(useManualSpeed);
        ExternalInterface::MessageGameToEngine message;
        message.Set_TraverseObject(m);
        SendMessage(message);
      }
      
      void SendExecuteBehavior(BehaviorManager::Mode mode)
      {
        ExternalInterface::ExecuteBehavior m;
        m.behaviorMode = mode;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ExecuteBehavior(m);
        SendMessage(message);
      }
      
      void SendSetNextBehaviorState(BehaviorManager::BehaviorState nextState)
      {
        ExternalInterface::SetBehaviorState m;
        m.behaviorState = nextState;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetBehaviorState(m);
        SendMessage(message);
      }
      
      void SendAbortPath()
      {
        ExternalInterface::AbortPath m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_AbortPath(m);
        SendMessage(message);
      }
      
      void SendAbortAll()
      {
        ExternalInterface::AbortAll m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_AbortAll(m);
        SendMessage(message);
      }
      
      void SendDrawPoseMarker(const Pose3d& p)
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
      
      void SendErasePoseMarker()
      {
        ExternalInterface::ErasePoseMarker m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ErasePoseMarker(m);
        SendMessage(message);
      }

      void SendWheelControllerGains(const f32 kpLeft, const f32 kiLeft, const f32 maxErrorSumLeft,
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

      
      void SendHeadControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum)
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
      
      void SendLiftControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum)
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
      
      void SendSteeringControllerGains(const f32 k1, const f32 k2)
      {
        ExternalInterface::SetSteeringControllerGains m;
        m.k1 = k1;
        m.k2 = k2;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetSteeringControllerGains(m);
        SendMessage(message);
      }
      
      void SendSetRobotVolume(const f32 volume)
      {
        ExternalInterface::SetRobotVolume m;
        m.volume = volume;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetRobotVolume(m);
        SendMessage(message);
      }
      
      void SendStartTestMode(TestMode mode, s32 p1, s32 p2, s32 p3)
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
      
      void SendIMURequest(u32 length_ms)
      {
        ExternalInterface::IMURequest m;
        m.length_ms = length_ms;
        ExternalInterface::MessageGameToEngine message;
        message.Set_IMURequest(m);
        SendMessage(message);
      }
      
      void SendAnimation(const char* animName, u32 numLoops)
      {
        static bool lastSendTime_sec = -1e6;
        
        // Don't send repeated animation commands within a half second
        if(inputController.getTime() > lastSendTime_sec + 0.5f)
        {
          ExternalInterface::PlayAnimation m;
          //m.animationID = animId;
          m.robotID = 1;
          m.animationName = animName;
          m.numLoops = numLoops;
          ExternalInterface::MessageGameToEngine message;
          message.Set_PlayAnimation(m);
          SendMessage(message);
          lastSendTime_sec = inputController.getTime();
        } else {
          printf("Ignoring duplicate SendAnimation keystroke.\n");
        }
        
      }

      void SendReplayLastAnimation()
      {
        ExternalInterface::ReplayLastAnimation m;
        m.numLoops = 1;
        m.robotID = 1;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ReplayLastAnimation(m);
        SendMessage(message);
      }

      void SendReadAnimationFile()
      {
        ExternalInterface::ReadAnimationFile m;
        ExternalInterface::MessageGameToEngine message;
        message.Set_ReadAnimationFile(m);
        SendMessage(message);
      }
      
      void SendStartFaceTracking(u8 timeout_sec)
      {
        ExternalInterface::StartFaceTracking m;
        m.timeout_sec = timeout_sec;
        ExternalInterface::MessageGameToEngine message;
        message.Set_StartFaceTracking(m);
        SendMessage(message);
      }
      
      void SendStopFaceTracking()
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

      void SendVisionSystemParams()
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

      void SendFaceDetectParams()
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
      
      void SendForceAddRobot()
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
      
      
    } // namespace WebotsKeyboardController
  } // namespace Cozmo
} // namespace Anki


// =======================================================================

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  WebotsKeyboardController::inputController.step(KB_TIME_STEP);
  
  WebotsKeyboardController::Init();

  while (WebotsKeyboardController::inputController.step(KB_TIME_STEP) != -1)
  {
    // Process keyboard input
    WebotsKeyboardController::Update();
  }
  
  return 0;
}

