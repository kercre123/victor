#ifndef MOTORS_H
#define MOTORS_H

#include "cozmoTypes.h"

#define MOTOR_PWM_MAXVAL 2400

void InitMotors(void);

//Sets an open loop speed to the two motors. The open loop speed value ranges
//from: [0..MOTOR_PWM_MAXVAL] and HAS to be within those boundaries
void SetMotorOLSpeed(s16 speedl, s16 speedr);

// Get the PWM commands being sent to the motors
void GetMotorOLSpeed(s16* speedl, s16* speedr);

void DisengageGripper(void);

void ManageMotors(void);

#endif // MOTORS_H