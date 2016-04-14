#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "watchdog.h"

static const int totalReset = (1 << WDOG_TOTAL_CHANNELS) - 1;
static int watchdogChannels = 0;

void Anki::Cozmo::HAL::Watchdog::init(void) {
  static const uint32_t RESET_TIME = 2048;  // 2seconds (1khz LPO)
  
  // Using the LPO (1khz clock)
  WDOG_TOVALL = RESET_TIME & 0xFFFF;
  WDOG_TOVALH = RESET_TIME >> 16;
  WDOG_PRESC  = 0;
  
  WDOG_STCTRLH = WDOG_STCTRLH_WDOGEN_MASK; // Turn on the watchdog

  watchdogChannels = totalReset;
}

void Anki::Cozmo::HAL::Watchdog::kick(const uint8_t channel) {
  watchdogChannels &= ~(1 << channel);
  
  if (watchdogChannels)  {
    return ;
  }
  
  watchdogChannels = totalReset;

  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
}
