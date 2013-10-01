#ifndef __ENCODERS_H
#define __ENCODERS_H

#include "cozmoTypes.h"

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
