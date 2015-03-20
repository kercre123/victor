#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/cozmoTypes.h"
#include "anki/cozmo/robot/hal.h"

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
  // Get true (gyro measured) speeds from robot model
  leftWheelSpeed_mmps = gCozmoBot.GetLeftWheelSpeed();
  rightWheelSpeed_mmps = gCozmoBot.GetRightWheelSpeed();

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


