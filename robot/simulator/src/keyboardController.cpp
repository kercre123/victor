
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/steeringController.h"

#include "headController.h"
#include "keyboardController.h"
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
        const f32 DRIVE_VELOCITY_SLOW = 100.0f; // mm/s
        const f32 TURN_WHEEL_VELOCITY_SLOW = 50.0f;  //mm/s
        const f32 TURN_VELOCITY_SLOW = 1.0f;
        const f32 LIFT_CENTER = -0.275;
        const f32 LIFT_UP = 0.635;
        const f32 LIFT_UPUP = 0.7;
        
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
          
          switch (key)
          {
            case webots::Robot::KEYBOARD_UP:
            {
              SteeringController::ExecuteDirectDrive(DRIVE_VELOCITY_SLOW, DRIVE_VELOCITY_SLOW);
              break;
            }
              
            case webots::Robot::KEYBOARD_DOWN:
            {
              SteeringController::ExecuteDirectDrive(-DRIVE_VELOCITY_SLOW, -DRIVE_VELOCITY_SLOW);
              break;
            }
              
            case webots::Robot::KEYBOARD_LEFT:
            {
              SteeringController::ExecuteDirectDrive(-TURN_WHEEL_VELOCITY_SLOW, TURN_WHEEL_VELOCITY_SLOW);
              break;
            }
              
            case webots::Robot::KEYBOARD_RIGHT:
            {
              SteeringController::ExecuteDirectDrive(TURN_WHEEL_VELOCITY_SLOW, -TURN_WHEEL_VELOCITY_SLOW);
              break;
            }
              
            case CKEY_HEAD_UP: //s-key: move head up
            {
              HeadController::SetAngularVelocity(1.5f*TURN_VELOCITY_SLOW);
              break;
            }
              
            case CKEY_HEAD_DOWN: //x-key: move head down
            {
              HeadController::SetAngularVelocity(-1.5f*TURN_VELOCITY_SLOW);
              break;
            }
            case CKEY_LIFT_UP: //a-key: move lift up
            {
              LiftController::SetAngularVelocity(1.5f*TURN_VELOCITY_SLOW);
              break;
            }
              
            case CKEY_LIFT_DOWN: //z-key: move lift down
            {
              LiftController::SetAngularVelocity(-1.5f*TURN_VELOCITY_SLOW);
              break;
            }
              /*
            case '1': //set lift to pickup position
            {
              SetLiftPitch(LIFT_CENTER);
              break;
            }
            case '2': //set lift to block +1 position
            {
              SetLiftPitch(LIFT_UP);
              break;
            }
            case '3': //set lift to highest position
            {
              SetLiftPitch(LIFT_UPUP);
              break;
            }
               */
              
            case CKEY_UNLOCK: //spacebar-key: unlock
            {
              DockingController::DisengageGripper();
              break;
            }
            default:
            {
              //if(not PathFollower::IsTraversingPath()) {
              if(Robot::GetOperationMode() == Robot::WAITING) {
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
              }
            }
              
          } // switch(key)
          
        } // if KEYBOARD_CONTROL_ENABLED
        
      } // KeyboardController::ProcessKeyStroke()
      
    } // namespace Sim
  } // namespace Cozmo
} // namespace Anki

