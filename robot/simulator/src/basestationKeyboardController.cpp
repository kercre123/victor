#include "basestationKeyboardController.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/common/basestation/math/point_impl.h"
#include "vizManager.h"
#include "behaviorManager.h"
#include "soundManager.h"

#include <stdio.h>
#include <string.h>

// Webots includes
#include <webots/Supervisor.hpp>


namespace Anki {
  namespace Cozmo {
    namespace Sim {

      extern webots::Supervisor basestationController;
      
      // Private memers:
      namespace {
        // Constants for Webots:
        const f32 DRIVE_VELOCITY_FAST = 60.f; // mm/s
        const f32 DRIVE_VELOCITY_SLOW = 10.f; // mm/s
        
        const f32 LIFT_SPEED_RAD_PER_SEC = 2.f;
        const f32 LIFT_ACCEL_RAD_PER_SEC2 = 10.f;

        const f32 HEAD_SPEED_RAD_PER_SEC = 4.f;
        const f32 HEAD_ACCEL_RAD_PER_SEC2 = 10.f;
        
        RobotManager* robotMgr_;
        Robot* robot_;
        
        BlockWorld* blockWorld_;
        BehaviorManager* behaviorMgr_;
        
        int lastKeyPressed_ = 0;
        int lastKeyAndModPressed_ = 0;
        
        bool isEnabled_ = false;
        
        webots::GPS* gps_;
        webots::Compass* compass_;
        
        webots::Node* root_ = nullptr;
        
        u8 poseMarkerMode_ = 0;
        Anki::Pose3d poseMarkerPose_;
        webots::Field* poseMarkerDiffuseColor_ = nullptr;
        double poseMarkerColor_[2][3] = { {0.1, 0.8, 0.1} // Goto pose color
                                         ,{0.8, 0.1, 0.1} // Place object color
                                        };
        
      } // private namespace
      
      
      void BSKeyboardController::Init(RobotManager *robotMgr, BlockWorld *blockWorld, BehaviorManager *behaviorMgr)
      {
        robotMgr_ = robotMgr;
        blockWorld_ = blockWorld;
        behaviorMgr_ = behaviorMgr;
        
        gps_ = basestationController.getGPS("gps");
        compass_ = basestationController.getCompass("compass");
        
        gps_->enable(BS_TIME_STEP);
        compass_->enable(BS_TIME_STEP);
        
        // Make root point to BlockWorldComms node
        webots::Field* rootChildren = basestationController.getRoot()->getField("children");
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
              nodeName.find("BlockWorldComms") != std::string::npos) {
            root_ = nd;
            
            // Find pose marker color field
            poseMarkerDiffuseColor_ = nd->getField("poseMarkerDiffuseColor");
          }
        }
      }
      
      void BSKeyboardController::Enable(void)
      {
        basestationController.keyboardEnable(BS_TIME_STEP);
        
        isEnabled_ = true;

        PrintHelp();

      } // Enable()
      
      void BSKeyboardController::Disable(void)
      {
        //Disable the keyboard
        basestationController.keyboardDisable();
        isEnabled_ = false;
      }
      
