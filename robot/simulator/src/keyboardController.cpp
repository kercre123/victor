
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"

#include "keyboardController.h"

#include <stdio.h>
#include <string.h>


namespace Anki {
  namespace Cozmo {
    namespace KeyboardController {
      
      //
      // Parameters / Constants
      //
      
      // For Webots:
      const f32 DRIVE_VELOCITY_SLOW = 5.0f;
      const f32 TURN_VELOCITY_SLOW = 1.0f;
      const f32 LIFT_CENTER = -0.275;
      const f32 LIFT_UP = 0.635;
      const f32 LIFT_UPUP = 0.7;
      
    }
    
    namespace {
      // Private memers:
      webots::Robot* robot = NULL;

      bool isInitialized = false;
      bool isEnabled = false;
    }
    
    void KeyboardController::Init(webots::Robot* robotIn)
    {
      if(robotIn != NULL) {
        robot = robotIn;
        isInitialized = true;
      } else {
        fprintf(stdout, "Given robot pointer is NULL!\n");
      }
    }
      
    void KeyboardController::Enable(void)
    {
      if(isInitialized) {
        //Update Keyboard every 0.1 seconds
        robot->keyboardEnable(TIME_STEP);
        
        isEnabled = true;
        
        printf("Drive: arrows\n");
        printf("Lift up/down: a/z\n");
        printf("Head up/down: s/x\n");
        printf("Undock: space\n");
      }
      else {
        fprintf(stdout, "Cannot enable uninitialized keyobard!\n");
      }
    }

    void KeyboardController::Disable(void)
    {
      //Disable the keyboard
      robot->keyboardDisable();
      isEnabled = false;
    }

    bool KeyboardController::IsEnabled()
    {
      return isEnabled;
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
      
      if (isEnabled)
      {
        int key = robot->keyboardGetKey();
        
        switch (key)
        {
          case webots::Robot::KEYBOARD_UP:
          {
            SetWheelAngularVelocity(DRIVE_VELOCITY_SLOW, DRIVE_VELOCITY_SLOW);
            break;
          }
            
          case webots::Robot::KEYBOARD_DOWN:
          {
            SetWheelAngularVelocity(-DRIVE_VELOCITY_SLOW, -DRIVE_VELOCITY_SLOW);
            break;
          }
            
          case webots::Robot::KEYBOARD_LEFT:
          {
            SetWheelAngularVelocity(-TURN_VELOCITY_SLOW, TURN_VELOCITY_SLOW);
            break;
          }
            
          case webots::Robot::KEYBOARD_RIGHT:
          {
            SetWheelAngularVelocity(TURN_VELOCITY_SLOW, -TURN_VELOCITY_SLOW);
            break;
          }
            
          case CKEY_HEAD_UP: //s-key: move head up
          {
            SetHeadPitch(GetHeadPitch() + 0.01f);
            break;
          }
            
          case CKEY_HEAD_DOWN: //x-key: move head down
          {
            SetHeadPitch(GetHeadPitch() - 0.01f);
            break;
          }
          case CKEY_LIFT_UP: //a-key: move lift up
          {
            SetLiftPitch(GetLiftPitch() + 0.02f);
            break;
          }
            
          case CKEY_LIFT_DOWN: //z-key: move lift down
          {
            SetLiftPitch(GetLiftPitch() - 0.02f);
            break;
          }
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
            
          case CKEY_UNLOCK: //spacebar-key: unlock
          {
            DisengageGripper();
            break;
          }
          default:
          {
            SetWheelAngularVelocity(0, 0);
          }
            
        } // switch(key)
        
      } // if KEYBOARD_CONTROL_ENABLED
      
    } // KeyboardController::ProcessKeyStroke()
    
    
  } // namespace Cozmo
  
} // namespace Anki

