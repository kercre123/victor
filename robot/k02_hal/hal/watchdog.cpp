#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "watchdog.h"
#include "spi.h"
#include "uart.h"

#include "hal/hardware.h"

static const int totalReset = (1 << WDOG_TOTAL_CHANNELS) - 1;
static int watchdogChannels = 0;

static const uint32_t TARGET_MAGIC = 'ANKI';
static const uint32_t MAXIMUM_RESET_COUNT = 10;

void Anki::Cozmo::HAL::Watchdog::init(void) {
  static const uint32_t RESET_TIME = 1 * 1024;  // 5 seconds (1khz LPO)
  volatile uint16_t* ctrlh = &WDOG_STCTRLH;
  
  __disable_irq();
  WDOG_UNLOCK = 0xC520;
  WDOG_UNLOCK = 0xD928;
  
  // Using the LPO (1khz clock)
  WDOG_TOVALL = RESET_TIME & 0xFFFF;
  WDOG_TOVALH = RESET_TIME >> 16;
  WDOG_PRESC  = 0;
  
  // Turn on the watchdog (with interrupts)
  WDOG_STCTRLH = 
    WDOG_STCTRLH_ALLOWUPDATE_MASK |
    WDOG_STCTRLH_WDOGEN_MASK | 
    WDOG_STCTRLH_IRQRSTEN_MASK; 

  NVIC_SetPriority(WDOG_EWM_IRQn, 0);
  NVIC_EnableIRQ(WDOG_EWM_IRQn);

  __enable_irq();

  watchdogChannels = 0;

  if (*reset_magic != TARGET_MAGIC) {
    *reset_magic = TARGET_MAGIC;
    *reset_count = 0;
  }
}

void Anki::Cozmo::HAL::Watchdog::kick(const uint8_t channel) {
  watchdogChannels |= 1 << channel;
}

void Anki::Cozmo::HAL::Watchdog::pet() {
  if (watchdogChannels != totalReset)  {
    return ;
  }

  watchdogChannels = 0;

  __disable_irq();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  __enable_irq();
}

extern "C"
void WDOG_EWM_IRQHandler(void)
{
  if (++(*reset_count) > MAXIMUM_RESET_COUNT) {
    *reset_count = 0;
    Anki::Cozmo::HAL::SPI::EnterRecoveryMode();
  }
}
