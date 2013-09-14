#include "hal/motors.h"
#include "hal/sim_motors.h"

#include <webots/robot.h>
#include <webots/motor.h>
#include <webots/connector.h>

#include <stdio.h>

#include "cozmoConfig.h"



//Reference to the steering wheels
WbDeviceTag wheels[2];
//The head pitch motor
WbDeviceTag head_pitch;
//The lift motor
WbDeviceTag lift;
WbDeviceTag lift2;

WbDeviceTag con_lift;



static bool locked = false;

//Count down after unlock until the next lock
static int unlockhysteresis = 0;

void InitMotors()
{
  wheels[0] = wb_robot_get_device(WHEEL_FL);
  wheels[1] = wb_robot_get_device(WHEEL_FR);
  head_pitch = wb_robot_get_device(PITCH);
  lift = wb_robot_get_device(LIFT);
  lift2 = wb_robot_get_device(LIFT2);
  
  
  //Set the motors to velocity mode
  wb_motor_set_position(wheels[0], INFINITY);
  wb_motor_set_position(wheels[1], INFINITY);
  
  //Set the head pitch to 0
  wb_motor_set_position(head_pitch, 0);
  wb_motor_set_position(lift, LIFT_CENTER);
  wb_motor_set_position(lift2, -LIFT_CENTER);
  
  
  locked = false;
  unlockhysteresis = 0;
  
  /* get a handler to the connector and the motor. */
  con_lift = wb_robot_get_device("connector");
  
  /* activate them. */
  wb_connector_enable_presence(con_lift, TIME_STEP);
  
}

//Sets an open loop speed to the two motors. The open loop speed value ranges
//from: [0..MOTOR_PWM_MAXVAL] and HAS to be within those boundaries
void SetMotorOLSpeed(s16 speedl, s16 speedr)
{
  // Convert PWM to rad/s
  // TODO: Do this properly.  For now assume MOTOR_PWM_MAXVAL achieves 1m/s lateral speed.
  
  // Radius ~= 15mm => circumference of ~95mm.
  // 1m/s == 10.5 rot/s == 66.1 rad/s
  float left_rad_per_s = speedl * 66.1 / MOTOR_PWM_MAXVAL;
  float right_rad_per_s = speedr * 66.1 / MOTOR_PWM_MAXVAL;
  
  wb_motor_set_velocity(wheels[0], -left_rad_per_s);
  wb_motor_set_velocity(wheels[1], -right_rad_per_s);
}

// Get the PWM commands being sent to the motors
void GetMotorOLSpeed(s16* speedl, s16* speedr)
{
}


void DisengageGripper()
{
  if (locked == true)
  {
    locked = false;
    unlockhysteresis = HIST;
    wb_connector_unlock(con_lift);
    printf("UNLOCKED!\n");
  }
}

void ManageMotors()
{
  //Should we lock to a block which is close to the connector?
  if (locked == false && wb_connector_get_presence(con_lift) == 1)
  {
    if (unlockhysteresis == 0)
    {
      wb_connector_lock(con_lift);
      locked = true;
      printf("LOCKED!\n");
    }else{
      unlockhysteresis--;
    }
  }
  
  
}


////////// SIMULATOR-ONLY FUNCTIONS ///////////

void SetLiftAngle(float angle)
{
  wb_motor_set_position(lift, angle);
  wb_motor_set_position(lift2, -angle);
}
float GetLiftAngle()
{
  return wb_motor_get_position(lift);
}


//Set an angular wheel velocity in rad/sec
void SetAngularWheelVelocity(float left, float right)
{
  // write actuators inputs
  wb_motor_set_velocity(wheels[0], -left);
  wb_motor_set_velocity(wheels[1], -right);
}


void SetHeadAngle(float pitch_angle)
{
  wb_motor_set_position(head_pitch, pitch_angle);
}

float GetHeadAngle()
{
  return wb_motor_get_position(head_pitch);
}
