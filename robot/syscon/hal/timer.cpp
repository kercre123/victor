#include "timer.h"
#include "nrf.h"

#include "hardware.h"
#include "backpack.h"
#include "cubes.h"

// This is the IRQ handler for various power modes
extern void main_execution();

static const int PERIOD = CYCLES_MS(5.0f) >> 8;

void Timer::init()
{
  // This setting will ensure correct behaviour when routing TIMER events through 
  // PPI and low power mode simultaneously.
  NRF_POWER->TASKS_CONSTLAT = 1;

  // Power on the peripheral
  NRF_RTC1->POWER = 1;
  
  // Stop the RTC so it can be reset
  NRF_RTC1->TASKS_STOP = 1;
  NRF_RTC1->TASKS_CLEAR = 1;
  
  // NOTE: When using the LFCLK with prescaler = 0, we only get 30.517 us
  // resolution. This should still provide enough for this chip/board.
  NRF_RTC1->PRESCALER = 0;

  NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE0_Msk;
  NRF_RTC1->CC[0] = 10; // In the near future

  // Configure lower-priority realtime trigger
  NVIC_SetPriority(RTC1_IRQn, TIMER_PRIORITY);
  NVIC_EnableIRQ(RTC1_IRQn);

  NRF_RTC1->TASKS_START = 1;
}

__asm void MicroWait(u32 microseconds)
{
loop
    SUBS r0, r0, #1
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    BNE loop
    BX lr
}


extern "C" void RTC1_IRQHandler() {
  if (NRF_RTC1->EVENTS_COMPARE[0]) {
    NRF_RTC1->EVENTS_COMPARE[0] = 0;

    NRF_RTC1->CC[0] += PERIOD;
    main_execution();
  }
}
