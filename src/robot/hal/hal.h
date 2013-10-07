#ifndef __HAL_H
#define __HAL_H

#include "cozmoTypes.h"


/////////////////////////////////////
/// TIMERS
/////////////////////////////////////

// Initialize system timers, excluding watchdog
void InitTimers(void);

// The microsecond clock counts 1 tick per microsecond after startup
// It will wrap every 71.58 minutes
u32 getMicroCounter(void);




/////////////////////////////////////
/// MOTORS
/////////////////////////////////////

#define MOTOR_PWM_MAXVAL 2400

void InitMotors(void);

//Sets an open loop speed to the two motors. The open loop speed value ranges
//from: [0..MOTOR_PWM_MAXVAL] and HAS to be within those boundaries
void SetMotorOLSpeed(s16 speedl, s16 speedr);

// Get the PWM commands being sent to the motors
void GetMotorOLSpeed(s16* speedl, s16* speedr);

void DisengageGripper(void);

void ManageMotors(void);



/////////////////////////////////////
/// ENCODERS
/////////////////////////////////////


void InitEncoders(void);

// Fetch the latest encoder speed in mm per second (settable using UNITS_PER_TICK)
s32 GetLeftWheelSpeed(void);
s32 GetRightWheelSpeed(void);

// Fetch the distance in mm travelled by each wheel - backward counts the same as forward
// This value will not overflow, and restarts at 0 after each startup
//u32 GetLeftWheelOdometer(void);
//u32 GetRightWheelOdometer(void);
//u32 GetWheelOdometer(void);

// Set the filter "factor" between 0..1: 0 means ONLY the new value counts (no filtering)
// The larger the value, the more we "filter", the more of a low-pass we become
//void SetEncoderFilterCoefficient(double fc);

// Get the filtered values back
s32 GetLeftWheelSpeedFiltered(void);
s32 GetRightWheelSpeedFiltered(void);

//Runs one iteration of the encoder value filter
void EncoderSpeedFilterIteration(void);

// Set the buffers for recording data into car test buffers
//void EncoderSetLeftBuffer(u16* buffer, u16* end);
//void EncoderSetRightBuffer(u16* buffer, u16* end);

#endif
