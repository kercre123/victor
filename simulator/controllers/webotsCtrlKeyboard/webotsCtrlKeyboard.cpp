/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "webotsCtrlKeyboard.h"

#include "../shared/ctrlCommonInitialization.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserTypesHelpers.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/encodedImage.h"
#include "anki/cozmo/basestation/moodSystem/emotionTypesHelpers.h"
#include "anki/cozmo/basestation/factory/factoryTestLogger.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/image_impl.h"
#include "clad/types/actionTypes.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/types/behaviorChooserType.h"
#include "clad/types/behaviorTypes.h"
#include "clad/types/ledTypes.h"
#include "clad/types/proceduralEyeParameters.h"
#include "util/math/math.h"
#include "util/logging/channelFilter.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/fileUtils/fileUtils.h"
#include <fstream>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <string.h>
#include <webots/Compass.hpp>
#include <webots/Display.hpp>
#include <webots/GPS.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Keyboard.hpp>

#define LOG_CHANNEL "Keyboard"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

// CAUTION: If enabled, you can mess up stuff stored on the robot's flash.
#define ENABLE_NVSTORAGE_WRITE 0

namespace Anki {
  namespace Cozmo {
      
      
      // Private members:
      namespace {

        std::set<int> lastKeysPressed_;
        
        bool wasMovingWheels_ = false;
        bool wasMovingHead_   = false;
        bool wasMovingLift_   = false;
        
        webots::Node* root_ = nullptr;
        
        u8 poseMarkerMode_ = 0;
        Anki::Pose3d prevPoseMarkerPose_;
        Anki::Pose3d poseMarkerPose_;
        webots::Field* poseMarkerDiffuseColor_ = nullptr;
        double poseMarkerColor_[2][3] = { {0.1, 0.8, 0.1} // Goto pose color
          ,{0.8, 0.1, 0.1} // Place object color
        };
        
        double lastKeyPressTime_;
        
        PathMotionProfile pathMotionProfile_ = PathMotionProfile();
        
        // For displaying cozmo's POV:
        webots::Display* cozmoCam_;
        webots::ImageRef* img_ = nullptr;
        
        EncodedImage _encodedImage;
        
        // Save robot image to file
        bool saveRobotImageToFile_ = false;
        
        std::string _drivingStartAnim = "";
        std::string _drivingLoopAnim = "";
        std::string _drivingEndAnim = "";

        // For exporting formatted log of mfg test data from robot
        FactoryTestLogger _factoryTestLogger;
        
        struct ObservedImageCentroid {
          Point2f     point;
          TimeStamp_t timestamp;
          
          template<class MsgType>
          void SetFromMessage(const MsgType& msg)
          {
            point.x() = msg.img_rect.x_topLeft + msg.img_rect.width*0.5f;
            point.y() = msg.img_rect.y_topLeft + msg.img_rect.height*0.5f;
            timestamp = msg.timestamp;
          }
          
        } _lastObservedImageCentroid;
        
      } // private namespace
    
      // ======== Message handler callbacks =======
    
