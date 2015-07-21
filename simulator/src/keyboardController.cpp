
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "pathFollower.h"
#include "steeringController.h"

#include "headController.h"
#include "anki/cozmo/simulator/keyboardController.h"
#include "liftController.h"
#include "dockingController.h"

#include <stdio.h>
#include <string.h>

// Webots includes
#include <webots/Supervisor.hpp>


namespace Anki {
  namespace Cozmo {
    namespace Sim {

      extern webots::Supervisor* CozmoBot;
      
      // Private memers:
      namespace {
        // Constants for Webots:
        const f32 DRIVE_VELOCITY_FAST = 75.0f; // mm/s
        const f32 DRIVE_VELOCITY_SLOW = 20.0f; // mm/s
        const f32 TURN_VELOCITY_SLOW = 1.0f;
        
        int lastKeyPressed_ = 0;
        bool isEnabled_ = false;
        
      } // private namespace
      
      void KeyboardController::Enable(void)
      {
        //Update Keyboard every 0.1 seconds
        CozmoBot->keyboardEnable(TIME_STEP);
        
        isEnabled_ = true;
        
        printf("Drive: arrows\n");
        printf("Lift up/down: a/z\n");
        printf("Head up/down: s/x\n");
        printf("Undock: space\n");
      } // Enable()
      
      void KeyboardController::Disable(void)
      {
        //Disable the keyboard
        CozmoBot->keyboardDisable();
        isEnabled_ = false;
      }
      
      bool KeyboardController::IsEnabled()
      {
        return isEnabled_;
      }
      
      
      //Check the keyboard keys and issue robot commands
      void KeyboardController::ProcessKeystroke()
      {
        using namespace HAL;
        
        //Why do some of those not match ASCII codes?
        //Numbers, spacebar etc. work, letters are different, why?
        //a, z, s, x, Space
        const s32 CKEY_LIFT_UP    = 65;
        const s32 CKEY_LIFT_DOWN  = 90;
        const s32 CKEY_HEAD_UP    = 83;
        const s32 CKEY_HEAD_DOWN  = 88;
        const s32 CKEY_UNLOCK     = 32;
        
        if (isEnabled_)
        {
          int key = CozmoBot->keyboardGetKey();
          
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
              SteeringController::ExecuteDirectDrive(wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_DOWN:
            {
              SteeringController::ExecuteDirectDrive(-wheelSpeed, -wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_LEFT:
            {
              SteeringController::ExecuteDirectDrive(-wheelSpeed, wheelSpeed);
              break;
            }
              
            case webots::Robot::KEYBOARD_RIGHT:
            {
              SteeringController::ExecuteDirectDrive(wheelSpeed, -wheelSpeed);
              break;
            }
              
            case CKEY_HEAD_UP: //s-key: move head up
            {
              HeadController::SetAngularVelocity(TURN_VELOCITY_SLOW);
              break;
            }
              
            case CKEY_HEAD_DOWN: //x-key: move head down
            {
              HeadController::SetAngularVelocity(-TURN_VELOCITY_SLOW);
              break;
            }
            case CKEY_LIFT_UP: //a-key: move lift up
            {
              LiftController::SetAngularVelocity(TURN_VELOCITY_SLOW);
              break;
            }
              
            case CKEY_LIFT_DOWN: //z-key: move lift down
            {
              LiftController::SetAngularVelocity(-TURN_VELOCITY_SLOW);
              break;
            }
            case '1': //set lift to low dock height
            {
              LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
              break;
            }
              
            case '2': //set lift to high dock height
            {
              LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
              break;
            }
              
            case '3': //set lift to carry height
            {
              LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
              break;
            }
              
            case '4': //set head to look all the way down
            {
              HeadController::SetDesiredAngle(MIN_HEAD_ANGLE);
              break;
            }
              
            case '5': //set head to straight ahead
            {
              HeadController::SetDesiredAngle(0);
              break;
            }
              
            case '6': //set head to look all the way up
            {
              HeadController::SetDesiredAngle(MAX_HEAD_ANGLE);
              break;
            }


            case CKEY_UNLOCK: //spacebar-key: unlock
            {
              if (HAL::IsGripperEngaged()) {
                HAL::DisengageGripper();
              } else {
                HAL::EngageGripper();
              }
              break;
            }
            default:
            {
              //if(not PathFollower::IsTraversingPath()) {
              //if(Robot::GetOperationMode() == Robot::WAITING) {
                // Don't stop the motors if they are being controlled
                // by other processes
                if (SteeringController::GetMode() == SteeringController::SM_DIRECT_DRIVE)
                  SteeringController::ExecuteDirectDrive(0.f, 0.f);
                
                if(LiftController::IsInPosition()) {
                  LiftController::SetAngularVelocity(0.f);
                }
                if(HeadController::IsInPosition()) {
                  HeadController::SetAngularVelocity(0.f);
                }
              //}
            }
              
          } // switch(key)
          
          lastKeyPressed_ = key | modifier_key;
          
        } // if KEYBOARD_CONTROL_ENABLED
        
      } // KeyboardController::ProcessKeyStroke()
      
    } // namespace Sim
  } // namespace Cozmo
} // namespace Anki

