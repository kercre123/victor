#include "app/wheelController.h"
#include "hal/motors.h"


static s16 desiredSpeed_ = 0;

void SetDesiredSpeed(s16 speed_mm_per_sec)
{
  desiredSpeed_ = speed_mm_per_sec;
}

s16 GetDesiredSpeed()
{
  return desiredSpeed_;
}

void ManageWheelSpeedController(s16 fidx)
{
  s16 lpwm = 0, rpwm = 0;

  // Get desired speed
  s16 speed = GetDesiredSpeed();
  
  if (speed > 0) {
    
    // THIS IS FOR SIM ONLY
    lpwm = speed * MOTOR_PWM_MAXVAL / 66.1;
    rpwm = speed * MOTOR_PWM_MAXVAL / 66.1;
    

    // Incorporate steer to determine left and right wheel PWMs
    lpwm += fidx * 10;
    rpwm -= fidx * 10;
  }
  
  
  SetMotorOLSpeed(lpwm,rpwm);
  
  
  
  
  ManageMotors();
}