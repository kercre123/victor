/*
 * File:          webots_keyboard_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */


#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/basestation/ui/messaging/uiMessages.h"
#include "anki/messaging/shared/TcpClient.h"

#include "behaviorManager.h"

#include <stdio.h>

// Webots includes
#include <webots/Supervisor.hpp>


#define BASESTATION_IP "127.0.0.1"

namespace Anki {
  namespace Cozmo {
    namespace WebotsKeyboardController {
      
      webots::Supervisor inputController;
      
      // Private memers:
      namespace {
        // Constants for Webots:
        const f32 DRIVE_VELOCITY_FAST = 60.f; // mm/s
        const f32 DRIVE_VELOCITY_SLOW = 10.f; // mm/s
        
        const f32 LIFT_SPEED_RAD_PER_SEC = 2.f;
        const f32 LIFT_ACCEL_RAD_PER_SEC2 = 10.f;
        
        const f32 HEAD_SPEED_RAD_PER_SEC = 4.f;
        const f32 HEAD_ACCEL_RAD_PER_SEC2 = 10.f;
        
        
        int lastKeyPressed_ = 0;
        int lastKeyAndModPressed_ = 0;
        
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
        
        BehaviorMode behaviorMode_ = BM_None;
        
        // Socket connection to basestation
        TcpClient bsClient;
        char sendBuf[128];
        
      } // private namespace
      
      // Forward declarations
      void ProcessKeystroke(void);
      void PrintHelp(void);
      void SendMessage(const UiMessage& msg);
      void SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps);
      void SendMoveHead(const f32 speed_rad_per_sec);
      void SendMoveLift(const f32 speed_rad_per_sec);
      void SendMoveHeadToAngle(const f32 rad, const f32 speed, const f32 accel);
      void SendMoveLiftToHeight(const f32 mm, const f32 speed, const f32 accel);
      void SendStopAllMotors();
      void SendImageRequest(u8 mode);
      void SendSaveImages(bool on);
      void SendEnableDisplay(bool on);
      void SendSetHeadlights(u8 intensity);
      void SendExecutePathToPose(const Pose3d& p);
      void SendExecutePlaceBlockOnGroundSequence(const Pose3d& p);
      void SendExecuteTestPlan();
      void SendClearAllBlocks();
      void SendSelectNextBlock();
      void SendExecuteBehavior(BehaviorMode mode);
      void SendAbortPath();
      void SendDrawPoseMarker(const Pose3d& p);
      void SendErasePoseMarker();
      void SendHeadControllerGains(const f32 kp, const f32 ki, const f32 maxErrorSum);
      void SendLiftControllerGains(const f32 kp, const f32 ki, const f32 maxErrorSum);
      void SendSelectNextSoundScheme();
      void SendStartTestMode(TestMode mode);
      
