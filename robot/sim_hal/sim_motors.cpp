#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"


#include <stdio.h>

extern CozmoBot gCozmoBot;


// TODO:
// Separate files for each motor class: wheels, gripper, head.


void InitMotors()
{
  gCozmoBot.SetWheelAngularVelocity(0,0);
  gCozmoBot.SetHeadPitch(0);
  gCozmoBot.SetLiftPitch(LIFT_CENTER);
}


//Sets an open loop speed to the two motors. The open loop speed value ranges
//from: [0..MOTOR_PWM_MAXVAL] and HAS to be within those boundaries
void SetMotorOLSpeed(s16 speedl, s16 speedr)
{
  if (IsKeyboardControllerEnabled()){
    return;
  }

  // Convert PWM to rad/s
  // TODO: Do this properly.  For now assume MOTOR_PWM_MAXVAL achieves 1m/s lateral speed.
  
  // Radius ~= 15mm => circumference of ~95mm.
  // 1m/s == 10.5 rot/s == 66.1 rad/s
  float left_rad_per_s = speedl * 66.1 / MOTOR_PWM_MAXVAL;
  float right_rad_per_s = speedr * 66.1 / MOTOR_PWM_MAXVAL;
  
  gCozmoBot.SetWheelAngularVelocity(left_rad_per_s, right_rad_per_s);
}

// Get the PWM commands being sent to the motors
void GetMotorOLSpeed(s16* speedl, s16* speedr)
{
}


void DisengageGripper()
{
  gCozmoBot.DisengageGripper();
}


