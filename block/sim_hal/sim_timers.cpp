#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/CozmoBot.h"

extern CozmoBot gCozmoBot;

static u32 us_time = 0;

// Initialize system timers, excluding watchdog
// This must run first in main()
void InitTimers(void)
{

}


// Get the number of microseconds since boot
u32 getMicroCounter(void)
{ 
  return gCozmoBot.getTime() * 1000000;
}