      void Init()
      {
        memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
        
        inputController.keyboardEnable(BS_TIME_STEP);
        
        gps_ = inputController.getGPS("gps");
        compass_ = inputController.getCompass("compass");
        
        gps_->enable(BS_TIME_STEP);
        compass_->enable(BS_TIME_STEP);
        
        // Make root point to BlockWorldComms node
        webots::Field* rootChildren = inputController.getRoot()->getField("children");
        int numRootChildren = rootChildren->getCount();
        for (int n = 0 ; n<numRootChildren; ++n) {
          webots::Node* nd = rootChildren->getMFNode(n);
          
          // Get the node name
          std::string nodeName = "";
          webots::Field* nameField = nd->getField("name");
          if (nameField) {
            nodeName = nameField->getSFString();
          }
          
          if (nd->getTypeName().find("Supervisor") != std::string::npos &&
              nodeName.find("WebotsKeyboardController") != std::string::npos) {
            root_ = nd;
            
            // Find pose marker color field
            poseMarkerDiffuseColor_ = nd->getField("poseMarkerDiffuseColor");
          }
        }
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
        printf("                   Request image:  i\n");
        printf("             Toggle image stream:  Shift+i\n");
        printf("              Toggle save images:  e\n");
        printf("        Toggle VizObject display:  d\n");
        printf("Goto/place object at pose marker:  g\n");
        printf("         Toggle pose marker mode:  Shift+g\n");
        printf("              Cycle block select:  .\n");
        printf("              Clear known blocks:  c\n");
        printf("          Dock to selected block:  p\n");
        printf("    Travel up/down selected ramp:  r\n");
        printf("       Start June 2014 dice demo:  j\n");
        printf("              Abort current path:  q\n");
        printf("         Update controller gains:  k\n");
        printf("             Cycle sound schemes:  m\n");
        printf("                      Test modes:  Alt + Testmode#\n");
        printf("                Follow test plan:  t\n");
        printf("                      Print help:  ?\n");
        printf("\n");
      }
      
      
      //Check the keyboard keys and issue robot commands
      void ProcessKeystroke()
      {
        
        // these are the ascii codes for the capital letter
        //Numbers, spacebar etc. work, letters are different, why?
        //a, z, s, x, Space
        const s32 CKEY_CANCEL_PATH = 81;  // q
        const s32 CKEY_LIFT_UP     = 65;  // a
        const s32 CKEY_LIFT_DOWN   = 90;  // z
        //const s32 CKEY_HEAD_UPUP  = 87;  // w
        const s32 CKEY_HEAD_UP     = 83;  // s
        const s32 CKEY_HEAD_DOWN   = 88;  // x
        const s32 CKEY_UNLOCK      = 32;  // space
        const s32 CKEY_REQUEST_IMG = 73;  // i
        const s32 CKEY_DISPLAY_TOGGLE = 68;  // d
        const s32 CKEY_HEADLIGHT   = 72;  // h
        const s32 CKEY_GOTO_POSE   = 71;  // g
        const s32 CKEY_CLEAR_BLOCKS = 67; // c
        //const s32 CKEY_COMMA   = 44;  // ,
        const s32 CKEY_CYCLE_BLOCK_SELECT   = 46;  // .
        //const s32 CKEY_FWDSLASH    = 47; // '/'
        //const s32 CKEY_BACKSLASH   = 92 // '\'
        const s32 CKEY_DOCK_TO_BLOCK  = 80;  // p
        const s32 CKEY_USE_RAMP = 82; // r
        const s32 CKEY_QUESTION_MARK  = 63; // '/'
        
        const s32 CKEY_START_DICE_DEMO= 74; // 'j' for "June"
        const s32 CKEY_SET_GAINS   = 75;  // 'k'
        const s32 CKEY_SET_VISIONSYSTEM_PARAMS = 86;  // v
        
        const s32 CKEY_TEST_PLAN = (s32)'T';
        const s32 CKEY_CYCLE_SOUND_SCHEME = (s32)'M';
        const s32 CKEY_EXPORT_IMAGES = (s32)'E';
        
        
        int key = inputController.keyboardGetKey();
        
        static bool keyboardRestart = false;
        if (keyboardRestart) {
          inputController.keyboardDisable();
          inputController.keyboardEnable(BS_TIME_STEP);
          keyboardRestart = false;
        }
        
        
        // Skip if same key as before
        if (key == lastKeyAndModPressed_)
          return;
        
        // Extract modifier key
        int modifier_key = key & ~webots::Supervisor::KEYBOARD_KEY;
        
        // Adjust wheel speed appropriately
        f32 wheelSpeed = DRIVE_VELOCITY_FAST;
        f32 liftSpeed = LIFT_SPEED_RAD_PER_SEC;
        f32 headSpeed = HEAD_SPEED_RAD_PER_SEC;
        if (modifier_key == webots::Supervisor::KEYBOARD_SHIFT) {
          wheelSpeed = DRIVE_VELOCITY_SLOW;
          liftSpeed *= 0.25;
          headSpeed *= 0.25;
        }
        
        // Set key to its modifier-less self
        key &= webots::Supervisor::KEYBOARD_KEY;
        
        
        //printf("keypressed: %d, modifier %d, orig_key %d, prev_key %d\n",
        //       key, modifier_key, key | modifier_key, lastKeyPressed_);
        
        while(1) {
          
          // Check for test mode (alt + key)
          if (modifier_key == webots::Supervisor::KEYBOARD_ALT) {
            if (key >= '0' && key <= '9') {
              TestMode m = TestMode(key - '0');
              printf("Sending test mode %d\n", m);
              SendStartTestMode(m);
              break;
            }
          }
          
          // Check for (mostly) single key commands
          switch (key)
          {
            case webots::Robot::KEYBOARD_UP:
            {
              SendDriveWheels(wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_DOWN:
            {
              SendDriveWheels(-wheelSpeed, -wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_LEFT:
            {
              SendDriveWheels(-wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_RIGHT:
            {
              SendDriveWheels(wheelSpeed, -wheelSpeed);
              break;
            }
              
            case CKEY_HEAD_UP: //s-key: move head UP
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.5 * HEAD_SPEED_RAD_PER_SEC : HEAD_SPEED_RAD_PER_SEC);
              SendMoveHead(speed);
              break;
            }
              
            case CKEY_HEAD_DOWN: //x-key: move head DOWN
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.5 * HEAD_SPEED_RAD_PER_SEC : HEAD_SPEED_RAD_PER_SEC);
              SendMoveHead(-speed);
              break;
            }
              
            case CKEY_LIFT_UP: //a-key: move lift up
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.25 * LIFT_SPEED_RAD_PER_SEC : 0.5 * LIFT_SPEED_RAD_PER_SEC);
              SendMoveLift(speed);
              break;
            }
              
            case CKEY_LIFT_DOWN: //z-key: move lift down
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.25 * LIFT_SPEED_RAD_PER_SEC : 0.5 * LIFT_SPEED_RAD_PER_SEC);
              SendMoveLift(-speed);
              break;
            }
              
            case '1': //set lift to low dock height
            {
              SendMoveLiftToHeight(LIFT_HEIGHT_LOWDOCK, liftSpeed, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '2': //set lift to high dock height
            {
              SendMoveLiftToHeight(LIFT_HEIGHT_HIGHDOCK, liftSpeed, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '3': //set lift to carry height
            {
              SendMoveLiftToHeight(LIFT_HEIGHT_CARRY, liftSpeed, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '4': //set head to look all the way down
            {
              SendMoveHeadToAngle(MIN_HEAD_ANGLE, headSpeed, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '5': //set head to straight ahead
            {
              SendMoveHeadToAngle(0, headSpeed, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '6': //set head to look all the way up
            {
              SendMoveHeadToAngle(MAX_HEAD_ANGLE, headSpeed, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case CKEY_UNLOCK: // Stop all motors
            {
              SendStopAllMotors();
              break;
            }
              
            case CKEY_REQUEST_IMG:
            {
              ImageSendMode_t mode = ISM_SINGLE_SHOT;
              if (modifier_key == webots::Supervisor::KEYBOARD_SHIFT) {
                static bool streamOn = false;
                if (streamOn) {
                  mode = ISM_OFF;
                } else {
                  mode = ISM_STREAM;
                }
                streamOn = !streamOn;
              }
              SendImageRequest(mode);
              break;
            }
              
            case CKEY_EXPORT_IMAGES:
            {
              // Toggle saving of images to pgm
              static bool saveImages = true;
              SendSaveImages(saveImages);
              saveImages = !saveImages;
              break;
            }
              
            case CKEY_DISPLAY_TOGGLE:
            {
              static bool showObjects = false;
              SendEnableDisplay(showObjects);
              showObjects = !showObjects;
              break;
            }
              
            case CKEY_HEADLIGHT:
            {
              static bool headlightsOn = false;
              headlightsOn = !headlightsOn;
              SendSetHeadlights(headlightsOn ? 128 : 0);
              break;
            }
            case CKEY_GOTO_POSE:
            {
              if (modifier_key == webots::Supervisor::KEYBOARD_SHIFT) {
                poseMarkerMode_ = !poseMarkerMode_;
                printf("Pose marker mode: %d\n", poseMarkerMode_);
                poseMarkerDiffuseColor_->setSFColor(poseMarkerColor_[poseMarkerMode_]);
                SendErasePoseMarker();
                break;
              }
              
              if (poseMarkerMode_ == 0) {
                // Execute path to pose
                SendExecutePathToPose(poseMarkerPose_);
                SendMoveHeadToAngle(-0.26, HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
              } else {
                SendExecutePlaceBlockOnGroundSequence(poseMarkerPose_);
                // Make sure head is tilted down so that it can localize well
                SendMoveHeadToAngle(-0.26, HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
                
              }
              break;
            }
              
            case CKEY_TEST_PLAN:
            {
              SendExecuteTestPlan();
              break;
            }
              
            case CKEY_CYCLE_BLOCK_SELECT:
            {
              SendSelectNextBlock();
              break;
            }
            case CKEY_CLEAR_BLOCKS:
            {
              SendClearAllBlocks();
              break;
            }
            case CKEY_DOCK_TO_BLOCK:
            {
              SendExecuteBehavior(BM_PickAndPlace);
              break;
            }
            case CKEY_USE_RAMP:
            {
              SendExecuteBehavior(BM_TraverseObject);
              break;
            }
            case CKEY_START_DICE_DEMO:
            {
              if (behaviorMode_ == BM_June2014DiceDemo) {
                SendExecuteBehavior(BM_None);
              } else {
                SendExecuteBehavior(BM_June2014DiceDemo);
              }
              break;
            }
            case CKEY_CANCEL_PATH:
            {
              SendAbortPath();
              break;
            }
              
            case CKEY_SET_GAINS:
            {
              if (root_) {
                // Head and lift gains
                f32 kp = root_->getField("headKp")->getSFFloat();
                f32 ki = root_->getField("headKi")->getSFFloat();
                f32 maxErrorSum = root_->getField("headMaxErrorSum")->getSFFloat();
                printf("New head gains: %f %f %f\n", kp, ki, maxErrorSum);
                SendHeadControllerGains(kp, ki, maxErrorSum);
                
                kp = root_->getField("liftKp")->getSFFloat();
                ki = root_->getField("liftKi")->getSFFloat();
                maxErrorSum = root_->getField("liftMaxErrorSum")->getSFFloat();
                printf("New lift gains: %f %f %f\n", kp, ki, maxErrorSum);
                SendLiftControllerGains(kp, ki, maxErrorSum);
              } else {
                printf("No WebotsKeyboardController was found in world\n");
              }
              break;
            }
            case CKEY_SET_VISIONSYSTEM_PARAMS:
            {
              /*
               if (root_) {
               // Vision system params
               VisionSystemParams_t p;
               p.integerCountsIncrement = root_->getField("integerCountsIncrement")->getSFInt32();
               p.minExposureTime = root_->getField("minExposureTime")->getSFFloat();
               p.maxExposureTime = root_->getField("maxExposureTime")->getSFFloat();
               p.highValue = root_->getField("highValue")->getSFInt32();
               p.percentileToMakeHigh = root_->getField("percentileToMakeHigh")->getSFFloat();
               printf("New VisionSystems params\n");
               robot_->SendSetVisionSystemParams(p);
               }
               */
              break;
            }
            case CKEY_CYCLE_SOUND_SCHEME:
            {
              SendSelectNextSoundScheme();
              break;
            }
            case CKEY_QUESTION_MARK:
            {
              PrintHelp();
              break;
            }
              
            default:
            {
              // If the last key pressed was a move wheels key, then stop wheels
              if (lastKeyPressed_ == webots::Robot::KEYBOARD_UP   ||
                  lastKeyPressed_ == webots::Robot::KEYBOARD_DOWN ||
                  lastKeyPressed_ == webots::Robot::KEYBOARD_LEFT ||
                  lastKeyPressed_ == webots::Robot::KEYBOARD_RIGHT)
              {
                SendDriveWheels(0, 0);
              }
              
              // If the last key pressed was a move lift key then stop it.
              if (lastKeyPressed_ == CKEY_LIFT_UP || lastKeyPressed_ == CKEY_LIFT_DOWN) {
                SendMoveLift(0);
              }
              
              // If the last key pressed was a move head key then stop it.
              if (lastKeyPressed_ == CKEY_HEAD_UP || lastKeyPressed_ == CKEY_HEAD_DOWN) {
                SendMoveHead(0);
              }
              break;
            }
              
          } // switch
          
          break;
        }  // while(1)
        
        lastKeyPressed_ = key;
        lastKeyAndModPressed_ = key | modifier_key;
        
        
      } // BSKeyboardController::ProcessKeyStroke()
      
      
      void Update()
      {
        // Connect to basestation if not already connected
        if (!bsClient.IsConnected()) {
          if (!bsClient.Connect(BASESTATION_IP, UI_MESSAGE_SERVER_LISTEN_PORT)) {
            printf("Failed to connect to UI message server listen port\n");
            return;
          }
          printf("WebotsKeyboardController connected to basestation!\n");
        }
        
        
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
        poseMarkerPose_.SetRotation(angle, Z_AXIS_3D);
        
        
        // Update pose marker if different from last time
        if (!(prevPoseMarkerPose_ == poseMarkerPose_)) {
          if (poseMarkerMode_ != 0) {
            // Place object mode
            SendDrawPoseMarker(poseMarkerPose_);
          }
        }
        
        ProcessKeystroke();
        
        prevPoseMarkerPose_ = poseMarkerPose_;
      }
      
      void SendMessage(const UiMessage& msg)
      {
        int sendBufLen = sizeof(RADIO_PACKET_HEADER);
        sendBuf[sendBufLen++] = msg.GetSize()+1;
        sendBuf[sendBufLen++] = msg.GetID();
        msg.GetBytes((u8*)(&sendBuf[sendBufLen]));
        sendBufLen += msg.GetSize();
        
        //int bytes_sent =
        bsClient.Send(sendBuf, sendBufLen);
        //printf("Sent %d bytes\n", bytes_sent);
      }
      
      void SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps)
      {
        MessageU2G_DriveWheels m;
        m.lwheel_speed_mmps = lwheel_speed_mmps;
        m.rwheel_speed_mmps = rwheel_speed_mmps;
        SendMessage(m);
      }
      
      void SendMoveHead(const f32 speed_rad_per_sec)
      {
        MessageU2G_MoveHead m;
        m.speed_rad_per_sec = speed_rad_per_sec;
        SendMessage(m);
      }
      
      void SendMoveLift(const f32 speed_rad_per_sec)
      {
        MessageU2G_MoveLift m;
        m.speed_rad_per_sec = speed_rad_per_sec;
        SendMessage(m);
      }
      
      void SendMoveHeadToAngle(const f32 rad, const f32 speed, const f32 accel)
      {
        MessageU2G_SetHeadAngle m;
        m.angle_rad = rad;
        m.max_speed_rad_per_sec = speed;
        m.accel_rad_per_sec2 = accel;
        SendMessage(m);
      }
      
      void SendMoveLiftToHeight(const f32 mm, const f32 speed, const f32 accel)
      {
        MessageU2G_SetLiftHeight m;
        m.height_mm = mm;
        m.max_speed_rad_per_sec = speed;
        m.accel_rad_per_sec2 = accel;
        SendMessage(m);
      }
      
      void SendStopAllMotors()
      {
        MessageU2G_StopAllMotors m;
        SendMessage(m);
      }
      
      void SendImageRequest(u8 mode)
      {
        MessageU2G_ImageRequest m;
        m.mode = mode;
        SendMessage(m);
      }
      
      void SendSaveImages(bool on)
      {
        MessageU2G_SaveImages m;
        m.enableSave = on;
        SendMessage(m);
      }
      
      void SendEnableDisplay(bool on)
      {
        MessageU2G_EnableDisplay m;
        m.enable = on;
        SendMessage(m);
      }
      
      void SendSetHeadlights(u8 intensity)
      {
        MessageU2G_SetHeadlights m;
        m.intensity = intensity;
        SendMessage(m);
      }
      
      void SendExecutePathToPose(const Pose3d& p)
      {
        MessageU2G_GotoPose m;
        m.x_mm = p.GetTranslation().x();
        m.y_mm = p.GetTranslation().y();
        m.rad = p.GetRotationAngle<'Z'>().ToFloat();
        m.level = 0;
        SendMessage(m);
      }
      
      void SendExecutePlaceBlockOnGroundSequence(const Pose3d& p)
      {
        MessageU2G_PlaceBlockOnGround m;
        m.x_mm = p.GetTranslation().x();
        m.y_mm = p.GetTranslation().y();
        m.rad = p.GetRotationAngle<'Z'>().ToFloat();
        m.level = 0;
        SendMessage(m);
      }
      
      void SendExecuteTestPlan()
      {
        MessageU2G_ExecuteTestPlan m;
        SendMessage(m);
      }
      
      void SendClearAllBlocks()
      {
        MessageU2G_ClearAllBlocks m;
        SendMessage(m);
      }
      
      void SendSelectNextBlock()
      {
        MessageU2G_SelectNextBlock m;
        SendMessage(m);
      }
      
      void SendExecuteBehavior(BehaviorMode mode)
      {
        MessageU2G_ExecuteBehavior m;
        m.behaviorMode = mode;
        SendMessage(m);
      }
      
      void SendAbortPath()
      {
        MessageU2G_AbortPath m;
        SendMessage(m);
      }
      
      void SendDrawPoseMarker(const Pose3d& p)
      {
        MessageU2G_DrawPoseMarker m;
        m.x_mm = p.GetTranslation().x();
        m.y_mm = p.GetTranslation().y();
        m.rad = p.GetRotationAngle<'Z'>().ToFloat();
        m.level = 0;
        SendMessage(m);
      }
      
      void SendErasePoseMarker()
      {
        MessageU2G_ErasePoseMarker m;
        SendMessage(m);
      }
      
      void SendHeadControllerGains(const f32 kp, const f32 ki, const f32 maxErrorSum)
      {
        MessageU2G_SetHeadControllerGains m;
        m.kp = kp;
        m.ki = ki;
        m.maxIntegralError = maxErrorSum;
        SendMessage(m);
      }
      
      void SendLiftControllerGains(const f32 kp, const f32 ki, const f32 maxErrorSum)
      {
        MessageU2G_SetLiftControllerGains m;
        m.kp = kp;
        m.ki = ki;
        m.maxIntegralError = maxErrorSum;
        SendMessage(m);
      }
      
      void SendSelectNextSoundScheme()
      {
        MessageU2G_SelectNextSoundScheme m;
        SendMessage(m);
      }
      
      void SendStartTestMode(TestMode mode)
      {
        MessageU2G_StartTestMode m;
        m.mode = mode;
        SendMessage(m);
      }
      
    } // namespace WebotsKeyboardController
  } // namespace Cozmo
} // namespace Anki


// =======================================================================

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  WebotsKeyboardController::inputController.step(BS_TIME_STEP);
  WebotsKeyboardController::Init();

  while (WebotsKeyboardController::inputController.step(BS_TIME_STEP) != -1)
  {
    // Process keyboard input
    WebotsKeyboardController::Update();
  }
  
  return 0;
}

