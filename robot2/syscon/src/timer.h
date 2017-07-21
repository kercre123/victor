#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

#include "hardware.h"

static const uint32_t MAIN_EXEC_TICK  = 1000000; // Microseconds
static const uint32_t MICROSEC_TO_SYSTICK = (SYSTEM_CLOCK / 1000000) << 8;

namespace Timer {
  extern uint32_t clock;
  void init();

  static uint32_t getTime(void) {
    return SysTick->VAL << 8;
  }
}

static inline void MicroWait(uint32_t microsecs) {
  int32_t target = Timer::getTime() + microsecs * MICROSEC_TO_SYSTICK;
  for (;;) {
    int32_t delta = target - Timer::getTime();
    if (delta < 0) return ;
  }
}

#endif
