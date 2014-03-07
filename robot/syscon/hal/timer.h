#ifndef TIMER_H
#define TIMER_H

#include "anki/cozmo/robot/hal.h"

// Initialize the RTC peripheral
void TimerInit();

// Get the counter with each tick being 120 ns.
u32 GetCounter();

#endif