    // For processing image chunks arriving from robot.
    // Sends complete images to VizManager for visualization (and possible saving).
    void WebotsKeyboardController::HandleImageChunk(ImageChunk const& msg)
    {
      const bool isImageReady = _encodedImage.AddChunk(msg);
      
      if(isImageReady)
      {
        Vision::ImageRGB img;
        Result result = _encodedImage.DecodeImageRGB(img);
        if(RESULT_OK != result) {
          printf("WARNING: image decode failed");
          return;
        }
        
        cv::Mat cvImg = img.get_CvMat_();
        
        const s32 outputColor = 1; // 1 for Green, 2 for Blue
        
        for(s32 i=0; i<cvImg.rows; ++i) {
          
          if(i % 2 == 0) {
            cv::Mat img_i = cvImg.row(i);
            img_i.setTo(0);
          } else {
            u8* img_i = cvImg.ptr(i);
            for(s32 j=0; j<cvImg.cols; ++j) {
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
        
        img_ = cozmoCam_->imageNew(cvImg.cols, cvImg.rows, cvImg.data, webots::Display::RGB);
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
                            msg.img_rect.x_topLeft + msg.img_rect.width/4 + 1,
                            msg.img_rect.y_topLeft + msg.img_rect.height/2 + 1);
        
        cozmoCam_->setColor(0xff0000);
        cozmoCam_->drawRectangle(msg.img_rect.x_topLeft, msg.img_rect.y_topLeft,
                                 msg.img_rect.width, msg.img_rect.height);
        cozmoCam_->drawText(dispStr,
                            msg.img_rect.x_topLeft + msg.img_rect.width/4,
                            msg.img_rect.y_topLeft + msg.img_rect.height/2);

        // Record centroid of observation in image
        _lastObservedImageCentroid.SetFromMessage(msg);
      }
      
    }
    
    void WebotsKeyboardController::HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg)
    {
      //printf("RECEIVED FACE OBSERVED: faceID %llu\n", msg.faceID);
      // _lastFace = msg;
      
      // Record centroid of observation in image
      _lastObservedImageCentroid.SetFromMessage(msg);
    }

    void WebotsKeyboardController::HandleRobotObservedPet(ExternalInterface::RobotObservedPet const& msg)
    {
      // Record centroid of observation in image
      _lastObservedImageCentroid.SetFromMessage(msg);
    }
    
    void WebotsKeyboardController::HandleLoadedKnownFace(Vision::LoadedKnownFace const& msg)
    {
      printf("HandleLoadedKnownFace: '%s' (ID:%d) first enrolled %zd seconds ago, last updated %zd seconds ago, last seen %zd seconds ago\n",
             msg.name.c_str(), msg.faceID, msg.secondsSinceFirstEnrolled, msg.secondsSinceLastUpdated, msg.secondsSinceLastSeen);
    }
    
    void WebotsKeyboardController::HandleDebugString(ExternalInterface::DebugString const& msg)
    {
      // Useful for debug, but otherwise unneeded since this is displayed in the
      // status window
      //printf("HandleDebugString: %s\n", msg.text.c_str());
    }

    void WebotsKeyboardController::HandleEngineErrorCode(const ExternalInterface::EngineErrorCodeMessage& msg)
    {
      printf("HandleEngineErrorCode: %s\n", EnumToString(msg.errorCode));
    }

    void WebotsKeyboardController::HandleFaceEnrollmentCompleted(const ExternalInterface::FaceEnrollmentCompleted &msg)
    {
      if(FaceEnrollmentResult::Success == msg.result)
      {
        printf("FaceEnrollmentCompleted: Added '%s' with ID=%d\n",
               msg.name.c_str(), msg.faceID);
      } else {
        printf("FaceEnrollment FAILED with result = '%s'\n", EnumToString(msg.result));
      }
      
    } // HandleRobotCompletedAction()
    
    // ============== End of message handlers =================

    void WebotsKeyboardController::PreInit()
    {
      // Make root point to WebotsKeyBoardController node
      root_ = GetSupervisor()->getSelf();

      // enable keyboard
      GetSupervisor()->getKeyboard()->enable(GetStepTimeMS());
    }
  
    void WebotsKeyboardController::WaitOnKeyboardToConnect()
    {
      webots::Field* autoConnectField = root_->getField("autoConnect");
      if( autoConnectField == nullptr ) {
        PRINT_NAMED_ERROR("WebotsKeyboardController.MissingField",
                          "missing autoConnect field, assuming we should auto connect");
        return;
      }
      else {
        bool autoConnect = autoConnectField->getSFBool();
        if( autoConnect ) {
          return;
        }
      }

      LOG_INFO("WebotsKeyboardController.WaitForStart", "Press Shift+Enter to start the engine");
      
      const int EnterKey = 4; // tested experimentally... who knows if this will work on other platforms
      const int ShiftEnterKey = EnterKey | webots::Keyboard::SHIFT;

      bool start = false;
      while( !start && !_shouldQuit ) {
        int key = -1;
        while((key = GetSupervisor()->getKeyboard()->getKey()) >= 0 && !_shouldQuit) {
          if(!start && key == ShiftEnterKey) {
            start = true;
            LOG_INFO("WebotsKeyboardController.StartEngine", "Starting our engines....");
          }
        }
        // manually step simulation
        GetSupervisor()->step(GetStepTimeMS());
      }
    }
  
    void WebotsKeyboardController::InitInternal()
    { 
      poseMarkerDiffuseColor_ = root_->getField("poseMarkerDiffuseColor");
        
      cozmoCam_ = GetSupervisor()->getDisplay("uiCamDisplay");
      
      auto doAutoBlockpoolField = root_->getField("doAutoBlockpool");
      if (doAutoBlockpoolField) {
        LOG_INFO("WebotsCtrlKeyboard.Init.DoAutoBlockpool", "%d", doAutoBlockpoolField->getSFBool());
        EnableAutoBlockpool(doAutoBlockpoolField->getSFBool());
      }
      
      _lastObservedImageCentroid.point = {-1.f,-1.f};
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
        printf("                   Turn in place:  < >\n");
        printf("               Move lift up/down:  a/z\n");
        printf("               Move head up/down:  s/x\n");
        printf("             Lift low/high/carry:  1/2/3\n");
        printf("            Head down/forward/up:  4/5/6\n");
        printf("  Turn towards last obs centroid:  0\n");
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
        printf("         Select behavior by type:  Shift+c\n");
        printf("         Select behavior by name:  Alt+Shift+c\n");
        printf("          Dock to selected block:  p\n");
        printf("          Dock from current pose:  Shift+p\n");
        printf("    Travel up/down selected ramp:  r\n");
        printf("                      Face plant:  Alt+r\n");
        printf("              Abort current path:  q\n");
        printf("                Abort everything:  Shift+q\n");
        printf("           Cancel current action:  Alt+q\n");
        printf("         Update controller gains:  k\n");
        printf("                 Request IMU log:  o\n");
        printf("           Toggle face detection:  f\n");
        printf(" Assign userName to current face:  Shift+f\n");
        printf("          Turn towards last face:  Alt+f\n");
        printf("              Reset 'owner' face:  Alt+Shift+f\n");
        printf("                      Test modes:  Alt + Testmode#\n");
        printf("                Follow test plan:  t\n");
        printf("       Force-add specified robot:  Shift+r\n");
        printf("                 Select behavior:  Shift+c\n");
        printf("         Select behavior chooser:  h\n");
        printf("       Select spark (unlockName):  Shift+h\n");
        printf("         Exit spark (unlockName):  Alt+h\n");
        printf("            Set emotion to value:  m\n");
        printf("     Rainbow pattern on backpack:  l\n");        
        printf("      Search side to side action:  Shift+l\n");
        printf("    Toggle cliff sensor handling:  Alt+l\n");
        printf("  Toggle engine lights component:  Alt+Shift+l\n");
        printf("      Play 'animationToSendName':  Shift+6\n");
        printf("  Set idle to'idleAnimationName':  Alt+Shift+6\n");
        printf("     Update Viz origin alignment:  ` <backtick>\n");
        printf("       Unlock progression unlock:  n\n");
        printf("         Lock progression unlock:  Shift+n\n");
        printf("    Respond 'no' to game request:  Alt+n\n");
        printf("             Flip selected block:  y\n");
        printf("                Set robot volume:  v\n");
        printf("      Toggle vision while moving:  V\n");
        printf("       Realign with block action:  _\n");
        printf("Toggle accel from streamObjectID: |\n");
        printf("               Toggle headlights: ,\n");
        printf("             Pronounce sayString: \" <double-quote>\n");
        printf("                 Set console var: ]\n");
        printf("        Quit keyboard controller:  Alt+Shift+x\n");
        printf("                      Print help:  ?,/\n");
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
        f32 driveAccel = root_->getField("driveAccel")->getSFFloat();
        
        f32 steeringCurvature = root_->getField("steeringCurvature")->getSFFloat();
        
        static bool keyboardRestart = false;
        if (keyboardRestart) {
          GetSupervisor()->getKeyboard()->disable();
          GetSupervisor()->getKeyboard()->enable(BS_TIME_STEP);
          keyboardRestart = false;
        }
        
        // Get all keys pressed this tic
        std::set<int> keysPressed;
        int key;
        while((key = GetSupervisor()->getKeyboard()->getKey()) >= 0) {
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
          const int modifier_key = key & ~webots::Keyboard::KEY;
          const bool shiftKeyPressed = modifier_key & webots::Keyboard::SHIFT;
          const bool altKeyPressed = modifier_key & webots::Keyboard::ALT;
          
          // Set key to its modifier-less self
          key &= webots::Keyboard::KEY;
          
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
          f32 liftSpeed = DEG_TO_RAD(root_->getField("liftSpeedDegPerSec")->getSFFloat());
          f32 liftAccel = DEG_TO_RAD(root_->getField("liftAccelDegPerSec2")->getSFFloat());
          f32 liftDurationSec = root_->getField("liftDurationSec")->getSFFloat();
          f32 headSpeed = DEG_TO_RAD(root_->getField("headSpeedDegPerSec")->getSFFloat());
          f32 headAccel = DEG_TO_RAD(root_->getField("headAccelDegPerSec2")->getSFFloat());
          f32 headDurationSec = root_->getField("headDurationSec")->getSFFloat();
          if (shiftKeyPressed) {
            wheelSpeed = root_->getField("driveSpeedSlow")->getSFFloat();
            liftSpeed *= 0.5;
            headSpeed *= 0.5;
          } else if(altKeyPressed) {
            wheelSpeed = root_->getField("driveSpeedTurbo")->getSFFloat();
          }
          
          // Point turn amount and speed/accel
          f32 pointTurnAngle = std::fabs(root_->getField("pointTurnAngle_deg")->getSFFloat());
          f32 pointTurnSpeed = std::fabs(root_->getField("pointTurnSpeed_degPerSec")->getSFFloat());
          f32 pointTurnAccel = std::fabs(root_->getField("pointTurnAccel_degPerSec2")->getSFFloat());
          
          // Dock speed
          const f32 dockSpeed_mmps = root_->getField("dockSpeed_mmps")->getSFFloat();
          const f32 dockAccel_mmps2 = root_->getField("dockAccel_mmps2")->getSFFloat();
          const f32 dockDecel_mmps2 = root_->getField("dockDecel_mmps2")->getSFFloat();
          
          // Path speeds
          const f32 pathSpeed_mmps = root_->getField("pathSpeed_mmps")->getSFFloat();
          const f32 pathAccel_mmps2 = root_->getField("pathAccel_mmps2")->getSFFloat();
          const f32 pathDecel_mmps2 = root_->getField("pathDecel_mmps2")->getSFFloat();
          const f32 pathPointTurnSpeed_radPerSec = root_->getField("pathPointTurnSpeed_radPerSec")->getSFFloat();
          const f32 pathPointTurnAccel_radPerSec2 = root_->getField("pathPointTurnAccel_radPerSec2")->getSFFloat();
          const f32 pathPointTurnDecel_radPerSec2 = root_->getField("pathPointTurnDecel_radPerSec2")->getSFFloat();
          const f32 pathReverseSpeed_mmps = root_->getField("pathReverseSpeed_mmps")->getSFFloat();

          // If any of the pathMotionProfile fields are different than the default values use a custom profile
          if(pathMotionProfile_.speed_mmps != pathSpeed_mmps ||
             pathMotionProfile_.accel_mmps2 != pathAccel_mmps2 ||
             pathMotionProfile_.decel_mmps2 != pathDecel_mmps2 ||
             pathMotionProfile_.pointTurnSpeed_rad_per_sec != pathPointTurnSpeed_radPerSec ||
             pathMotionProfile_.pointTurnAccel_rad_per_sec2 != pathPointTurnAccel_radPerSec2 ||
             pathMotionProfile_.pointTurnDecel_rad_per_sec2 != pathPointTurnDecel_radPerSec2 ||
             pathMotionProfile_.dockSpeed_mmps != dockSpeed_mmps ||
             pathMotionProfile_.dockAccel_mmps2 != dockAccel_mmps2 ||
             pathMotionProfile_.dockDecel_mmps2 != dockDecel_mmps2 ||
             pathMotionProfile_.reverseSpeed_mmps != pathReverseSpeed_mmps)
          {
            pathMotionProfile_.isCustom = true;
          }

          pathMotionProfile_.speed_mmps = pathSpeed_mmps;
          pathMotionProfile_.accel_mmps2 = pathAccel_mmps2;
          pathMotionProfile_.decel_mmps2 = pathDecel_mmps2;
          pathMotionProfile_.pointTurnSpeed_rad_per_sec = pathPointTurnSpeed_radPerSec;
          pathMotionProfile_.pointTurnAccel_rad_per_sec2 = pathPointTurnAccel_radPerSec2;
          pathMotionProfile_.pointTurnDecel_rad_per_sec2 = pathPointTurnDecel_radPerSec2;
          pathMotionProfile_.dockSpeed_mmps = dockSpeed_mmps;
          pathMotionProfile_.dockAccel_mmps2 = dockAccel_mmps2;
          pathMotionProfile_.dockDecel_mmps2 = dockDecel_mmps2;
          pathMotionProfile_.reverseSpeed_mmps = pathReverseSpeed_mmps;
          
          // For pickup or placeRel, specify whether or not you want to use the
          // given approach angle for pickup, placeRel, or roll actions
          bool useApproachAngle = root_->getField("useApproachAngle")->getSFBool();
          f32 approachAngle_rad = DEG_TO_RAD(root_->getField("approachAngle_deg")->getSFFloat());
          
          // For placeOn and placeOnGround, specify whether or not to use the exactRotation specified
          bool useExactRotation = root_->getField("useExactPlacementRotation")->getSFBool();
          
          //printf("keypressed: %d, modifier %d, orig_key %d, prev_key %d\n",
          //       key, modifier_key, key | modifier_key, lastKeyPressed_);
          
          const std::string drivingStartAnim = root_->getField("drivingStartAnim")->getSFString();
          const std::string drivingLoopAnim = root_->getField("drivingLoopAnim")->getSFString();
          const std::string drivingEndAnim = root_->getField("drivingEndAnim")->getSFString();
          if(_drivingStartAnim.compare(drivingStartAnim) != 0 ||
             _drivingLoopAnim.compare(drivingLoopAnim) != 0 ||
             _drivingEndAnim.compare(drivingEndAnim) != 0)
          {
            _drivingStartAnim = drivingStartAnim;
            _drivingLoopAnim = drivingLoopAnim;
            _drivingEndAnim = drivingEndAnim;
          
            // Pop whatever driving animations were being used and push the new ones
            ExternalInterface::MessageGameToEngine msg1;
            msg1.Set_PopDrivingAnimations(ExternalInterface::PopDrivingAnimations());
            SendMessage(msg1);
          
            ExternalInterface::PushDrivingAnimations m;
            m.drivingStartAnim = AnimationTriggerFromString(_drivingStartAnim.c_str());
            m.drivingLoopAnim = AnimationTriggerFromString(_drivingLoopAnim.c_str());
            m.drivingEndAnim = AnimationTriggerFromString(_drivingEndAnim.c_str());
            
            ExternalInterface::MessageGameToEngine msg2;
            msg2.Set_PushDrivingAnimations(m);
            SendMessage(msg2);
          }
          
          
          // Check for test mode (alt + key)
          bool testMode = false;
          if (altKeyPressed) {
            if (key >= '0' && key <= '9') {
              if (shiftKeyPressed) {
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
                  p3 = root_->getField("liftTest_powerPercent")->getSFInt32();    // Power to run motor at. If 0, cycle through increasing power. Only used during LiftTF_TEST_POWER.
                  break;
                case TestMode::TM_HEAD:
                  p1 = root_->getField("headTest_flags")->getSFInt32();
                  p2 = root_->getField("headTest_nodCycleTimeMS")->getSFInt32();  // Nodding cycle time in ms (if HTF_NODDING flag is set)
                  p3 = root_->getField("headTest_powerPercent")->getSFInt32();    // Power to run motor at. If 0, cycle through increasing power. Only used during HTF_TEST_POWER.
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
                case TestMode::TM_STOP_TEST:
                  p1 = 100;  // slow speed (mmps)
                  p2 = 200;  // fast speed (mmps)
                  p3 = 1000; // period (ms)
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
              case webots::Keyboard::UP:
              {
                ++throttleDir;
                break;
              }
                
              case webots::Keyboard::DOWN:
              {
                --throttleDir;
                break;
              }
                
              case webots::Keyboard::LEFT:
              {
                --steeringDir;
                break;
              }
                
              case webots::Keyboard::RIGHT:
              {
                ++steeringDir;
                break;
              }
                
              case (s32)'<':
              {
                if(altKeyPressed) {
                  SendTurnInPlaceAtSpeed(DEG_TO_RAD(pointTurnSpeed), DEG_TO_RAD(pointTurnAccel));
                }
                else {
                  SendTurnInPlace(DEG_TO_RAD(pointTurnAngle), DEG_TO_RAD(pointTurnSpeed), DEG_TO_RAD(pointTurnAccel));
                }
                break;
              }
                
              case (s32)'>':
              {
                if(altKeyPressed) {
                  SendTurnInPlaceAtSpeed(DEG_TO_RAD(-pointTurnSpeed), DEG_TO_RAD(pointTurnAccel));
                } else {
                  SendTurnInPlace(DEG_TO_RAD(-pointTurnAngle), DEG_TO_RAD(-pointTurnSpeed), DEG_TO_RAD(pointTurnAccel));
                }
                break;
              }
                
              case webots::Keyboard::PAGEUP:
              {
                SendMoveHeadToAngle(MAX_HEAD_ANGLE, 20, 2);
                break;
              }
                
              case webots::Keyboard::PAGEDOWN:
              {
                SendMoveHeadToAngle(MIN_HEAD_ANGLE, 20, 2);
                break;
              }
                
              case (s32)'S':
              {
                if(altKeyPressed) {
                  // Re-read animations and send them to physical robot
                  SendReplayLastAnimation();
                }
                else {
                  commandedHeadSpeed += headSpeed;
                  movingHead = true;
                }
                break;
              }
                
              case (s32)'X':
              {
                if(shiftKeyPressed && altKeyPressed) {
                  _shouldQuit = true;
                }
                else {
                  commandedHeadSpeed -= headSpeed;
                  movingHead = true;
                }
                break;
              }
                
              case (s32)'A':
              {
                if(altKeyPressed) {
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
                if(altKeyPressed) {
                  static bool liftPowerEnable = false;
                  SendEnableLiftPower(liftPowerEnable);
                  liftPowerEnable = !liftPowerEnable;
                } else {
                  commandedLiftSpeed -= liftSpeed;
                  movingLift = true;
                }
                break;
              }
                
              case '0':
              {
                if(_lastObservedImageCentroid.point.AllGTE(0.f))
                {
                  using namespace ExternalInterface;
                  TurnTowardsImagePoint msg;
                  
                  msg.x = _lastObservedImageCentroid.point.x();
                  msg.y = _lastObservedImageCentroid.point.y();
                  msg.timestamp = _lastObservedImageCentroid.timestamp;
                  
                  SendMessage(MessageGameToEngine(std::move(msg)));
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
                
                if (shiftKeyPressed) {
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
                
                if (shiftKeyPressed) {
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
                ImageSendMode mode = ImageSendMode::Off;
                
                const bool alsoSaveState = altKeyPressed;
                
                if (shiftKeyPressed) {
                  static bool streamOn = false;
                  if (streamOn) {
                    mode = ImageSendMode::Off;
                    printf("Saving robot image/state stream OFF.\n");
                  } else {
                    mode = ImageSendMode::Stream;
                    printf("Saving robot image %sstream ON.\n", alsoSaveState ? "and state " : "");
                  }
                  streamOn = !streamOn;
                } else {
                  mode = ImageSendMode::SingleShot;
                  printf("Saving single robot image%s.\n", alsoSaveState ? " and state message" : "");
                }
                
                SendSaveImages(mode);
                if(alsoSaveState) {
                  // NOTE: saving robot state does not support "one-shot", so it will just toggle on and off
                  SendSaveState(mode != ImageSendMode::Off);
                }
                break;
              }
                
              case (s32)'D':
              {
              
                // Shift+Alt+D = delocalize
                if(shiftKeyPressed && altKeyPressed)
                {
                  ExternalInterface::ForceDelocalizeRobot delocMsg;
                  delocMsg.robotID = 1;
                  SendMessage(ExternalInterface::MessageGameToEngine(std::move(delocMsg)));
                } else if(shiftKeyPressed) {
                  
                  // FREE KEY COMBO!!!
                  
                } else if(altKeyPressed) {

                  // FREE KEY COMBO!!!
                  
                } else {
                  static bool showObjects = false;
                  SendEnableDisplay(showObjects);
                  showObjects = !showObjects;
                }
                break;
              }
                
              case (s32)'G':
              {
                if (shiftKeyPressed) {
                  poseMarkerMode_ = !poseMarkerMode_;
                  printf("Pose marker mode: %d\n", poseMarkerMode_);
                  poseMarkerDiffuseColor_->setSFColor(poseMarkerColor_[poseMarkerMode_]);
                  SendErasePoseMarker();
                  break;
                }
                
                bool useManualSpeed = altKeyPressed;
                  
                if (poseMarkerMode_ == 0) {
                  // Execute path to pose
                  printf("Going to pose marker at x=%f y=%f angle=%f (useManualSpeed: %d)\n",
                         poseMarkerPose_.GetTranslation().x(),
                         poseMarkerPose_.GetTranslation().y(),
                         poseMarkerPose_.GetRotationAngle<'Z'>().ToFloat(),
                         useManualSpeed);

                  SendExecutePathToPose(poseMarkerPose_, pathMotionProfile_, useManualSpeed);
                  //SendMoveHeadToAngle(-0.26, headSpeed, headAccel);
                } else {
                  
                  // Indicate whether or not to place object at the exact rotation specified or
                  // just use the nearest preActionPose so that it's merely aligned with the specified pose.
                  printf("Setting block on ground at rotation %f rads about z-axis (%s)\n", poseMarkerPose_.GetRotationAngle<'Z'>().ToFloat(), useExactRotation ? "Using exact rotation" : "Using nearest preActionPose" );
                  
                  SendPlaceObjectOnGroundSequence(poseMarkerPose_,
                                                  pathMotionProfile_,
                                                  useExactRotation,
                                                  useManualSpeed);
                  // Make sure head is tilted down so that it can localize well
                  //SendMoveHeadToAngle(-0.26, headSpeed, headAccel);
                  
                }
                break;
              }
                
              case (s32)'L':
              {

                if(shiftKeyPressed && altKeyPressed) {
                  ExternalInterface::EnableLightStates msg;
                  static bool enableLightComponent = false;
                  printf("EnableLightsComponent: %d", enableLightComponent);
                  msg.enable = enableLightComponent;
                  enableLightComponent = !enableLightComponent;
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_EnableLightStates(msg);
                  SendMessage(msgWrapper);
                }
                else if(shiftKeyPressed) {
                  ExternalInterface::QueueSingleAction msg;
                  msg.robotID = 1;
                  msg.position = QueueActionPosition::NOW;
                  
                  using SFNOD = ExternalInterface::SearchForNearbyObjectDefaults;
                  ExternalInterface::SearchForNearbyObject searchAction {
                    -1,
                    Util::numeric_cast<f32>(Util::EnumToUnderlying(SFNOD::BackupDistance_mm)),
                    Util::numeric_cast<f32>(Util::EnumToUnderlying(SFNOD::BackupSpeed_mms)),
                    Util::numeric_cast<f32>(DEG_TO_RAD(Util::EnumToUnderlying(SFNOD::HeadAngle_deg)))
                  };
                  msg.action.Set_searchForNearbyObject(std::move(searchAction));

                  ExternalInterface::MessageGameToEngine message;
                  message.Set_QueueSingleAction(msg);
                  SendMessage(message);
                }
                else if (altKeyPressed) {
                  static bool enableCliffSensor = false;

                  printf("setting enable cliff sensor to %d\n", enableCliffSensor);
                  ExternalInterface::MessageGameToEngine msg;
                  msg.Set_EnableCliffSensor(ExternalInterface::EnableCliffSensor{enableCliffSensor});
                  SendMessage(msg);

                  enableCliffSensor = !enableCliffSensor;
                }
                else {

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
                    msg.offset[i] = 0;
                  }
                
                  if(!backpackLightsOn) {
                    // Use red channel to control left and right lights
                    msg.onColor[(uint32_t)LEDId::LED_BACKPACK_RIGHT]  = ::Anki::NamedColors::RED >> 1; // Make right light dimmer
                    msg.onColor[(uint32_t)LEDId::LED_BACKPACK_LEFT]   = ::Anki::NamedColors::RED;
                    msg.onColor[(uint32_t)LEDId::LED_BACKPACK_BACK]   = ::Anki::NamedColors::RED;
                    msg.onColor[(uint32_t)LEDId::LED_BACKPACK_MIDDLE] = ::Anki::NamedColors::CYAN;
                    msg.onColor[(uint32_t)LEDId::LED_BACKPACK_FRONT]  = ::Anki::NamedColors::YELLOW;
                  }
                
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_SetBackpackLEDs(msg);
                  SendMessage(msgWrapper);

                  backpackLightsOn = !backpackLightsOn;
                }
                
                break;
              }
                
              case (s32)'T':
              {
                if(altKeyPressed && shiftKeyPressed)
                {
                  // Hijacking this for pet tracking for now, since we aren't doing much
                  // tool code reading these days...
                  //SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::ReadToolCode()));
                  
                  using namespace ExternalInterface;
                  TrackToPet trackAction(5.f, Vision::UnknownFaceID, Vision::PetType::Unknown);
                  SendMessage(MessageGameToEngine(std::move(trackAction)));
                  
                } else if(shiftKeyPressed) {
                  static bool trackingObject = false;
                  
                  trackingObject = !trackingObject;

                  if(trackingObject) {
                    const bool headOnly = false;
                    
                    printf("Telling robot to track %sto the currently observed object %d\n",
                           headOnly ? "its head " : "",
                           GetLastObservedObject().id);
                    
                    SendTrackToObject(GetLastObservedObject().id, headOnly);
                  } else {
                    // Disable tracking
                    SendTrackToObject(std::numeric_limits<u32>::max());
                  }
                  
                } else if(altKeyPressed) {
                  static bool trackingFace = false;
                  
                  trackingFace = !trackingFace;
                  
                  if(trackingFace) {
                    const bool headOnly = false;
                    
                    printf("Telling robot to track %sto the currently observed face %d\n",
                           headOnly ? "its head " : "",
                           (u32)GetLastObservedFaceID());
                    
                    SendTrackToFace((u32)GetLastObservedFaceID(), headOnly);
                  } else {
                    // Disable tracking
                    SendTrackToFace(std::numeric_limits<u32>::max());
                  }

                } else {
                  SendExecuteTestPlan(pathMotionProfile_);
                }
                break;
              }
                
              case (s32)'.':
              {
                SendSelectNextObject();
                break;
              }
                
              case (s32)',':
              {
                static bool toggle = true;
                printf("Turning headlight %s\n", toggle ? "ON" : "OFF");
                SendSetHeadlight(toggle);
                toggle = !toggle;
                break;
              }
                
              case (s32)'|':
              {
                static bool enableAccelStreaming = true;
                u32 streamObjectID = root_->getField("streamObjectID")->getSFInt32();
                printf("%s streaming of accel data from object %d\n", enableAccelStreaming ? "Enable" : "Disable", streamObjectID);
                ExternalInterface::StreamObjectAccel msg(streamObjectID, enableAccelStreaming);
                SendMessage(ExternalInterface::MessageGameToEngine(std::move(msg)));
                enableAccelStreaming = !enableAccelStreaming;
                break;
              }
                
              case (s32)'C':
              {
                if(shiftKeyPressed) {
               
                  // Send whatever animation is specified in the animationToSendName field
                  webots::Field* behaviorNameField = root_->getField("behaviorName");
                  if (behaviorNameField == nullptr) {
                    printf("ERROR: No behaviorName field found in WebotsKeyboardController.proto\n");
                    break;
                  }
                  std::string behaviorName = behaviorNameField->getSFString();
                  if (behaviorName.empty()) {
                    printf("ERROR: behaviorName field is empty\n");
                    break;
                  }
                  
                  // FactoryTest behavior has to start on a charger so we need to wake up the robot first
                  if(behaviorName == "FactoryTest")
                  {
                    // Mute sound because annoying
                    SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetDebugConsoleVarMessage("BFT_PlaySound", "false")));
                    SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetDebugConsoleVarMessage("BFT_ConnectToRobotOnly", "false")));

                    
                    SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::EnableAllReactionTriggers("webots",false)));
                    SendSetRobotVolume(1.f);
                  }
                  
                  SendMessage(ExternalInterface::MessageGameToEngine(
                                ExternalInterface::ActivateBehaviorChooser(BehaviorChooserType::Selection)));

                  printf("Selecting behavior by NAME: %s\n", behaviorName.c_str());
                  if (behaviorName == "LiftLoadTest") {
                    SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetLiftLoadTestAsRunnable()));
                  }
                  SendMessage(ExternalInterface::MessageGameToEngine(
                                ExternalInterface::ExecuteBehaviorByName(behaviorName)));
                }
                else if(altKeyPressed) {
                
                  // rsam: Clear and Delete have change its meaning. Removing until someone complains that it's gone,
                  // at which point we can evaluate what they need. Sorry if this causes interruptions, unfortunately
                  // I can't keep supporting all current features in the refactor.
                  
                  // SendClearAllObjects();
                }
                else {
                
                  // rsam: Clear and Delete have change its meaning. Removing until someone complains that it's gone,
                  // at which point we can evaluate what they need. Sorry if this causes interruptions, unfortunately
                  // I can't keep supporting all current features in the refactor.
                
                  // 'c' without SHIFT
                  // SendClearAllBlocks();
                }
                break;
              }

              case (s32)'H':
              {

                if( shiftKeyPressed || altKeyPressed)
                {

                  if( shiftKeyPressed && altKeyPressed ) {
                    printf("ERROR: invalid hotkey\n");
                    break;
                  }

                  if( shiftKeyPressed ) {
                    webots::Field* unlockNameField = root_->getField("unlockName");
                    if (unlockNameField == nullptr) {
                      printf("ERROR: No unlockNameField field found in WebotsKeyboardController.proto\n");
                      break;
                    }
                
                    std::string unlockName = unlockNameField->getSFString();
                    if (unlockName.empty()) {
                      printf("ERROR: unlockName field is empty\n");
                      break;
                    }

                    UnlockId unlock = UnlockIdsFromString(unlockName.c_str());
                    if( unlock != UnlockId::Count ) {
                      ExternalInterface::ActivateSpark activate(unlock);
                      ExternalInterface::BehaviorManagerMessageUnion behaviorUnion;
                      behaviorUnion.Set_ActivateSpark(activate);
                      ExternalInterface::BehaviorManagerMessage behaviorMsg(1, behaviorUnion);
                      ExternalInterface::MessageGameToEngine msg;
                      msg.Set_BehaviorManagerMessage(behaviorMsg);
                      SendMessage(msg);
                    }
                    else {
                      PRINT_NAMED_WARNING("StartSpark.InvalidSparkName",
                                          "no unlock found for '%s'",
                                          unlockName.c_str());
                    }
                  }
                  else {
                    // deactivate spark
                    ExternalInterface::ActivateSpark deactivate(UnlockId::Count);
                    ExternalInterface::BehaviorManagerMessageUnion behaviorUnion;
                    behaviorUnion.Set_ActivateSpark(deactivate);
                    ExternalInterface::BehaviorManagerMessage behaviorMsg(1, behaviorUnion);
                    ExternalInterface::MessageGameToEngine msg;
                    msg.Set_BehaviorManagerMessage(behaviorMsg);
                    SendMessage(msg);
                  }
                }
                else {
                  // select behavior chooser
                  webots::Field* behaviorChooserNameField = root_->getField("behaviorChooserName");
                  if (behaviorChooserNameField == nullptr) {
                    printf("ERROR: No behaviorChooserNameField field found in WebotsKeyboardController.proto\n");
                    break;
                  }
                  
                  std::string behaviorChooserName = behaviorChooserNameField->getSFString();
                  if (behaviorChooserName.empty()) {
                    printf("ERROR: behaviorChooserName field is empty\n");
                    break;
                  }
                  
                  BehaviorChooserType chooser = BehaviorChooserTypeFromString(behaviorChooserName);
                  if( chooser == BehaviorChooserType::Count ) {
                    printf("ERROR: could not convert string '%s' to valid behavior chooser type\n",
                           behaviorChooserName.c_str());
                    break;
                  }
                  
                  printf("sending behavior chooser '%s'\n", BehaviorChooserTypeToString(chooser));
                
                  SendMessage(ExternalInterface::MessageGameToEngine(
                                ExternalInterface::ActivateBehaviorChooser(chooser)));
                }
                
                break;
              }

              case (s32)'M':
              {
                const uint32_t tag = root_->getField("nvTag")->getSFInt32();
                const uint32_t numBlobs = root_->getField("nvNumBlobs")->getSFInt32();
                const uint32_t blobLength = root_->getField("nvBlobDataLength")->getSFInt32();
                
                // Shift + Alt + M: Erases specified tag
                if(shiftKeyPressed && altKeyPressed)
                {
                  if(ENABLE_NVSTORAGE_WRITE)
                  {
                    SendNVStorageEraseEntry((NVStorage::NVEntryTag)tag);
                  }
                  else
                  {
                    LOG_INFO("SendNVStorageEraseEntry.Disabled",
                             "Set ENABLE_NVSTORAGE_WRITE to 1 if you really want to do this!");
                  }
                }
                // Shift + M: Stores random data to tag
                // If tag is a multi-tag, writes numBlobs blobs of random data blobLength long
                // If tag is a single tag, writes 1 blob of random data that is blobLength long
                else if(shiftKeyPressed)
                {
                  if(ENABLE_NVSTORAGE_WRITE)
                  {
                    Util::RandomGenerator r;
                    
                    for(int i=0;i<numBlobs;i++)
                    {
                      printf("blob data: %d\n", i);
                      uint8_t data[blobLength];
                      for(int i=0;i<blobLength;i++)
                      {
                        int n = r.RandInt(256);
                        printf("%d ", n);
                        data[i] = (uint8_t)n;
                      }
                      printf("\n\n");
                      SendNVStorageWriteEntry((NVStorage::NVEntryTag)tag, data, blobLength, i, numBlobs);
                    }
                  }
                  else
                  {
                    LOG_INFO("SendNVStorageWriteEntry.Disabled",
                             "Set ENABLE_NVSTORAGE_WRITE to 1 if you really want to do this!");
                  }
                  
                  break;
                }
                // Alt + M: Reads data at tag
                else if(altKeyPressed)
                {
                  ClearReceivedNVStorageData((NVStorage::NVEntryTag)tag);
                  SendNVStorageReadEntry((NVStorage::NVEntryTag)tag);
                  break;
                }
              
                webots::Field* emotionNameField = root_->getField("emotionName");
                if (emotionNameField == nullptr) {
                  printf("ERROR: No emotionNameField field found in WebotsKeyboardController.proto\n");
                  break;
                }
                
                std::string emotionName = emotionNameField->getSFString();
                if (emotionName.empty()) {
                  printf("ERROR: emotionName field is empty\n");
                  break;
                }
                
                webots::Field* emotionValField = root_->getField("emotionVal");
                if (emotionValField == nullptr) {
                  printf("ERROR: No emotionValField field found in WebotsKeyboardController.proto\n");
                  break;
                }

                float emotionVal = emotionValField->getSFFloat();
                EmotionType emotionType = EmotionTypeFromString(emotionName.c_str());

                SendMessage(ExternalInterface::MessageGameToEngine(
                              ExternalInterface::MoodMessage(1,
                                ExternalInterface::MoodMessageUnion(
                                  ExternalInterface::SetEmotion( emotionType, emotionVal )))));

                break;
              }
                
              case (s32)'P':
              {
                bool usePreDockPose = !shiftKeyPressed;
                //bool useManualSpeed = altKeyPressed;
                
                // Hijacking ALT key for low placement
                bool useManualSpeed = false;
                bool placeOnGroundAtOffset = altKeyPressed;

                f32 placementOffsetX_mm = 0;
                if (placeOnGroundAtOffset) {
                  placementOffsetX_mm = root_->getField("placeOnGroundOffsetX_mm")->getSFFloat();
                }
                
                // Exact rotation to use if useExactRotation == true
                const double* rotVals = root_->getField("exactPlacementRotation")->getSFRotation();
                Rotation3d rot(rotVals[3], {static_cast<f32>(rotVals[0]), static_cast<f32>(rotVals[1]), static_cast<f32>(rotVals[2])} );
                printf("Rotation %f\n", rot.GetAngleAroundZaxis().ToFloat());
                
                if (GetCarryingObjectID() < 0) {
                  // Not carrying anything so pick up!
                  SendPickupSelectedObject(pathMotionProfile_,
                                           usePreDockPose,
                                           useApproachAngle,
                                           approachAngle_rad,
                                           useManualSpeed);
                } else {
                  if (placeOnGroundAtOffset) {
                    SendPlaceRelSelectedObject(pathMotionProfile_,
                                               usePreDockPose,
                                               placementOffsetX_mm,
                                               useApproachAngle,
                                               approachAngle_rad,
                                               useManualSpeed);
                  } else {
                    SendPlaceOnSelectedObject(pathMotionProfile_,
                                              usePreDockPose,
                                              useApproachAngle,
                                              approachAngle_rad,
                                              useManualSpeed);
                  }
                }
                
                
                break;
              }
                
              case (s32)'R':
              {
                bool usePreDockPose = !shiftKeyPressed;
                bool useManualSpeed = false;
                
                if (altKeyPressed) {
//                  SendTraverseSelectedObject(pathMotionProfile_,
//                                             usePreDockPose,
//                                             useManualSpeed);
                  SendFacePlant(-1,
                                pathMotionProfile_,
                                usePreDockPose);
                } else {
                  SendMountSelectedCharger(pathMotionProfile_,
                                           usePreDockPose,
                                           useManualSpeed);
                }
                break;
              }
                
              case (s32)'W':
              {
                bool usePreDockPose = !shiftKeyPressed;
                bool useManualSpeed = false;
                bool doDeepRoll = root_->getField("doDeepRoll")->getSFBool();
                
                if (altKeyPressed) {
                  SendPopAWheelie(-1,
                                  pathMotionProfile_,
                                  usePreDockPose,
                                  useApproachAngle,
                                  approachAngle_rad,
                                  useManualSpeed);
                } else {
                  SendRollSelectedObject(pathMotionProfile_,
                                         doDeepRoll,
                                         usePreDockPose,
                                         useApproachAngle,
                                         approachAngle_rad,
                                         useManualSpeed);
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
                msg.color = ::Anki::NamedColors::ORANGE;
                
                ExternalInterface::MessageGameToEngine msgWrapper;
                msgWrapper.Set_VisualizeQuad(msg);
                msgHandler_.SendMessage(1, msgWrapper);
                */
                
                if (shiftKeyPressed) {
                  // SHIFT + Q: Cancel everything (paths, animations, docking, etc.)
                  SendAbortAll();
                } else if(altKeyPressed) {
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
                  
                  if(shiftKeyPressed) {
                    f32 steer_k1 = root_->getField("steerK1")->getSFFloat();
                    f32 steer_k2 = root_->getField("steerK2")->getSFFloat();
                    f32 steerDistOffsetCap = root_->getField("steerDistOffsetCap_mm")->getSFFloat();
                    f32 steerAngOffsetCap = root_->getField("steerAngOffsetCap_rad")->getSFFloat();
                    printf("New steering gains: k1 %f, k2 %f, distOffsetCap %f, angOffsetCap %f\n",
                           steer_k1, steer_k2, steerDistOffsetCap, steerAngOffsetCap);
                    SendControllerGains(ControllerChannel::controller_steering, steer_k1, steer_k2, steerDistOffsetCap, steerAngOffsetCap);
                    
                    // Point turn gains
                    f32 kp = root_->getField("pointTurnKp")->getSFFloat();
                    f32 ki = root_->getField("pointTurnKi")->getSFFloat();
                    f32 kd = root_->getField("pointTurnKd")->getSFFloat();
                    f32 maxErrorSum = root_->getField("pointTurnMaxErrorSum")->getSFFloat();
                    printf("New pointTurn gains: kp=%f ki=%f kd=%f maxErrorSum=%f\n", kp, ki, kd, maxErrorSum);
                    SendControllerGains(ControllerChannel::controller_pointTurn, kp, ki, kd, maxErrorSum);
                    
                  } else {
                    
                    // Wheel gains
                    f32 kp = root_->getField("wheelKp")->getSFFloat();
                    f32 ki = root_->getField("wheelKi")->getSFFloat();
                    f32 kd = 0;
                    f32 maxErrorSum = root_->getField("wheelMaxErrorSum")->getSFFloat();
                    printf("New wheel gains: kp=%f ki=%f kd=%f\n", kp, ki, maxErrorSum);
                    SendControllerGains(ControllerChannel::controller_wheel, kp, ki, kd, maxErrorSum);
                    
                    // Head and lift gains
                    kp = root_->getField("headKp")->getSFFloat();
                    ki = root_->getField("headKi")->getSFFloat();
                    kd = root_->getField("headKd")->getSFFloat();
                    maxErrorSum = root_->getField("headMaxErrorSum")->getSFFloat();
                    printf("New head gains: kp=%f ki=%f kd=%f maxErrorSum=%f\n", kp, ki, kd, maxErrorSum);
                    SendControllerGains(ControllerChannel::controller_head, kp, ki, kd, maxErrorSum);
                    
                    kp = root_->getField("liftKp")->getSFFloat();
                    ki = root_->getField("liftKi")->getSFFloat();
                    kd = root_->getField("liftKd")->getSFFloat();
                    maxErrorSum = root_->getField("liftMaxErrorSum")->getSFFloat();
                    printf("New lift gains: kp=%f ki=%f kd=%f maxErrorSum=%f\n", kp, ki, kd, maxErrorSum);
                    SendControllerGains(ControllerChannel::controller_lift, kp, ki, kd, maxErrorSum);
                  }
                } else {
                  printf("No WebotsKeyboardController was found in world\n");
                }
                break;
              }
                
              case (s32)'V':
              {
                if(shiftKeyPressed) {
                  static bool visionWhileMovingEnabled = false;
                  visionWhileMovingEnabled = !visionWhileMovingEnabled;
                  printf("%s vision while moving.\n", (visionWhileMovingEnabled ? "Enabling" : "Disabling"));
                  ExternalInterface::VisionWhileMoving msg;
                  msg.enable = visionWhileMovingEnabled;
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_VisionWhileMoving(msg);
                  SendMessage(msgWrapper);
                } else {
                  const f32 robotVolume = root_->getField("robotVolume")->getSFFloat();
                  printf("Set robot volume to %f\n", robotVolume);
                  SendSetRobotVolume(robotVolume);
                }
                break;
              }
                
              case (s32)'B':
              {
                if(shiftKeyPressed && altKeyPressed)
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
                else if(GetLastObservedObject().id >= 0 && GetLastObservedObject().isActive)
                {
                  // Proof of concept: cycle colors
                  const s32 NUM_COLORS = 4;
                  const ColorRGBA colorList[NUM_COLORS] = {
                    ::Anki::NamedColors::RED, ::Anki::NamedColors::GREEN, ::Anki::NamedColors::BLUE,
                    ::Anki::NamedColors::BLACK
                  };

                  static s32 colorIndex = 0;

                  ExternalInterface::SetActiveObjectLEDs msg;
                  msg.objectID = GetLastObservedObject().id;
                  msg.robotID = 1;
                  msg.onPeriod_ms = 250;
                  msg.offPeriod_ms = 250;
                  msg.transitionOnPeriod_ms = 500;
                  msg.transitionOffPeriod_ms = 100;
                  msg.turnOffUnspecifiedLEDs = 1;
                  msg.offset = 0;
                  msg.rotationPeriod_ms = 0;
                  
                  if(shiftKeyPressed) {
                    printf("Updating active block edge\n");
                    msg.onColor = ::Anki::NamedColors::RED;
                    msg.offColor = ::Anki::NamedColors::BLACK;
                    msg.whichLEDs = WhichCubeLEDs::FRONT;
                    msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE;
                    msg.relativeToX = GetRobotPose().GetTranslation().x();
                    msg.relativeToY = GetRobotPose().GetTranslation().y();
                    
                  } else if( altKeyPressed) {
                    static s32 edgeIndex = 0;
                    
                    printf("Turning edge %d new color %d (%x)\n",
                           edgeIndex, colorIndex, u32(colorList[colorIndex]));
                    
                    msg.whichLEDs = (WhichCubeLEDs)(1 << edgeIndex);
                    msg.onColor = colorList[colorIndex];
                    msg.offColor = 0;
                    msg.turnOffUnspecifiedLEDs = 0;
                    msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE;
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
                    msg.offColor = ::Anki::NamedColors::BLACK;
                    msg.whichLEDs = WhichCubeLEDs::FRONT;
                    msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                    msg.turnOffUnspecifiedLEDs = 1;
                    
                    
/*
                    static bool white = false;
                    white = !white;
                    if (white) {
                      ExternalInterface::SetAllActiveObjectLEDs m;
                      m.robotID = 1;
                      m.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                      m.objectID = GetLastObservedObject().id;
                      for(s32 iLED = 0; iLED<(s32)ActiveObjectConstants::NUM_CUBE_LEDS; ++iLED) {
                        m.onColor[iLED]  = ::Anki::NamedColors::WHITE;
                        m.offColor[iLED]  = ::Anki::NamedColors::BLACK;
                        m.onPeriod_ms[iLED] = 250;
                        m.offPeriod_ms[iLED] = 250;
                        m.transitionOnPeriod_ms[iLED] = 500;
                        m.transitionOffPeriod_ms[iLED] = 100;
                      }
                      ExternalInterface::MessageGameToEngine msgWrapper;
                      msgWrapper.Set_SetAllActiveObjectLEDs(m);
                      SendMessage(msgWrapper);
                      break;
                    } else {
                      msg.onColor = ::Anki::NamedColors::RED;
                      msg.offColor = ::Anki::NamedColors::BLACK;
                      msg.whichLEDs = WhichCubeLEDs::FRONT;
                      msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                      msg.turnOffUnspecifiedLEDs = 0;
                      ExternalInterface::MessageGameToEngine msgWrapper;
                      msgWrapper.Set_SetActiveObjectLEDs(msg);
                      SendMessage(msgWrapper);

                      msg.onColor = ::Anki::NamedColors::GREEN;
                      msg.offColor = ::Anki::NamedColors::BLACK;
                      msg.whichLEDs = WhichCubeLEDs::RIGHT;
                      msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                      msg.turnOffUnspecifiedLEDs = 0;
                      msgWrapper.Set_SetActiveObjectLEDs(msg);
                      SendMessage(msgWrapper);
                      
                      msg.onColor = ::Anki::NamedColors::BLUE;
                      msg.offColor = ::Anki::NamedColors::BLACK;
                      msg.whichLEDs = WhichCubeLEDs::BACK;
                      msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                      msg.turnOffUnspecifiedLEDs = 0;
                      msgWrapper.Set_SetActiveObjectLEDs(msg);
                      SendMessage(msgWrapper);

                      msg.onColor = ::Anki::NamedColors::YELLOW;
                      msg.offColor = ::Anki::NamedColors::BLACK;
                      msg.whichLEDs = WhichCubeLEDs::LEFT;
                      msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
                      msg.turnOffUnspecifiedLEDs = 0;
                      msgWrapper.Set_SetActiveObjectLEDs(msg);
                      SendMessage(msgWrapper);
                    }
*/

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
                if (shiftKeyPressed && altKeyPressed) {
                  f32 distToMarker = root_->getField("alignWithObjectDistToMarker_mm")->getSFFloat();
                  SendAlignWithObject(-1, // tell game to use blockworld's "selected" object
                                      distToMarker,
                                      pathMotionProfile_,
                                      true,
                                      useApproachAngle,
                                      approachAngle_rad);
                  
                } else if(shiftKeyPressed) {
                  ExternalInterface::TurnTowardsObject msg;
                  msg.robotID = 1;
                  msg.objectID = std::numeric_limits<u32>::max(); // HACK to tell game to use blockworld's "selected" object
                  msg.panTolerance_rad = DEG_TO_RAD(5);
                  msg.maxTurnAngle_rad = DEG_TO_RAD(90);
                  msg.headTrackWhenDone = 0;
                  
                  ExternalInterface::MessageGameToEngine msgWrapper;
                  msgWrapper.Set_TurnTowardsObject(msg);
                  SendMessage(msgWrapper);
                } else if (altKeyPressed) {
                  SendGotoObject(-1, // tell game to use blockworld's "selected" object
                                 sqrtf(2.f)*44.f,
                                 pathMotionProfile_);
                } else {
                  SendIMURequest(2000);
                }
                break;
              }
                
              case (s32)'`':
              {
                LOG_INFO("KeyboardCtrl.ProcessKeyStroke", "Updating viz origin");
                CycleVizOrigin();
                break;
              }
                
              case (s32) '!':
              {
                webots::Field* factoryIDs = root_->getField("activeObjectFactoryIDs");
                webots::Field* connect = root_->getField("activeObjectConnect");
                
                if (factoryIDs && connect) {
                  ExternalInterface::BlockSelectedMessage msg;
                  for (int i=0; i<factoryIDs->getCount(); ++i) {
                    msg.factoryId = static_cast<u32>(strtol(factoryIDs->getMFString(i).c_str(), nullptr, 16));
                    msg.selected = connect->getSFBool();
                    
                    if (msg.factoryId == 0) {
                      continue;
                    }
                    
                    LOG_INFO("BlockSelected", "factoryID 0x%x, connect %d", msg.factoryId, msg.selected);
                    ExternalInterface::MessageGameToEngine msgWrapper;
                    msgWrapper.Set_BlockSelectedMessage(msg);
                    SendMessage(msgWrapper);
                  }
                }
                break;
              }

              case (s32)'@':
              {
                static bool enable = true;
                ExternalInterface::SendAvailableObjects msg;
                msg.robotID = 1;
                msg.enable = enable;
                
                LOG_INFO("SendAvailableObjects", "enable: %d", enable);
                ExternalInterface::MessageGameToEngine msgWrapper;
                msgWrapper.Set_SendAvailableObjects(msg);
                SendMessage(msgWrapper);
                
                enable = !enable;
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
                if(altKeyPressed) {
                  SendClearCalibrationImages();
                } else {
                  SendSaveCalibrationImage();
                }
                break;
              }
              case (s32)'%':
              {
                SendComputeCameraCalibration();
                break;
              }
              case (s32)'&':
              {
                if(altKeyPressed) {
                  LOG_INFO("SendNVStorageReadEntry", "NVEntry_CameraCalib");
                  ClearReceivedNVStorageData(NVStorage::NVEntryTag::NVEntry_CameraCalib);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CameraCalib);
                } else {
                  
                  if (ENABLE_NVSTORAGE_WRITE) {
                    // Toggle write/erase
                    static bool writeNotErase = true;
                    if (writeNotErase) {
                      f32 focalLength_x = root_->getField("focalLength_x")->getSFFloat();
                      f32 focalLength_y = root_->getField("focalLength_y")->getSFFloat();
                      f32 center_x = root_->getField("imageCenter_x")->getSFFloat();
                      f32 center_y = root_->getField("imageCenter_y")->getSFFloat();
                      LOG_INFO("SendCameraCalibrationEraseEntry",
                               "fx: %f, fy: %f, cx: %f, cy: %f",
                               focalLength_x, focalLength_y, center_x, center_y);

                      // Method 1
                      //SendCameraCalibration(focalLength_x, focalLength_y, center_x, center_y);
                      
                      // Method 2
                      CameraCalibration calib(focalLength_x, focalLength_y,
                                              center_x, center_y,
                                              0, 240, 320, {});
                      std::vector<u8> calibVec(calib.Size());
                      calib.Pack(calibVec.data(), calib.Size());
                      SendNVStorageWriteEntry(NVStorage::NVEntryTag::NVEntry_CameraCalib,
                                              calibVec.data(), calibVec.size(),
                                              0, 1);
                    } else {
                      LOG_INFO("SendNVStorageEraseEntry", "NVEntry_CameraCalib");
                      SendNVStorageEraseEntry(NVStorage::NVEntryTag::NVEntry_CameraCalib);
                    }
                    writeNotErase = !writeNotErase;
                    
                  } else {
                    LOG_INFO("SendNVStorageWriteEntry.CameraCalibration.Disabled",
                             "Set ENABLE_NVSTORAGE_WRITE to 1 if you really want to do this!");
                  }
                  
                }
                break;
              }
              case (s32)'(':
              {
                NVStorage::NVEntryTag tag = NVStorage::NVEntryTag::NVEntry_GameSkillLevels;
                
                // NVStorage multiWrite / multiRead test
                if(altKeyPressed) {
                  LOG_INFO("SendNVStorageReadEntry", "Putting image in %s", EnumToString(tag));
                  ClearReceivedNVStorageData(tag);
                  SendNVStorageReadEntry(tag);
                } else {
                  
                  if (ENABLE_NVSTORAGE_WRITE) {
                    // Toggle write/erase
                    static bool writeNotErase = true;
                    if (writeNotErase) {
                      static const char* inFile = "nvstorage_input.jpg";
                      FILE* fp = fopen(inFile, "rb");
                      if (fp) {
                        std::vector<u8> d(30000);
                        size_t numBytes = fread(d.data(), 1, d.size(), fp);
                        d.resize(numBytes);
                        LOG_INFO("SendNVStorageWriteEntry.ReadInputImage",
                                 "Tag: %s, read %zu bytes", EnumToString(tag), numBytes);
                        
                        ExternalInterface::NVStorageWriteEntry temp;
                        size_t MAX_BLOB_SIZE = temp.data.size();
                        u8 numTotalBlobs = static_cast<u8>(ceilf(static_cast<f32>(numBytes) / MAX_BLOB_SIZE));
                        
                        LOG_INFO("SendNVStorageWriteEntry.Sending",
                                 "Tag: %s, NumBlobs %d, maxBlobSize %zu",
                                 EnumToString(tag), numTotalBlobs, MAX_BLOB_SIZE);

                        for (int i=0; i<numTotalBlobs; ++i) {
                          SendNVStorageWriteEntry(tag,
                                                  d.data() + i * MAX_BLOB_SIZE, MIN(MAX_BLOB_SIZE, numBytes - (i*MAX_BLOB_SIZE)),
                                                  i, numTotalBlobs);
                        }
                      } else {
                        printf("%s open failed\n", inFile);
                      }
                    } else {
                      LOG_INFO("SendNVStorageEraseEntry", "%s", EnumToString(tag));
                      SendNVStorageEraseEntry(tag);
                    }
                    writeNotErase = !writeNotErase;
                  } else {
                    LOG_INFO("SendNVStorageWriteEntry.Disabled",
                             "Set ENABLE_NVSTORAGE_WRITE to 1 if you really want to do this! (Tag: %s)",
                             EnumToString(tag));
                  }
                  
                }
                break;
              }
              case (s32)')':
              {
                LOG_INFO("RetrievingAllMfgTestData", "...");
                
                // Get all Mfg test images and results
                if(altKeyPressed) {
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibImage1);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibImage2);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibImage3);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibImage4);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibImage5);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibImage6);
                  
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_ToolCodeImageLeft);
                  SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_ToolCodeImageRight);
                }
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_PlaypenTestResults);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_IMUInfo);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CameraCalib);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibMetaInfo);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_CalibPose);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_ToolCodeInfo);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_ObservedCubePose);
                SendNVStorageReadEntry(NVStorage::NVEntryTag::NVEntry_BirthCertificate);
                
                // Start log
                _factoryTestLogger.StartLog("", true);
                break;
              }
              case (s32)'*':
              {
                using namespace ExternalInterface;
                using Param = ProceduralEyeParameter;
                DisplayProceduralFace msg;
                msg.leftEye.resize(static_cast<size_t>(Param::NumParameters),  0);
                msg.rightEye.resize(static_cast<size_t>(Param::NumParameters), 0);
                
                if(altKeyPressed) {
                  // Reset to base face
                  msg.leftEye[static_cast<s32>(Param::EyeCenterX)]  = 32;
                  msg.leftEye[static_cast<s32>(Param::EyeCenterY)]  = 32;
                  msg.rightEye[static_cast<s32>(Param::EyeCenterX)] = 96;
                  msg.rightEye[static_cast<s32>(Param::EyeCenterY)] = 32;
                  
                  msg.leftEye[static_cast<s32>(Param::EyeScaleX)] = 1.f;
                  msg.leftEye[static_cast<s32>(Param::EyeScaleY)] = 1.f;
                  msg.rightEye[static_cast<s32>(Param::EyeScaleX)] = 1.f;
                  msg.rightEye[static_cast<s32>(Param::EyeScaleY)] = 1.f;
                  
                  for(auto radiusParam : {Param::UpperInnerRadiusX, Param::UpperInnerRadiusY,
                    Param::UpperOuterRadiusX, Param::UpperOuterRadiusY,
                    Param::LowerInnerRadiusX, Param::LowerInnerRadiusY,
                    Param::LowerOuterRadiusX, Param::LowerOuterRadiusY})
                  {
                    const s32 radiusIndex = static_cast<s32>(radiusParam);
                    msg.leftEye[radiusIndex]  = 0.25f;
                    msg.rightEye[radiusIndex] = 0.25f;
                  }
                  
                  msg.faceAngle_deg = 0;
                  msg.faceScaleX = 1.f;
                  msg.faceScaleY = 1.f;
                  msg.faceCenX  = 0;
                  msg.faceCenY  = 0;
                  
                } else {
                  // Send a random procedural face
                  // NOTE: idle animatino should be set to _LIVE_ or _ANIM_TOOL_ first.
                  Util::RandomGenerator rng;
                  
                  msg.leftEye[static_cast<s32>(Param::UpperInnerRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::UpperInnerRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::LowerInnerRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::LowerInnerRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::UpperOuterRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::UpperOuterRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::LowerOuterRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::LowerOuterRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.leftEye[static_cast<s32>(Param::EyeCenterX)]    = rng.RandIntInRange(24,40);
                  msg.leftEye[static_cast<s32>(Param::EyeCenterY)]    = rng.RandIntInRange(28,36);
                  msg.leftEye[static_cast<s32>(Param::EyeScaleX)]     = 1.f;
                  msg.leftEye[static_cast<s32>(Param::EyeScaleY)]     = 1.f;
                  msg.leftEye[static_cast<s32>(Param::EyeAngle)]      = 0;//rng.RandIntInRange(-10,10);
                  msg.leftEye[static_cast<s32>(Param::LowerLidY)]     = rng.RandDblInRange(0., .25);
                  msg.leftEye[static_cast<s32>(Param::LowerLidAngle)] = rng.RandIntInRange(-20, 20);
                  msg.leftEye[static_cast<s32>(Param::LowerLidBend)]  = 0;//rng.RandDblInRange(0, 0.2);
                  msg.leftEye[static_cast<s32>(Param::UpperLidY)]     = rng.RandDblInRange(0., .25);
                  msg.leftEye[static_cast<s32>(Param::UpperLidAngle)] = rng.RandIntInRange(-20, 20);
                  msg.leftEye[static_cast<s32>(Param::UpperLidBend)]  = 0;//rng.RandDblInRange(0, 0.2);
                  
                  msg.rightEye[static_cast<s32>(Param::UpperInnerRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::UpperInnerRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::LowerInnerRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::LowerInnerRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::UpperOuterRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::UpperOuterRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::LowerOuterRadiusX)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::LowerOuterRadiusY)]   = rng.RandDblInRange(0., 1.);
                  msg.rightEye[static_cast<s32>(Param::EyeCenterX)]    = rng.RandIntInRange(88,104);
                  msg.rightEye[static_cast<s32>(Param::EyeCenterY)]    = rng.RandIntInRange(28,36);
                  msg.rightEye[static_cast<s32>(Param::EyeScaleX)]     = rng.RandDblInRange(0.8, 1.2);
                  msg.rightEye[static_cast<s32>(Param::EyeScaleY)]     = rng.RandDblInRange(0.8, 1.2);
                  msg.rightEye[static_cast<s32>(Param::EyeAngle)]      = 0;//rng.RandIntInRange(-15,15);
                  msg.rightEye[static_cast<s32>(Param::LowerLidY)]     = rng.RandDblInRange(0., .25);
                  msg.rightEye[static_cast<s32>(Param::LowerLidAngle)] = rng.RandIntInRange(-20, 20);
                  msg.rightEye[static_cast<s32>(Param::LowerLidBend)]  = rng.RandDblInRange(0., 0.2);
                  msg.rightEye[static_cast<s32>(Param::UpperLidY)]     = rng.RandDblInRange(0., .25);
                  msg.rightEye[static_cast<s32>(Param::UpperLidAngle)] = rng.RandIntInRange(-20, 20);
                  msg.rightEye[static_cast<s32>(Param::UpperLidBend)]  = rng.RandDblInRange(0, 0.2);
                  
                  msg.faceAngle_deg = 0; //rng.RandIntInRange(-10, 10);
                  msg.faceScaleX = 1.f;//rng.RandDblInRange(0.9, 1.1);
                  msg.faceScaleY = 1.f;//rng.RandDblInRange(0.9, 1.1);
                  msg.faceCenX  = 0; //rng.RandIntInRange(-5, 5);
                  msg.faceCenY  = 0; //rng.RandIntInRange(-5, 5);
                }
                
                SendMessage(MessageGameToEngine(std::move(msg)));

                break;
              }
              case (s32)'^':
              {
                if(altKeyPressed)
                {
                  webots::Field* idleAnimToSendField = root_->getField("idleAnimationName");
                  if(idleAnimToSendField == nullptr) {
                    printf("ERROR: No idleAnimationName field found in WebotsKeyboardController.proto\n");
                    break;
                  }
                  std::string idleAnimToSendName = idleAnimToSendField->getSFString();
                  
                  using namespace ExternalInterface;
                  if(idleAnimToSendName.empty()) {
                    SendMessage(MessageGameToEngine(PopIdleAnimation()));
                  } else {
                    SendMessage(MessageGameToEngine(PushIdleAnimation(AnimationTriggerFromString(idleAnimToSendName.c_str()))));
                  }

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
                  
                  SendAnimation(animToSendName.c_str(), animNumLoops, true);
                }
                break;
              }
              case (s32)'~':
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
                SendAnimationGroup(animToSendName.c_str(), true);
                break;
              }
              
              case (s32)'?':
              case (s32)'/':
              {
                PrintHelp();
                break;
              }
                
              case (s32)']':
              {
                // Set console variable
                webots::Field* field = root_->getField("consoleVarName");
                if(nullptr == field) {
                  printf("No consoleVarName field\n");
                } else {
                  ExternalInterface::SetDebugConsoleVarMessage msg;
                  msg.varName = field->getSFString();
                  if(msg.varName.empty()) {
                    printf("Empty consoleVarName\n");
                  } else {
                    field = root_->getField("consoleVarValue");
                    if(nullptr == field) {
                      printf("No consoleVarValue field\n");
                    } else {
                      msg.tryValue = field->getSFString();
                      printf("Trying to set console var '%s' to '%s'\n",
                             msg.varName.c_str(), msg.tryValue.c_str());
                      SendMessage(ExternalInterface::MessageGameToEngine(std::move(msg)));
                    }
                  }
                }
                break;
              }
                
              // FIXME: Remove after animation iteration - JMR
              case (s32)'+':
              {
                // Send whatever animation is specified in the animationToSendName field
                webots::Field* animToSendNameField = root_->getField("DEV_AnimationName");
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
                
                SendDevAnimation(animToSendName.c_str(), animNumLoops);
                break;
              }
                
              case (s32)'F':
              {
                if (shiftKeyPressed && !altKeyPressed) {
                  // SHIFT+F: Associate name with current face
                  webots::Field* userNameField = root_->getField("userName");
                  webots::Field* enrollToIDField = root_->getField("enrollToID");
                  
                  if(nullptr != userNameField)
                  {
                    std::string userName = userNameField->getSFString();
                    if(!userName.empty())
                    {
                      if(nullptr == enrollToIDField)
                      {
                        printf("No 'enrollToID' field!");
                        break;
                      }
                      
                      const s32 enrollToID = enrollToIDField->getSFInt32();
                      
//                      printf("Assigning name '%s' to ID %d\n", userName.c_str(), GetLastObservedFaceID());
//                      ExternalInterface::AssignNameToFace assignNameToFace;
//                      assignNameToFace.faceID = GetLastObservedFaceID();
//                      assignNameToFace.name   = userName;
//                      SendMessage(ExternalInterface::MessageGameToEngine(std::move(assignNameToFace)));

                      webots::Field* saveFaceField = root_->getField("saveFaceToRobot");
                      if( saveFaceField == nullptr ) {
                        PRINT_NAMED_ERROR("WebotsKeyboardController.MissingField",
                                          "missing saveFaceToRobot field");
                        break;
                      }

                      using namespace ExternalInterface;
                      
                      // Set face enrollment settings
                      bool saveFaceToRobot = saveFaceField->getSFBool();
                      
                      const bool sayName = true;
                      const bool useMusic = false;
                      const s32 observedID = Vision::UnknownFaceID; // GetLastObservedFaceID();
                      printf("Enrolling face ID %d with name '%s'\n", observedID, userName.c_str());
                      SetFaceToEnroll setFaceToEnroll(userName, observedID, enrollToID, saveFaceToRobot, sayName, useMusic);
                      SendMessage(MessageGameToEngine(std::move(setFaceToEnroll)));
                      
                      // Enable selection chooser and specify EnrollFace now that settings are sent
                      SendMessage(MessageGameToEngine(ActivateBehaviorChooser(BehaviorChooserType::Selection)));
                      SendMessage(MessageGameToEngine(ExecuteBehaviorByName("EnrollFace")));
                    }
                    
                  } else {
                    printf("No 'userName' field\n");
                  }
                  
                } else if(altKeyPressed && !shiftKeyPressed) {
                  // ALT+F: Turn to face the pose of the last observed face:
                  printf("Turning to last face\n");
                  ExternalInterface::TurnTowardsLastFacePose turnTowardsPose; // construct w/ defaults for speed
                  turnTowardsPose.panTolerance_rad = DEG_TO_RAD(10);
                  turnTowardsPose.maxTurnAngle_rad = M_PI;
                  turnTowardsPose.robotID = 1;
                  turnTowardsPose.sayName = true;
                  SendMessage(ExternalInterface::MessageGameToEngine(std::move(turnTowardsPose)));
                } else if(altKeyPressed && shiftKeyPressed) {
                  // SHIFT+ALT+F: Erase current face
                  using namespace ExternalInterface;
                  SendMessage(MessageGameToEngine(EraseEnrolledFaceByID(GetLastObservedFaceID())));
                } else {
                  // Just F: Toggle face detection
                  static bool isFaceDetectionEnabled = true;
                  isFaceDetectionEnabled = !isFaceDetectionEnabled;
                  SendEnableVisionMode(VisionMode::DetectingFaces, isFaceDetectionEnabled);
                }
                break;
              }
                
              case (s32)'J':
              {

                // unused!

                break;
              }

              case (s32)'N':
              {
                if( altKeyPressed ) {
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::DenyGameStart()));
                }
                else {
                  webots::Field* unlockNameField = root_->getField("unlockName");
                  if (unlockNameField == nullptr) {
                    printf("ERROR: No unlockNameField field found in WebotsKeyboardController.proto\n");
                    break;
                  }
                
                  std::string unlockName = unlockNameField->getSFString();
                  if (unlockName.empty()) {
                    printf("ERROR: unlockName field is empty\n");
                    break;
                  }

                  UnlockId unlock = UnlockIdsFromString(unlockName.c_str());
                  bool val = !shiftKeyPressed;
                  SendMessage( ExternalInterface::MessageGameToEngine(
                                 ExternalInterface::RequestSetUnlock(unlock, val)));

                }
                break;
              }
                
              case (s32)':':
              {
                
                SendRollActionParams(root_->getField("rollLiftHeight_mm")->getSFFloat(),
                                     root_->getField("rollDriveSpeed_mmps")->getSFFloat(),
                                     root_->getField("rollDriveAccel_mmps2")->getSFFloat(),
                                     root_->getField("rollDriveDuration_ms")->getSFInt32(),
                                     root_->getField("rollBackupDist_mm")->getSFFloat());
                break;
              }
                
              case (s32)';':
              {
                // Toggle enabling of reactionary behaviors
                static bool enable = false;
                printf("Enable reactionary behaviors: %d\n", enable);
                ExternalInterface::EnableAllReactionTriggers m;
                m.enabled = enable;
                m.enableID = "webots";
                ExternalInterface::MessageGameToEngine message;
                message.Set_EnableAllReactionTriggers(m);
                SendMessage(message);
                
                enable = !enable;
                break;
              }
                
              case (s32)'"':
              {
                webots::Field* sayStringField = root_->getField("sayString");
                if(sayStringField == nullptr) {
                  printf("ERROR: No sayString field found in WebotsKeyboardController.proto\n");
                  break;
                }
                
                ExternalInterface::SayText sayTextMsg;
                sayTextMsg.text = sayStringField->getSFString();
                if(sayTextMsg.text.empty()) {
                  printf("ERROR: sayString field is empty\n");
                }
                // TODO: Add ability to set action style, voice style, duration scalar and pitch from KB controller
                sayTextMsg.voiceStyle = SayTextVoiceStyle::CozmoProcessing_Sentence;
                sayTextMsg.durationScalar = 2.f;
                sayTextMsg.voicePitch = 0.f;
                sayTextMsg.playEvent = AnimationTrigger::Count;
                                
                printf("Saying '%s' in voice style '%s' w/ duration scalar %f\n",
                       sayTextMsg.text.c_str(),
                       EnumToString(sayTextMsg.voiceStyle),
                       sayTextMsg.durationScalar);
                SendMessage(ExternalInterface::MessageGameToEngine(std::move(sayTextMsg)));
                break;
              }
              
              case (s32)'Y':
              {
                if(altKeyPressed && shiftKeyPressed)
                {
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::RestoreRobotFromBackup()));
                }
                else if(altKeyPressed)
                {
                  SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::WipeRobotGameData()));
                }
                else
                {
                  ExternalInterface::FlipBlock m;
                  m.objectID = -1;
                  m.motionProf = pathMotionProfile_;
                  ExternalInterface::MessageGameToEngine message;
                  message.Set_FlipBlock(m);
                  SendMessage(message);
                }
                break;
              }
                
              case (s32)'_':
              {
                ExternalInterface::SetCameraSettings settings;
                settings.exposure_ms = root_->getField("exposure_ms")->getSFFloat();
                settings.gain = root_->getField("gain")->getSFFloat();
                settings.enableAutoExposure = root_->getField("enableAutoExposure")->getSFBool();
                ExternalInterface::MessageGameToEngine message;
                message.Set_SetCameraSettings(settings);
                SendMessage(message);
                break;
              }
              
              case (s32)'-':
              {
                if(altKeyPressed)
                {
                  ExternalInterface::PlayCubeAnim s;
                  s.trigger = CubeAnimationTrigger::WakeUp;
                  s.objectID = 1;
                  ExternalInterface::MessageGameToEngine m;
                  m.Set_PlayCubeAnim(s);
                  SendMessage(m);
                }
                else
                {
                  ExternalInterface::PlayCubeAnim s;
                  s.trigger = CubeAnimationTrigger::Flash;
                  s.objectID = 1;
                  ExternalInterface::MessageGameToEngine m;
                  m.Set_PlayCubeAnim(s);
                  SendMessage(m);
                }
                break;
              }
            
              case 0x04: // Webots carriage return?
              {
                break;
              }
                
              default:
              {
                // Unsupported key: ignore.
                printf("Key '%c' (0x%x) is not bound to an action\n", key, key);
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
          
          SendDriveWheels(leftSpeed, rightSpeed, driveAccel, driveAccel);
          wasMovingWheels_ = true;
        } else if(wasMovingWheels_ && !movingWheels) {
          // If we just stopped moving the wheels:
          SendDriveWheels(0, 0, driveAccel, driveAccel);
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
      
    
    void WebotsKeyboardController::TestLightCube()
      {
        static std::vector<ColorRGBA> colors = {{
          ::Anki::NamedColors::RED, ::Anki::NamedColors::GREEN, ::Anki::NamedColors::BLUE,
          ::Anki::NamedColors::CYAN, ::Anki::NamedColors::ORANGE, ::Anki::NamedColors::YELLOW
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
          msg.objectID = GetLastObservedObject().id;
          msg.robotID = 1;
          msg.onPeriod_ms = 100;
          msg.offPeriod_ms = 100;
          msg.transitionOnPeriod_ms = 50;
          msg.transitionOffPeriod_ms = 50;
          msg.turnOffUnspecifiedLEDs = 1;
          msg.onColor = *colorIter;
          msg.offColor = 0;
          msg.whichLEDs = *ledIter;
          msg.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
          
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
        // Get poseMarker pose
        const double* trans = root_->getPosition();
        const double* rot = root_->getOrientation();
        poseMarkerPose_.SetTranslation({1000*static_cast<f32>(trans[0]),
                                        1000*static_cast<f32>(trans[1]),
                                        1000*static_cast<f32>(trans[2])});
        poseMarkerPose_.SetRotation({static_cast<f32>(rot[0]), static_cast<f32>(rot[1]), static_cast<f32>(rot[2]),
                                     static_cast<f32>(rot[3]), static_cast<f32>(rot[4]), static_cast<f32>(rot[5]),
                                     static_cast<f32>(rot[6]), static_cast<f32>(rot[7]), static_cast<f32>(rot[8])} );
        
        // Update pose marker if different from last time
        if (!(prevPoseMarkerPose_ == poseMarkerPose_)) {
          if (poseMarkerMode_ != 0) {
            // Place object mode
            SendDrawPoseMarker(poseMarkerPose_);
          }
        }

        
        ProcessKeystroke();

        if( _shouldQuit ) {
          return 1;
        }
        else {        
          return 0;
        }
      }
    
      void WebotsKeyboardController::HandleRobotConnected(ExternalInterface::RobotConnectionResponse const &msg)
      {
        // Things to do on robot connect
        SendSetRobotVolume(0);
      }
   
    
      void WebotsKeyboardController::HandleNVStorageOpResult(const ExternalInterface::NVStorageOpResult &msg)
      {
        if (msg.op != NVStorage::NVOperation::NVOP_READ ||
            msg.result == NVStorage::NVResult::NV_MORE) {
          // Do nothing for write/erase acks or in-progress reads
          return;
        }

        // Check result flag
        if (msg.result != NVStorage::NVResult::NV_OKAY) {
          PRINT_NAMED_WARNING("HandleNVStorageOpResult.Read.Failed",
                              "tag: %s, res: %s",
                              EnumToString(msg.tag),
                              EnumToString(msg.result));
          return;
        }
        
        const std::vector<u8>* recvdData = GetReceivedNVStorageData(msg.tag);
        if (recvdData == nullptr) {
          LOG_INFO("HandleNVStorageOpResult.Read.NoDataReceived", "Tag: %s", EnumToString(msg.tag));
          return;
        }
        
        switch(msg.tag) {
          case NVStorage::NVEntryTag::NVEntry_IMUInfo:
          {
            IMUInfo info;
            if (recvdData->size() != MakeWordAligned(info.Size())) {
              LOG_INFO("HandleNVStorageOpResult.IMUInfo.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(info.Size()), recvdData->size());
              break;
            }
            info.Unpack(recvdData->data(), info.Size());
            
            _factoryTestLogger.Append(info);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_CameraCalib:
          {
            CameraCalibration calib;
            if (recvdData->size() != MakeWordAligned(calib.Size())) {
              LOG_INFO("HandleNVStorageOpResult.CamCalibration.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(calib.Size()), recvdData->size());
              break;
            }
            calib.Unpack(recvdData->data(), calib.Size());
            
            _factoryTestLogger.Append(calib);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_CalibMetaInfo:
          {
            CalibMetaInfo info;
            if (recvdData->size() != MakeWordAligned(info.Size())) {
              LOG_INFO("HandleNVStorageOpResult.CalibMetaInfo.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(info.Size()), recvdData->size());
              break;
            }
            info.Unpack(recvdData->data(), info.Size());
            
            _factoryTestLogger.Append(info);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_ToolCodeInfo:
          {
            ToolCodeInfo info;
            if (recvdData->size() != MakeWordAligned(info.Size())) {
              LOG_INFO("HandleNVStorageOpResult.ToolCodeInfo.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(info.Size()), recvdData->size());
              break;
            }
            info.Unpack(recvdData->data(), info.Size());
            
            _factoryTestLogger.Append(info);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_CalibPose:
          {
            PoseData info;
            if (recvdData->size() != MakeWordAligned(info.Size())) {
              LOG_INFO("HandleNVStorageOpResult.CalibPose.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(info.Size()), recvdData->size());
              break;
            }
            info.Unpack(recvdData->data(), info.Size());

            _factoryTestLogger.AppendCalibPose(info);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_ObservedCubePose:
          {
            PoseData info;
            if (recvdData->size() != MakeWordAligned(info.Size())) {
              LOG_INFO("HandleNVStorageOpResult.ObservedCubePose.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(info.Size()), recvdData->size());
              break;
            }
            info.Unpack(recvdData->data(), info.Size());
            
            _factoryTestLogger.AppendObservedCubePose(info);
            
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_PlaypenTestResults:
          {
            FactoryTestResultEntry result;
            if (recvdData->size() != MakeWordAligned(result.Size())) {
              LOG_INFO("HandleNVStorageOpResult.PlaypenTestResults.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(result.Size()), recvdData->size());
              break;
            }
            result.Unpack(recvdData->data(), result.Size());
            
            _factoryTestLogger.Append(result);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_BirthCertificate:
          {
            BirthCertificate result;
            if (recvdData->size() != MakeWordAligned(result.Size())) {
              LOG_INFO("HandleNVStorageOpResult.BirthCertificate.UnexpectedSize",
                       "Expected %zu, got %zu", MakeWordAligned(result.Size()), recvdData->size());
              break;
            }
            result.Unpack(recvdData->data(), result.Size());
            
            _factoryTestLogger.Append(result);
            
            if (_factoryTestLogger.IsOpen()) {
              _factoryTestLogger.CloseLog();
            }
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_CalibImage1:
          case NVStorage::NVEntryTag::NVEntry_CalibImage2:
          case NVStorage::NVEntryTag::NVEntry_CalibImage3:
          case NVStorage::NVEntryTag::NVEntry_CalibImage4:
          case NVStorage::NVEntryTag::NVEntry_CalibImage5:
          case NVStorage::NVEntryTag::NVEntry_CalibImage6:
          case NVStorage::NVEntryTag::NVEntry_ToolCodeImageLeft:
          case NVStorage::NVEntryTag::NVEntry_ToolCodeImageRight:
          {
            char outFile[128];
            sprintf(outFile,  "nvstorage_output_%s.jpg", EnumToString(msg.tag));
            _factoryTestLogger.AddFile(outFile, *recvdData);
            
            break;
          }
          case NVStorage::NVEntryTag::NVEntry_IMUAverages:
          {
            LOG_INFO("IMUAveragesData", "size: %lu", recvdData->size());
            PrintBytesHex((char*)(recvdData->data()), (int)recvdData->size());
            
            break;
          }
          default:
            PRINT_NAMED_DEBUG("HandleNVStorageOpResult.UnhandledTag", "%s (size: %zu)", EnumToString(msg.tag), recvdData->size());
            for(auto data : *recvdData)
            {
              printf("%d ", data);
            }
            printf("\n");
            break;
        }
      }
    

  } // namespace Cozmo
} // namespace Anki


// =======================================================================

int main(int argc, char **argv)
{
  using namespace Anki;
  using namespace Anki::Cozmo;

  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  // create platform
  const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlKeyboard");
  // initialize logger
  WebotsCtrlShared::DefaultAutoGlobalLogger autoLogger(dataPlatform, params.filterLog);

  Anki::Cozmo::WebotsKeyboardController webotsCtrlKeyboard(BS_TIME_STEP);
  webotsCtrlKeyboard.PreInit();
  webotsCtrlKeyboard.WaitOnKeyboardToConnect();
  
  webotsCtrlKeyboard.Init();
  while (webotsCtrlKeyboard.Update() == 0)
  {
  }

  return 0;
}
