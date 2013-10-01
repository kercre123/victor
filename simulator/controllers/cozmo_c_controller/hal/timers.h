#ifndef __TIMERS_H
#define __TIMERS_H

#include "cozmoTypes.h"

// Initialize system timers, excluding watchdog
void InitTimers(void);

// The microsecond clock counts 1 tick per microsecond after startup
// It will wrap every 71.58 minutes
u32 getMicroCounter(void);


#endif
