#include "basestationKeyboardController.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/robot/cozmoConfig.h"

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
        const f32 DRIVE_VELOCITY_FAST = 70.f; // mm/s
        const f32 DRIVE_VELOCITY_SLOW = 30.f; // mm/s
        
        const f32 LIFT_SPEED_RAD_PER_SEC = 0.5f;
        const f32 LIFT_ACCEL_RAD_PER_SEC2 = 10.f;

        const f32 HEAD_SPEED_RAD_PER_SEC = 0.5f;
        const f32 HEAD_ACCEL_RAD_PER_SEC2 = 10.f;
        
        RobotManager* robotMgr_;
        Robot* robot_;
        
        int lastKeyPressed_ = 0;
        
        bool isEnabled_ = false;
        
      } // private namespace
      
      
      void BSKeyboardController::Init(RobotManager *robotMgr)
      {
        robotMgr_ = robotMgr;
      }
      
      void BSKeyboardController::Enable(void)
      {
        basestationController.keyboardEnable(TIME_STEP);
        
        isEnabled_ = true;

        printf("\nBasestation keyboard control\n");
        printf("===============================\n");
        printf("Drive: arrows\n");
        printf("Lift low/high/carry: 1/2/3\n");
        printf("Head down/forward/up: 4/5/6\n\n");

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
      
      
      //Check the keyboard keys and issue robot commands
      void BSKeyboardController::ProcessKeystroke()
      {
        
        //Why do some of those not match ASCII codes?
        //Numbers, spacebar etc. work, letters are different, why?
        //a, z, s, x, Space
        //const s32 CKEY_LIFT_UPUP  = 81;  // q
        const s32 CKEY_LIFT_UP    = 65;  // a
        const s32 CKEY_LIFT_DOWN  = 90;  // z
        //const s32 CKEY_HEAD_UPUP  = 87;  // w
        const s32 CKEY_HEAD_UP    = 83;  // s
        const s32 CKEY_HEAD_DOWN  = 88;  // x
        //const s32 CKEY_UNLOCK     = 32;  // space

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
          
          // Skip if same key as before
          if (key == lastKeyPressed_)
            return;
          
          // Extract modifier key
          int modifier_key = key & ~webots::Supervisor::KEYBOARD_KEY;
          
          // Adjust wheel speed appropriately
          f32 wheelSpeed = DRIVE_VELOCITY_FAST;
          if (modifier_key == webots::Supervisor::KEYBOARD_SHIFT) {
            wheelSpeed = DRIVE_VELOCITY_SLOW;
          }
          
          // Set key to its modifier-less self
          key &= webots::Supervisor::KEYBOARD_KEY;
          
          
          //printf("keypressed: %d, modifier %d, orig_key %d, prev_key %d\n",
          //       key, modifier_key, key | modifier_key, lastKeyPressed_);
          
          switch (key)
          {
            case webots::Robot::KEYBOARD_UP:
            {
              robot_->SendDriveWheels(wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_DOWN:
            {
              robot_->SendDriveWheels(-wheelSpeed, -wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_LEFT:
            {
              robot_->SendDriveWheels(-wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_RIGHT:
            {
              robot_->SendDriveWheels(wheelSpeed, -wheelSpeed);
              break;
            }
              
            case CKEY_HEAD_UP: //s-key: move head UP 1 degree from current position (or 5 degrees with shift)
            {
              const Radians adjAngle((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? DEG_TO_RAD(5) : DEG_TO_RAD(1));
              robot_->SendMoveHead((robot_->get_headAngle() + adjAngle).ToFloat(),
                                   HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case CKEY_HEAD_DOWN: //x-key: move head DOWN 1 degree from current position (or 5 degrees with shift)
            {
              const Radians adjAngle((modifier_key == webots::Supervisor::KEYBOARD_SHIFT) ? DEG_TO_RAD(5) : DEG_TO_RAD(1));
              robot_->SendMoveHead((robot_->get_headAngle() - adjAngle).ToFloat(),
                                   HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case CKEY_LIFT_UP: //a-key: move lift up
            {

              break;
            }
              
            case CKEY_LIFT_DOWN: //z-key: move lift down
            {

              break;
            }

            case '1': //set lift to low dock height
            {
              robot_->SendMoveLift(LIFT_HEIGHT_LOWDOCK, LIFT_SPEED_RAD_PER_SEC, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '2': //set lift to high dock height
            {
              robot_->SendMoveLift(LIFT_HEIGHT_HIGHDOCK, LIFT_SPEED_RAD_PER_SEC, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '3': //set lift to carry height
            {
              robot_->SendMoveLift(LIFT_HEIGHT_CARRY, LIFT_SPEED_RAD_PER_SEC, LIFT_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '4': //set head to look all the way down
            {
              robot_->SendMoveHead(MIN_HEAD_ANGLE, HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }

            case '5': //set head to straight ahead
            {
              robot_->SendMoveHead(0, HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }
              
            case '6': //set head to look all the way up
            {
              robot_->SendMoveHead(MAX_HEAD_ANGLE, HEAD_SPEED_RAD_PER_SEC, HEAD_ACCEL_RAD_PER_SEC2);
              break;
            }

            default:
            {
              // Stop wheels
              robot_->SendDriveWheels(0, 0);
            }
              
          } // switch(key)
          
          lastKeyPressed_ = key | modifier_key;
          
        } // if KEYBOARD_CONTROL_ENABLED
        
      } // BSKeyboardController::ProcessKeyStroke()
      
    } // namespace Sim
  } // namespace Cozmo
} // namespace Anki

