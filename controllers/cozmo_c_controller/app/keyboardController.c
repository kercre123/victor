#include "keyboardController.h"
#include "hal/sim_motors.h"
#include "hal/motors.h"
#include "cozmoConfig.h"

#include <stdio.h>
#include <string.h>

#include <webots/robot.h>
#include <webots/connector.h>



//Why do some of those not match ASCII codes?
//Numbers, spacebar etc. work, letters are different, why?
//a, z, s, x, Space
#define CKEY_LIFT_UP    65 
#define CKEY_LIFT_DOWN  90 
#define CKEY_HEAD_UP    83 
#define CKEY_HEAD_DOWN  88 
#define CKEY_UNLOCK     32 


static BOOL keyboardCtrlEnabled_ = FALSE;

void EnableKeyboardController(void) 
{
  //Update Keyboard every 0.1 seconds
  wb_robot_keyboard_enable(TIME_STEP); 
  keyboardCtrlEnabled_ = TRUE;

  printf("Drive: arrows\n");
  printf("Lift up/down: a/z\n");
  printf("Head up/down: s/x\n");
  printf("Undock: space\n");
}

void DisableKeyboardController(void)
{
  //Disable the keyboard
  wb_robot_keyboard_disable();
  keyboardCtrlEnabled_ = FALSE;
}

BOOL IsKeyboardControllerEnabled()
{
  return keyboardCtrlEnabled_;
}

//Check the keyboard keys and issue robot commands
void RunKeyboardController()
{
  int key = wb_robot_keyboard_get_key();
  
  static float pitch_angle = 0;
  static float lift_angle = LIFT_CENTER;

  
  switch (key)
  {
    case WB_ROBOT_KEYBOARD_UP:
    {  
      SetAngularWheelVelocity(DRIVE_VELOCITY_SLOW, DRIVE_VELOCITY_SLOW);
      break;
    }
    
    case WB_ROBOT_KEYBOARD_DOWN:
    {
      SetAngularWheelVelocity(-DRIVE_VELOCITY_SLOW, -DRIVE_VELOCITY_SLOW);
      break;
    }
    
    case WB_ROBOT_KEYBOARD_LEFT:
    {
      SetAngularWheelVelocity(-TURN_VELOCITY_SLOW, TURN_VELOCITY_SLOW);
      break;
    }
    
    case WB_ROBOT_KEYBOARD_RIGHT:
    {
      SetAngularWheelVelocity(TURN_VELOCITY_SLOW, -TURN_VELOCITY_SLOW);
      break;
    }
    
    case CKEY_HEAD_UP: //s-key: move head up
    {
      pitch_angle += 0.01f;
      SetHeadAngle(pitch_angle);
      break;
    }
    
    case CKEY_HEAD_DOWN: //x-key: move head down
    {
      pitch_angle -= 0.01f;
      SetHeadAngle(pitch_angle);
      break;
    }
    case CKEY_LIFT_UP: //a-key: move lift up
    {
      lift_angle += 0.02f;
      SetLiftAngle(lift_angle);
      break;
    }
    
    case CKEY_LIFT_DOWN: //z-key: move lift down
    {
      lift_angle -= 0.02f;
      SetLiftAngle(lift_angle);
      break;
    }
    case '1': //set lift to pickup position
    {
      lift_angle = LIFT_CENTER;
      SetLiftAngle(lift_angle);
      break;
    }
    case '2': //set lift to block +1 position
    {
      lift_angle = LIFT_UP;
      SetLiftAngle(lift_angle);
      break;
    }
    case '3': //set lift to highest position
    {
      lift_angle = LIFT_UPUP;
      SetLiftAngle(lift_angle);
      break;
    }
    
    case CKEY_UNLOCK: //spacebar-key: unlock
    {
      DisengageGripper();
      break;
    }
    
    
    default:
    {
      SetAngularWheelVelocity(0, 0);
    }
    
  }
  
}