#ifndef SIM_MOTORS_H
#define SIM_MOTORS_H

#include "cozmoTypes.h"


//Names of the wheels used for steering
#define WHEEL_FL "wheel_fl"
#define WHEEL_FR "wheel_fr"
#define PITCH "motor_head_pitch"
#define LIFT "lift_motor"
#define LIFT2 "lift_motor2"
#define GOAL "goal_motor"

#define DRIVE_VELOCITY_SLOW 5.0f
#define TURN_VELOCITY_SLOW 1.0f
#define HIST 50
#define LIFT_CENTER -0.275
#define LIFT_UP 0.635
#define LIFT_UPUP 0.7


void SetLiftAngle(float angle);
float GetLiftAngle(void);

//Set an angular wheel velocity in rad/sec
void SetAngularWheelVelocity(float left, float right);

void GetMotorAngles(float *left_angle, float *right_angle);

void SetHeadAngle(float pitch_angle);
float GetHeadAngle(void);

#endif // SIM_MOTORS_H
