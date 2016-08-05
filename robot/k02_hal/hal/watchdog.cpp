#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "watchdog.h"
#include "spi.h"
#include "uart.h"
#include "dac.h"

#include "anki/cozmo/robot/hal.h"

#include "hal/hardware.h"

static const int totalReset = (1 << WDOG_TOTAL_CHANNELS) - 1;
static int watchdogChannels = 0;

static const uint32_t MAXIMUM_RESET_COUNT = 5;

void Anki::Cozmo::HAL::Watchdog::init(void) {
  static const uint32_t RESET_TIME = 2 * 128;  // 2 seconds (1khz LPO)
  volatile uint16_t* ctrlh = &WDOG_STCTRLH;

  __disable_irq();
  WDOG_UNLOCK = 0xC520;
  WDOG_UNLOCK = 0xD928;
  
  // Using the LPO (1khz clock)
  WDOG_PRESC  = WDOG_PRESC_PRESCVAL(7);
  WDOG_TOVALL = RESET_TIME & 0xFFFF;
  WDOG_TOVALH = RESET_TIME >> 16;
  
  // Turn on the watchdog (with interrupts)
  WDOG_STCTRLH =
    WDOG_STCTRLH_BYTESEL(0x00) |
    WDOG_STCTRLH_IRQRSTEN_MASK |
    WDOG_STCTRLH_ALLOWUPDATE_MASK |
    WDOG_STCTRLH_WDOGEN_MASK |
    0x0100U;

  NVIC_SetPriority(WDOG_EWM_IRQn, 0);
  NVIC_EnableIRQ(WDOG_EWM_IRQn);

  __enable_irq();

  watchdogChannels = 0;
}

void Anki::Cozmo::HAL::Watchdog::suspend(void) {
  __disable_irq();
  WDOG_UNLOCK = 0xC520;
  WDOG_UNLOCK = 0xD928;

  static const uint32_t RESET_TIME = 120 * 128; // 2 minutes (1khz LPO)

  WDOG_PRESC  = WDOG_PRESC_PRESCVAL(7);
  WDOG_TOVALL = RESET_TIME & 0xFFFF;
  WDOG_TOVALH = RESET_TIME >> 16;
  
  WDOG_STCTRLH = WDOG_STCTRLH_BYTESEL(0x00) |
                 WDOG_STCTRLH_ALLOWUPDATE_MASK |
                 WDOG_STCTRLH_WDOGEN_MASK |
                 0x0100U;

  MicroWait(100000); // An appropriate amount of time before refreshing

  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
}

void Anki::Cozmo::HAL::Watchdog::kick(const uint8_t channel) {
  watchdogChannels |= 1 << channel;
}

void Anki::Cozmo::HAL::Watchdog::pet() {
  // Refresh only every 1/8th of a second
  static const int REFRESH_PERIOD = 7440 / 8;
  static int refresh_delay = 0;
  
  if (--refresh_delay > 0 || watchdogChannels != totalReset)  {
    return ;
  }

  watchdogChannels = 0;
  refresh_delay = REFRESH_PERIOD;

  __disable_irq();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  __enable_irq();
}

extern "C"
void WDOG_EWM_IRQHandler(void)
{
  Anki::Cozmo::HAL::DAC::Tone(2.0f);
  if (WDOG_RSTCNT > MAXIMUM_RESET_COUNT) {
    Anki::Cozmo::HAL::SPI::EnterRecoveryMode();
  }
}
