#include "hal/timers.h"
#include "hal/sim_timers.h"


static u32 us_time = 0;

// Initialize system timers, excluding watchdog
// This must run first in main()
void InitTimers(void)
{

}


// Get the number of microseconds since boot
u32 getMicroCounter(void)
{
  return us_time;
}



////// SIM ONLY //////////

void ManageTimers(u32 ms_step)
{
  us_time = us_time + ms_step * 1000;
}
