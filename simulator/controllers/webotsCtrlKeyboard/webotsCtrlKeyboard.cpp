/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "webotsCtrlKeyboard.h"

#include <opencv2/imgproc/imgproc.hpp>
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/vision/basestation/image.h"
#include "anki/cozmo/basestation/imageDeChunker.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/demoBehaviorChooser.h"
#include "anki/cozmo/basestation/block.h"
#include "clad/types/actionTypes.h"
#include "clad/types/proceduralEyeParameters.h"
#include "clad/types/ledTypes.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/types/demoBehaviorState.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
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
      
      
      // Private members:
      namespace {

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
        
        
        
        // For displaying cozmo's POV:
        webots::Display* cozmoCam_;
        webots::ImageRef* img_ = nullptr;
        
        ImageDeChunker _imageDeChunker;
        
        // Save robot image to file
        bool saveRobotImageToFile_ = false;
        ExternalInterface::RobotObservedFace _lastFace;
        
      } // private namespace
    
      // ======== Message handler callbacks =======
    
    // For processing image chunks arriving from robot.
    // Sends complete images to VizManager for visualization (and possible saving).
    void WebotsKeyboardController::HandleImageChunk(ImageChunk const& msg)
    {
      const u16 width  = Vision::CameraResInfo[(int)msg.resolution].width;
      const u16 height = Vision::CameraResInfo[(int)msg.resolution].height;
      const bool isImageReady = _imageDeChunker.AppendChunk(msg.imageId, msg.frameTimeStamp, height, width,
        msg.imageEncoding, msg.imageChunkCount, msg.chunkId, msg.data.data(), (uint32_t)msg.data.size());
      
      
      if(isImageReady)
      {
        cv::Mat img = _imageDeChunker.GetImage();
        if(img.channels() == 1) {
          cvtColor(img, img, CV_GRAY2RGB);
        }
        
        const s32 outputColor = 1; // 1 for Green, 2 for Blue
        
        for(s32 i=0; i<img.rows; ++i) {
          
          if(i % 2 == 0) {
            cv::Mat img_i = img.row(i);
            img_i.setTo(0);
          } else {
            u8* img_i = img.ptr(i);
            for(s32 j=0; j<img.cols; ++j) {
              img_i[3*j+outputColor] = std::max(std::max(img_i[3*j], img_i[3*j + 1]), img_i[3*j + 2]);
              
              img_i[3*j+(3-outputColor)] /= 2;
              img_i[3*j] = 0; // kill red channel
              
              // [Optional] Add a bit of noise
              f32 noise = 20.f*static_cast<f32>(std::rand()) / static_cast<f32>(RAND_MAX) - 0.5f;
              img_i[3*j+outputColor] = static_cast<u8>(std::max(0.f,std::min(255.f,static_cast<f32>(img_i[3*j+outputColor]) + noise)));
              
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
    
    
    void WebotsKeyboardController::HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg)
    {
      if(cozmoCam_ == nullptr) {
        printf("RECEIVED OBJECT OBSERVED: objectID %d\n", msg.objectID);
      } else {
        // Draw a rectangle in red with the object ID as text in the center
        cozmoCam_->setColor(0x000000);
        
        //std::string dispStr(ObjectType::GetName(msg.objectType));
        //dispStr += " ";
        //dispStr += std::to_string(msg.objectID);
        std::string dispStr("Type=" + std::string(ObjectTypeToString(msg.objectType)) + "\nID=" + std::to_string(msg.objectID));
        cozmoCam_->drawText(dispStr,
                            msg.img_topLeft_x + msg.img_width/4 + 1,
                            msg.img_topLeft_y + msg.img_height/2 + 1);
        
        cozmoCam_->setColor(0xff0000);
        cozmoCam_->drawRectangle(msg.img_topLeft_x, msg.img_topLeft_y,
                                 msg.img_width, msg.img_height);
        cozmoCam_->drawText(dispStr,
                            msg.img_topLeft_x + msg.img_width/4,
                            msg.img_topLeft_y + msg.img_height/2);
        
      }
      
    }
    
    void WebotsKeyboardController::HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg)
    {
      //printf("RECEIVED FACE OBSERVED: faceID %llu\n", msg.faceID);
      _lastFace = msg;
    }

    // ============== End of message handlers =================
    
    
    void WebotsKeyboardController::InitInternal()
      { 
        // Make root point to WebotsKeyBoardController node
        root_ = GetSupervisor()->getSelf();
        
        GetSupervisor()->keyboardEnable(GetStepTimeMS());
        
        gps_ = GetSupervisor()->getGPS("gps");
        compass_ = GetSupervisor()->getCompass("compass");
        
        gps_->enable(BS_TIME_STEP);
        compass_->enable(BS_TIME_STEP);
        
        poseMarkerDiffuseColor_ = root_->getField("poseMarkerDiffuseColor");
        
        cozmoCam_ = GetSupervisor()->getDisplay("uiCamDisplay");
        
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

      }
    
    
    WebotsKeyboardController::WebotsKeyboardController(s32 step_time_ms) :
    UiGameController(step_time_ms)
    {
      
    }
      
      void WebotsKeyboardController::PrintHelp()
      {
        printf("\nBasestation keyboard control\n");
        printf("===============================\n");
        printf("                           Drive:  arrows  (Hold shift for slower speeds)\n");
        printf("               Move lift up/down:  a/z\n");
        printf("               Move head up/down:  s/x\n");
        printf("             Lift low/high/carry:  1/2/3\n");
        printf("            Head down/forward/up:  4/5/6\n");
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
        printf("              Abort current path:  q\n");
        printf("                Abort everything:  Shift+q\n");
        printf("         Update controller gains:  k\n");
        printf("             Cycle sound schemes:  m\n");
        printf("                 Request IMU log:  o\n");
        printf("            Toggle face tracking:  f (Shift+f)\n");
        printf("                      Test modes:  Alt + Testmode#\n");
        printf("                Follow test plan:  t\n");
        printf("        Force-add specifed robot:  Shift+r\n");
        printf("           Set DemoState Default:  j\n");
        printf("         Set DemoState FacesOnly:  Shift+j\n");
        printf("        Set DemoState BlocksOnly:  Alt+j\n");
        printf("      Play 'animationToSendName':  Shift+6\n");
        printf("  Set idle to'idleAnimationName':  Shift+Alt+6\n");
        printf("     Update Viz origin alignment:  ` <backtick>\n");
        printf("                      Print help:  ?\n");
        printf("\n");
      }
      
      //Check the keyboard keys and issue robot commands
      void WebotsKeyboardController::ProcessKeystroke()
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
        
        root_ = GetSupervisor()->getSelf();
        f32 wheelSpeed = root_->getField("driveSpeedNormal")->getSFFloat();
        
        f32 steeringCurvature = root_->getField("steeringCurvature")->getSFFloat();
        
        static bool keyboardRestart = false;
        if (keyboardRestart) {
          GetSupervisor()->keyboardDisable();
          GetSupervisor()->keyboardEnable(BS_TIME_STEP);
          keyboardRestart = false;
        }
        
        // Get all keys pressed this tic
        std::set<int> keysPressed;
        int key;
        while((key = GetSupervisor()->keyboardGetKey()) != 0) {
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
          
          lastKeyPressTime_ = GetSupervisor()->getTime();
          
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
                case TestMode::TM_DIRECT_DRIVE:
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
                case TestMode::TM_LIFT:
                  p1 = root_->getField("liftTest_flags")->getSFInt32();
                  p2 = root_->getField("liftTest_nodCycleTimeMS")->getSFInt32();  // Nodding cycle time in ms (if LiftTF_NODDING flag is set)
                  p3 = 250;
                  break;
                case TestMode::TM_HEAD:
                  p1 = root_->getField("headTest_flags")->getSFInt32();
                  p2 = root_->getField("headTest_nodCycleTimeMS")->getSFInt32();  // Nodding cycle time in ms (if HTF_NODDING flag is set)
                  p3 = 250;
                  break;
                case TestMode::TM_PLACE_BLOCK_ON_GROUND:
                  p1 = 100;  // x_offset_mm
                  p2 = -10;  // y_offset_mm
                  p3 = 0;    // angle_offset_degrees
                  break;
                case TestMode::TM_LIGHTS:
                  // p1: flags (See LightTestFlags)
                  // p2: The LED channel to activate (applies if LTF_CYCLE_ALL not enabled)
                  // p3: The color to set it to (applies if LTF_CYCLE_ALL not enabled)
                  p1 = (s32)LightTestFlags::LTF_CYCLE_ALL;
                  p2 = (s32)LEDId::LED_BACKPACK_RIGHT;
                  p3 = (s32)LEDColor::LED_GREEN;
                  break;
                default:
                  break;
              }
              
              printf("Sending test mode %s\n", TestModeToString(m));
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
                ImageSendMode mode = ImageSendMode::SingleShot;
                
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  // CTRL/CMD+SHIFT+I - Toggle physical robot image streaming
                  static bool streamOn = false;
                  if (streamOn) {
                    mode = ImageSendMode::Off;
                    printf("Turning robot image streaming OFF.\n");
                  } else {
                    mode = ImageSendMode::Stream;
                    printf("Turning robot image streaming ON.\n");
                  }
                  streamOn = !streamOn;
                } else {
                  printf("Requesting single robot image.\n");
                }
                

                // Determine resolution from "streamResolution" setting in the keyboard controller
                // node
                ImageResolution resolution = (ImageResolution)IMG_STREAM_RES;
               
                if (root_) {
                  const std::string resString = root_->getField("streamResolution")->getSFString();
                  printf("Attempting to switch robot to %s resolution.\n", resString.c_str());
                  if(resString == "VGA") {
                    resolution = ImageResolution::VGA;
                  } else if(resString == "QVGA") {
                    resolution = ImageResolution::QVGA;
                  } else if(resString == "CVGA") {
                    resolution = ImageResolution::CVGA;
                  } else {
                    printf("Unsupported streamResolution = %s\n", resString.c_str());
                  }
                }
                
                SendSetRobotImageSendMode(mode, resolution);

                break;
              }
                
              case (s32)'U':
              {
                // TODO: How to choose which robot
                const RobotID_t robotID = 1;
                
                // U - Request a single image from the game for a specified robot
                ImageSendMode mode = ImageSendMode::SingleShot;
                
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  // SHIFT+I - Toggle image streaming from the game
                  static bool streamOn = true;
                  if (streamOn) {
                    mode = ImageSendMode::Off;
                    printf("Turning game image streaming OFF.\n");
                  } else {
                    mode = ImageSendMode::Stream;
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
                for(s32 i=0; i<(s32)LEDId::NUM_BACKPACK_LEDS; ++i)
                {
                  msg.onColor[i] = 0;
                  msg.offColor[i] = 0;
                  msg.onPeriod_ms[i] = 1000;
                  msg.offPeriod_ms[i] = 2000;
                  msg.transitionOnPeriod_ms[i] = 500;
                  msg.transitionOffPeriod_ms[i] = 500;
                }
                
                if(!backpackLightsOn) {
                  msg.onColor[(uint32_t)LEDId::LED_BACKPACK_RIGHT]  = NamedColors::GREEN;
                  msg.onColor[(uint32_t)LEDId::LED_BACKPACK_LEFT]   = NamedColors::RED;
                  msg.onColor[(uint32_t)LEDId::LED_BACKPACK_BACK]   = NamedColors::BLUE;
                  msg.onColor[(uint32_t)LEDId::LED_BACKPACK_MIDDLE] = NamedColors::CYAN;
                  msg.onColor[(uint32_t)LEDId::LED_BACKPACK_FRONT]  = NamedColors::YELLOW;
                }
                
                ExternalInterface::MessageGameToEngine msgWrapper;
                msgWrapper.Set_SetBackpackLEDs(msg);
                SendMessage(msgWrapper);

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
                           GetCurrentlyObservedObject().id);
                    
                    m.objectID = GetCurrentlyObservedObject().id;
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
               
                  // Send whatever animation is specified in the animationToSendName field
                  webots::Field* behaviorNameField = root_->getField("behaviorName");
                  if (behaviorNameField == nullptr) {
                    printf("ERROR: No behaviorName field found in WebotsKeyboardController.proto\n");
                    break;
                  }
                  std::string behaviorName = behaviorNameField->getSFString();
                  if (behaviorName.empty()) {
                    printf("ERROR: animationToSendName field is empty\n");
                    break;
                  }
                  
                  SendExecuteBehavior(behaviorName);
                  
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
                //bool useManualSpeed = (modifier_key & webots::Supervisor::KEYBOARD_ALT);
                
                // Hijacking ALT key for low placement
                bool useManualSpeed = false;
                bool placeOnGroundAtOffset = (modifier_key & webots::Supervisor::KEYBOARD_ALT);

                f32 placementOffsetX_mm = 0;
                if (placeOnGroundAtOffset) {
                  placementOffsetX_mm = root_->getField("placeOnGroundOffsetX_mm")->getSFFloat();
                }
                
                SendPickAndPlaceSelectedObject(usePreDockPose,
                                               placementOffsetX_mm, 0, 0,
                                               placeOnGroundAtOffset,
                                               useManualSpeed);
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
                  SendCancelAction();
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
                  msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                  msg.objectID = jsonMsg["objectID"].asUInt();
                  for(s32 iLED = 0; iLED<(s32)ActiveObjectConstants::NUM_CUBE_LEDS; ++iLED) {
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
                else if(GetCurrentlyObservedObject().id >= 0 && GetCurrentlyObservedObject().isActive)
                {
                  // Proof of concept: cycle colors
                  const s32 NUM_COLORS = 4;
                  const ColorRGBA colorList[NUM_COLORS] = {
                    NamedColors::RED, NamedColors::GREEN, NamedColors::BLUE,
                    NamedColors::BLACK
                  };

                  static s32 colorIndex = 0;

                  ExternalInterface::SetActiveObjectLEDs msg;
                  msg.objectID = GetCurrentlyObservedObject().id;
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
                    msg.whichLEDs = static_cast<u8>(WhichCubeLEDs::FRONT);
                    msg.makeRelative = 2;
                    msg.relativeToX = GetRobotPose().GetTranslation().x();
                    msg.relativeToY = GetRobotPose().GetTranslation().y();
                    
                  } else if( modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                    static s32 edgeIndex = 0;
                    
                    printf("Turning edge %d new color %d (%x)\n",
                           edgeIndex, colorIndex, u32(colorList[colorIndex]));
                    
                    msg.whichLEDs = (1 << edgeIndex);
                    msg.onColor = colorList[colorIndex];
                    msg.offColor = 0;
                    msg.turnOffUnspecifiedLEDs = 0;
                    msg.makeRelative = 2;
                    msg.relativeToX = GetRobotPose().GetTranslation().x();
                    msg.relativeToY = GetRobotPose().GetTranslation().y();
                    
                    ++edgeIndex;
                    if(edgeIndex == (s32)ActiveObjectConstants::NUM_CUBE_LEDS) {
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
                    msg.whichLEDs = static_cast<u8>(WhichCubeLEDs::FRONT);
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
                
              case (s32)'`':
              {
                printf("Updating viz origin\n");
                UpdateVizOrigin();
                break;
              }

              case (s32)'@':
              {
                //SendAnimation("ANIM_BACK_AND_FORTH_EXCITED", 3);
                SendAnimation("ANIM_TEST", 1);
                SendSetIdleAnimation("ANIM_IDLE");
                
                break;
              }
              case (s32)'#':
              {
                SendQueuePlayAnimAction("ANIM_TEST", 1, QueueActionPosition::NEXT);
                SendQueuePlayAnimAction("ANIM_TEST", 1, QueueActionPosition::NEXT);
                
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
              case (s32)'*':
              {
                /*
                // Send a procedural face
                using namespace ExternalInterface;
                using Param = ProceduralEyeParameter;
                DisplayProceduralFace msg;
                msg.robotID = 1;
                msg.leftEye.resize(static_cast<size_t>(Param::NumParameters));
                msg.rightEye.resize(static_cast<size_t>(Param::NumParameters));
                auto SetHelper = [&msg](Param param) {
                  msg.leftEye[static_cast<size_t>(param)] = 2.f*static_cast<f32>(rand())/static_cast<f32>(RAND_MAX)-1.f;
                  msg.rightEye[static_cast<size_t>(param)] = 2.f*static_cast<f32>(rand())/static_cast<f32>(RAND_MAX)-1.f;
                };
                
                SetHelper(Param::EyeWidth);
                SetHelper(Param::EyeHeight);
                SetHelper(Param::PupilWidth);
                SetHelper(Param::PupilHeight);
                SetHelper(Param::BrowAngle);
                SetHelper(Param::PupilCenX);
                SetHelper(Param::PupilCenY);
                
                msg.faceAngle = 2.f*static_cast<f32>(rand())/static_cast<f32>(RAND_MAX) - 1.f;

                SendMessage(MessageGameToEngine(std::move(msg)));
                */

                break;
              }
              case (s32)'^':
              {
                if(modifier_key & webots::Supervisor::KEYBOARD_ALT)
                {
                  webots::Field* idleAnimToSendField = root_->getField("idleAnimationName");
                  if(idleAnimToSendField == nullptr) {
                    printf("ERROR: No idleAnimationName field found in WebotsKeyboardController.proto\n");
                    break;
                  }
                  std::string idleAnimToSendName = idleAnimToSendField->getSFString();
                  
                  SendSetIdleAnimation(idleAnimToSendName);
                }
                else {
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
                }
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
                
              case (s32)'/':
              {
                PrintHelp();
                break;
              }
                
              case (s32)'F':
              {
                if (modifier_key & webots::Supervisor::KEYBOARD_SHIFT) {
                  SendStopFaceTracking();
                } else if(modifier_key & webots::Supervisor::KEYBOARD_ALT) {
                  // Turn to face the pose of the last observed face:
                  printf("Turning to face ID = %llu\n", _lastFace.faceID);
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::FacePose(_lastFace.world_x, _lastFace.world_y, _lastFace.world_z, DEG_TO_RAD(10), M_PI, 1)));
                } else {
                  SendStartFaceTracking(5);
                }
                break;
              }
                
              case (s32)'J':
              {
                if (webots::Supervisor::KEYBOARD_SHIFT == modifier_key)
                {
                  // Send DemoState FacesOnly
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetDemoState(DemoBehaviorState::FacesOnly)));
                }
                else if (webots::Supervisor::KEYBOARD_ALT == modifier_key)
                {
                  // Send DemoState BlocksOnly
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetDemoState(DemoBehaviorState::BlocksOnly)));
                }
                else
                {
                  // Send DemoState Default
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetDemoState(DemoBehaviorState::Default)));
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
      
      void WebotsKeyboardController::ProcessJoystick()
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
                  SendPickAndPlaceSelectedObject(usePreDockPose);
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
      
      
    void WebotsKeyboardController::TestLightCube()
      {
        static std::vector<ColorRGBA> colors = {{
          NamedColors::RED, NamedColors::GREEN, NamedColors::BLUE,
          NamedColors::CYAN, NamedColors::ORANGE, NamedColors::YELLOW
        }};
        static std::vector<WhichCubeLEDs> leds = {{
          WhichCubeLEDs::BACK,
          WhichCubeLEDs::LEFT,
          WhichCubeLEDs::FRONT,
          WhichCubeLEDs::RIGHT
        }};
        
        static auto colorIter = colors.begin();
        static auto ledIter = leds.begin();
        static s32 counter = 0;
        
        if(counter++ == 30) {
          counter = 0;
          
          ExternalInterface::SetActiveObjectLEDs msg;
          msg.objectID = GetCurrentlyObservedObject().id;
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
    
            
      s32 WebotsKeyboardController::UpdateInternal()
      {
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
        
        return 0;
      }
    
      

  } // namespace Cozmo
} // namespace Anki


// =======================================================================

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  Anki::Cozmo::WebotsKeyboardController webotsCtrlKeyboard(BS_TIME_STEP);
  
  webotsCtrlKeyboard.Init();
  while (webotsCtrlKeyboard.Update() == 0)
  {
  }

  return 0;
}
