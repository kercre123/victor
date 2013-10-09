
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"

#include "keyboardController.h"

#include <stdio.h>
#include <string.h>

extern CozmoBot gCozmoBot;


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
  gCozmoBot.keyboardEnable(TIME_STEP); 
  keyboardCtrlEnabled_ = TRUE;

  printf("Drive: arrows\n");
  printf("Lift up/down: a/z\n");
  printf("Head up/down: s/x\n");
  printf("Undock: space\n");
}

void DisableKeyboardController(void)
{
  //Disable the keyboard
  gCozmoBot.keyboardDisable();
  keyboardCtrlEnabled_ = FALSE;
}

BOOL IsKeyboardControllerEnabled()
{
  return keyboardCtrlEnabled_;
}

//Check the keyboard keys and issue robot commands
void RunKeyboardController()
{
  if (!keyboardCtrlEnabled_) {
    return;
  }

  int key = gCozmoBot.keyboardGetKey();
  
  switch (key)
  {
    case webots::Robot::KEYBOARD_UP:
    {  
      gCozmoBot.SetWheelAngularVelocity(DRIVE_VELOCITY_SLOW, DRIVE_VELOCITY_SLOW);
      break;
    }
    
    case webots::Robot::KEYBOARD_DOWN:
    {
      gCozmoBot.SetWheelAngularVelocity(-DRIVE_VELOCITY_SLOW, -DRIVE_VELOCITY_SLOW);
      break;
    }
    
    case webots::Robot::KEYBOARD_LEFT:
    {
      gCozmoBot.SetWheelAngularVelocity(-TURN_VELOCITY_SLOW, TURN_VELOCITY_SLOW);
      break;
    }
    
    case webots::Robot::KEYBOARD_RIGHT:
    {
      gCozmoBot.SetWheelAngularVelocity(TURN_VELOCITY_SLOW, -TURN_VELOCITY_SLOW);
      break;
    }
    
    case CKEY_HEAD_UP: //s-key: move head up
    {
      gCozmoBot.SetHeadPitch(gCozmoBot.GetHeadPitch() + 0.01f);
      break;
    }
    
    case CKEY_HEAD_DOWN: //x-key: move head down
    {
      gCozmoBot.SetHeadPitch(gCozmoBot.GetHeadPitch() - 0.01f);
      break;
    }
    case CKEY_LIFT_UP: //a-key: move lift up
    {
      gCozmoBot.SetLiftPitch(gCozmoBot.GetLiftPitch() + 0.02f);
      break;
    }
    
    case CKEY_LIFT_DOWN: //z-key: move lift down
    {
      gCozmoBot.SetLiftPitch(gCozmoBot.GetLiftPitch() - 0.02f);
      break;
    }
    case '1': //set lift to pickup position
    {
      gCozmoBot.SetLiftPitch(LIFT_CENTER);
      break;
    }
    case '2': //set lift to block +1 position
    {
      gCozmoBot.SetLiftPitch(LIFT_UP);
      break;
    }
    case '3': //set lift to highest position
    {
      gCozmoBot.SetLiftPitch(LIFT_UPUP);
      break;
    }
    
    case CKEY_UNLOCK: //spacebar-key: unlock
    {
      gCozmoBot.DisengageGripper();
      break;
    }
    
    
    default:
    {
      gCozmoBot.SetWheelAngularVelocity(0, 0);
    }
    
  }
  
}