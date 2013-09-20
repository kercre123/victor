#include "cozmoConfig.h"
#include "cozmoTypes.h" 
#include "hal/encoders.h"
#include "hal/timers.h"
#include "hal/sim_motors.h"


//Defines the initial coefficient for the filter
#define DEFAULT_FILTER_COEF 0.9f


static s32 leftWheelSpeed_mmps = 0;
static s32 rightWheelSpeed_mmps = 0;

void InitEncoders(void)
{            

}


// Fetch the latest encoder speed in mm per second (settable using UNITS_PER_TICK)
s32 GetLeftWheelSpeed(void)
{
  return leftWheelSpeed_mmps;
}

s32 GetRightWheelSpeed(void)
{
  return rightWheelSpeed_mmps;
}



/////////////////////////////// FILTERING THE ENCODER VALUES
////////////////////////////////////////////////////////////

// This is the variable containing the filter coefficient
float filter_coef = DEFAULT_FILTER_COEF;

float filterSpeedL = 0;
float filterSpeedR = 0;

// Runs one step of the wheel encoder filter;
void EncoderSpeedFilterIteration(void)
{
  // Get rad/s
  float leftRad, rightRad, dLeftRad, dRightRad;
  static float prevLeftRad = 0, prevRightRad = 0;
  u32 dtime = 0;
  static u32 prev_us_time = 0;

  GetMotorAngles(&leftRad, &rightRad);
  dLeftRad = prevLeftRad - leftRad;
  dRightRad = prevRightRad - rightRad;

  prevLeftRad = leftRad;
  prevRightRad = rightRad;

  // Convert to mm/s
  float dLeftMM = dLeftRad * WHEEL_RAD_TO_MM;
  float dRightMM = dRightRad * WHEEL_RAD_TO_MM;

  leftWheelSpeed_mmps = dLeftMM / CONTROL_DT;
  rightWheelSpeed_mmps = dRightMM / CONTROL_DT;




    filterSpeedL = GetLeftWheelSpeed() * (1.0f - filter_coef)   +   (filterSpeedL * filter_coef);
    filterSpeedR = GetRightWheelSpeed() * (1.0f - filter_coef)   +   (filterSpeedR * filter_coef);
}

// Get the filtered values back
s32 GetLeftWheelSpeedFiltered(void)
{
    return filterSpeedL;
}
s32 GetRightWheelSpeedFiltered(void)
{
    return filterSpeedR;
}


///////// SIM ONLY /////////

