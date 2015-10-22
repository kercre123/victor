#ifndef __TIMERS_H
#define __TIMERS_H

#include "stm32f2xx.h"

// Initialize system timers, excluding watchdog
void InitTimers(void);
// Wait the specified number of microseconds
void MicroWait(u32 us);

// The microsecond clock counts 1 tick per microsecond after startup
// It will wrap every 71.58 minutes
u32 getMicroCounter(void);

// This version of the microsecond clock saves CPU in tight hardware polling loops
// However, it wraps every 65ms, so is only good for measuring tiny intervals
#define getShortMicroCounter() (u16)(TIM6->CNT)

#endif
