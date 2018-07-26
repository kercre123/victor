
#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

#include "hardware.h"
#include "messages.h"

static const uint32_t TARGET_EXEC_PERIOD = 200;
static const uint32_t MICROSEC_TO_SYSTICK = (SYSTEM_CLOCK / 1000000) << 8;

// Audio timing is the primary clock definition
static const int AUDIO_SHIFT      = 3; // 3 = 3000000hz
static const int AUDIO_PRESCALE   = 2 << AUDIO_SHIFT;
static const int AUDIO_DATA_CLOCK = SYSTEM_CLOCK / AUDIO_PRESCALE;

static const int AUDIO_DECIMATION = 96;

// This calculates the main execution timing to fit audio into even chunks
static const uint16_t MAIN_EXEC_PRESCALE = 4; // Timer prescale
static const uint16_t MAIN_EXEC_OVERFLOW = AUDIO_SAMPLES_PER_FRAME * AUDIO_DECIMATION * 2 * AUDIO_PRESCALE / MAIN_EXEC_PRESCALE;

namespace Timer {
  extern uint32_t clock;
  void init();

  static inline uint32_t getTime(void) {
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