      bool BSKeyboardController::IsEnabled()
      {
        return isEnabled_;
      }
      
      
      void BSKeyboardController::PrintHelp()
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
        printf("       Start June 2014 dice demo:  j\n");
        printf("              Abort current path:  q\n");
        printf("         Update controller gains:  k\n");
        printf("             Cycle sound schemes:  m\n");
        printf("                      Test modes:  Alt + Testmode#\n");
        printf("                Follow test plan:  t\n");
        printf("                      Print help:  ?\n");
        printf("\n");
      }
      
      
      void BSKeyboardController::Update()
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
        poseMarkerPose_.SetRotation(angle, Z_AXIS_3D);

        
        
        // Update pose marker footprint based on carried object
        if (poseMarkerMode_ == 0) {
          // Goto pose mode
          // ...
        } else {
          // Place object mode
          if (robot_->IsCarryingBlock()) {
            Quad2f blockFootprint = blockWorld_->GetBlockByID(robot_->GetCarryingBlock())->GetBoundingQuadXY(poseMarkerPose_);
            VizManager::getInstance()->DrawPoseMarker(0, blockFootprint, VIZ_COLOR_GREEN);
          }
        }
        
        ProcessKeystroke();
      }
      
      //Check the keyboard keys and issue robot commands
      void BSKeyboardController::ProcessKeystroke()
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
        const s32 CKEY_QUESTION_MARK  = 63; // '/'
        
        const s32 CKEY_START_DICE_DEMO= 74; // 'j' for "June"
        const s32 CKEY_SET_GAINS   = 75;  // 'k'
        const s32 CKEY_SET_VISIONSYSTEM_PARAMS = 86;  // v

        const s32 CKEY_TEST_PLAN = (s32)'T';
        const s32 CKEY_CYCLE_SOUND_SCHEME = (s32)'M';
        const s32 CKEY_EXPORT_IMAGES = (s32)'E';

        // Get robot
        robot_ = NULL;
        if (robotMgr_->GetNumRobots()) {
          robot_ = robotMgr_->GetRobotByID(robotMgr_->GetRobotIDList()[0]);
        } else {
          return;
        }

        if (isEnabled_)
        {
          int key = basestationController.keyboardGetKey();
          
          static bool keyboardRestart = false;
          if (keyboardRestart) {
            basestationController.keyboardDisable();
            basestationController.keyboardEnable(BS_TIME_STEP);
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
              robot_->SendStartTestMode(m);
              break;
            }
          }
          
          // Check for (mostly) single key commands
          switch (key)
          {
            case webots::Robot::KEYBOARD_UP:
            {
              robot_->DriveWheels(wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_DOWN:
            {
              robot_->DriveWheels(-wheelSpeed, -wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_LEFT:
            {
              robot_->DriveWheels(-wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_RIGHT:
            {
              robot_->DriveWheels(wheelSpeed, -wheelSpeed);
              break;
            }
              
            case CKEY_HEAD_UP: //s-key: move head UP
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.5 * HEAD_SPEED_RAD_PER_SEC : HEAD_SPEED_RAD_PER_SEC);
              robot_->MoveHead(speed);
              break;
            }
              
            case CKEY_HEAD_DOWN: //x-key: move head DOWN
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.5 * HEAD_SPEED_RAD_PER_SEC : HEAD_SPEED_RAD_PER_SEC);
              robot_->MoveHead(-speed);
              break;
            }
              
            case CKEY_LIFT_UP: //a-key: move lift up
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.25 * LIFT_SPEED_RAD_PER_SEC : 0.5 * LIFT_SPEED_RAD_PER_SEC);
              robot_->MoveLift(speed);
              break;
            }
              
            case CKEY_LIFT_DOWN: //z-key: move lift down
            {
              const f32 speed((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? 0.25 * LIFT_SPEED_RAD_PER_SEC : 0.5 * LIFT_SPEED_RAD_PER_SEC);
              robot_->MoveLift(-speed);
              break;
            }

            case '1': //set lift to low dock height
            {
              if(robot_->IsCarryingBlock()) {
                // Put the block down right here
                robot_->ExecutePlaceBlockOnGroundSequence();
                
              } else {
                // Just move the lift down
                robot_->MoveLiftToHeight(LIFT_HEIGHT_LOWDOCK, liftSpeed, LIFT_ACCEL_RAD_PER_SEC2);
              }
              break;
            }
              
            case '2': //set lift to high dock height
            {
              robot_->MoveLiftToHeight(LIFT_HEIGHT_HIGHDOCK, liftSpeed, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '3': //set lift to carry height
            {
              robot_->MoveLiftToHeight(LIFT_HEIGHT_CARRY, liftSpeed, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '4': //set head to look all the way down
            {
              robot_->MoveHeadToAngle(MIN_HEAD_ANGLE, headSpeed, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }

            case '5': //set head to straight ahead
            {
              robot_->MoveHeadToAngle(0, headSpeed, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '6': //set head to look all the way up
            {
              robot_->MoveHeadToAngle(MAX_HEAD_ANGLE, headSpeed, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }

            case CKEY_UNLOCK: // Stop all motors
            {
              robot_->StopAllMotors();
              break;
            }
              
            case CKEY_REQUEST_IMG:
            {
              ImageSendMode_t mode = ISM_SINGLE_SHOT;
              if (modifier_key == webots::Supervisor::KEYBOARD_SHIFT) {
                static bool streamOn = false;
                if (streamOn) {
                  mode = ISM_OFF;
                  blockWorld_->EnableDraw(false);
                } else {
                  mode = ISM_STREAM;
                  blockWorld_->EnableDraw(true);
                }
                streamOn = !streamOn;
              }
              robot_->SendImageRequest(mode);
              break;
            }
              
            case CKEY_EXPORT_IMAGES:
            {
              // Toggle saving of images to pgm
              robot_->SaveImages(!robot_->IsSavingImages());
              printf("Saving images: %s\n", robot_->IsSavingImages() ? "ON" : "OFF");
              break;
            }

            case CKEY_DISPLAY_TOGGLE:
            {
              static bool showObjects = false;
              VizManager::getInstance()->ShowObjects(showObjects);
              showObjects = !showObjects;
              break;
            }
              
            case CKEY_HEADLIGHT:
            {
              static bool headlightsOn = false;
              headlightsOn = !headlightsOn;
              robot_->SetHeadlight(headlightsOn ? 128 : 0);
              break;
            }
            case CKEY_GOTO_POSE:
            {
              if (modifier_key == webots::Supervisor::KEYBOARD_SHIFT) {
                poseMarkerMode_ = !poseMarkerMode_;
                printf("Pose marker mode: %d\n", poseMarkerMode_);
                VizManager::getInstance()->EraseAllQuadsWithType(VIZ_QUAD_POSE_MARKER);
                poseMarkerDiffuseColor_->setSFColor(poseMarkerColor_[poseMarkerMode_]);
                break;
              }
              
              if (poseMarkerMode_ == 0) {
                // Execute path to pose
                if (robot_->ExecutePathToPose(poseMarkerPose_) == RESULT_OK) {
                  // Make sure head is tilted down so that it can localize well
                  robot_->MoveHeadToAngle(-0.26, HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
                }
              } else {
                // TODO...
                printf("PLACE OBJECT ON GROUND NOT YET IMPLEMENTED\n");
              }
              break;
            }

            case CKEY_TEST_PLAN:
            {
              robot_->ExecuteTestPath();
              break;
            }

            case CKEY_CYCLE_BLOCK_SELECT:
            {
              behaviorMgr_->SelectNextBlockOfInterest();
              break;
            }
            case CKEY_CLEAR_BLOCKS:
            {
              blockWorld_->ClearAllExistingBlocks();
              VizManager::getInstance()->EraseVizObjectType(VIZ_OBJECT_CUBOID);
              break;
            }
            case CKEY_DOCK_TO_BLOCK:
            {
              behaviorMgr_->StartMode(BM_PickAndPlace);
              break;
            }
            case CKEY_START_DICE_DEMO:
            {
              if (behaviorMgr_->GetMode() == BM_None) {
                behaviorMgr_->StartMode(BM_June2014DiceDemo);
                robot_->SetDefaultLights(0x008080, 0x008080);
              } else {
                behaviorMgr_->StartMode(BM_None);
              }
              break;
            }
            case CKEY_CANCEL_PATH:
            {
              robot_->AbortCurrentPath();
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
                robot_->SendHeadControllerGains(kp, ki, maxErrorSum);
                
                kp = root_->getField("liftKp")->getSFFloat();
                ki = root_->getField("liftKi")->getSFFloat();
                maxErrorSum = root_->getField("liftMaxErrorSum")->getSFFloat();
                printf("New lift gains: %f %f %f\n", kp, ki, maxErrorSum);
                robot_->SendLiftControllerGains(kp, ki, maxErrorSum);
              } else {
                printf("No BlockWorldComms was found in world\n");
              }
              break;
            }
            case CKEY_SET_VISIONSYSTEM_PARAMS:
            {
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
              break;
            }
            case CKEY_CYCLE_SOUND_SCHEME:
            {
              SoundSchemeID_t nextSoundScheme = (SoundSchemeID_t)(SoundManager::getInstance()->GetScheme() + 1);
              if (nextSoundScheme == NUM_SOUND_SCHEMES) {
                nextSoundScheme = SOUND_SCHEME_COZMO;
              }
              printf("Sound scheme: %d\n", nextSoundScheme);
              SoundManager::getInstance()->SetScheme(nextSoundScheme);
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
                robot_->DriveWheels(0, 0);
              }
              
              // If the last key pressed was a move lift key then stop it.
              if (lastKeyPressed_ == CKEY_LIFT_UP || lastKeyPressed_ == CKEY_LIFT_DOWN) {
                robot_->MoveLift(0);
              }
              
              // If the last key pressed was a move head key then stop it.
              if (lastKeyPressed_ == CKEY_HEAD_UP || lastKeyPressed_ == CKEY_HEAD_DOWN) {
                robot_->MoveHead(0);
              }
              
            }
              
          } // switch(key)
            
            break;
          } // while(1)
          
          lastKeyPressed_ = key;
          lastKeyAndModPressed_ = key | modifier_key;
          
        } // if KEYBOARD_CONTROL_ENABLED
        
      } // BSKeyboardController::ProcessKeyStroke()
      
    } // namespace Sim
  } // namespace Cozmo
} // namespace Anki

