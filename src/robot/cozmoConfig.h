#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"

#include "app/vehicleMath.h"

// Cozmo control loop is 100Hz
#define TIME_STEP 10 //ms

const float CONTROL_DT = TIME_STEP*0.001;
const float ONE_OVER_CONTROL_DT = 1.0/CONTROL_DT;

const float WHEEL_DIAMETER_MM = 30;  // This should be in sync with the CozmoBot proto file
const float HALF_WHEEL_CIRCUM = WHEEL_DIAMETER_MM * M_PI_2;
const float WHEEL_RAD_TO_MM = WHEEL_DIAMETER_MM / 2;  // or HALF_WHEEL_CIRCUM / PI;

#endif // COZMO_CONFIG_H