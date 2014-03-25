#ifndef TIMER_H
#define TIMER_H

#include "portable.h"

#define US_PER_COUNT (8)

// Initialize the RTC peripheral
void TimerInit();

// Get the counter with each tick being 120 ns.
u32 GetCounter();

// Wait in a loop for the specified amount of microseconds
void MicroWait(u32 microseconds);

#endif
