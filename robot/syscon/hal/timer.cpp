#include "timer.h"
#include "nrf.h"

#include "hardware.h"
#include "backpack.h"
#include "cubes.h"

// This is the IRQ handler for various power modes
static void setup_next_main_exec();
void main_execution();

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
  NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE3_Msk |
                       RTC_INTENSET_COMPARE0_Msk |
                       RTC_INTENSET_COMPARE1_Msk |
                       RTC_INTENSET_COMPARE2_Msk;
  
  setup_next_main_exec();

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

static void setup_next_main_exec() {
  static const int PERIOD = CYCLES_MS(5.0f);
  static int target = PERIOD + GetCounter();

  NRF_RTC1->CC[TIMER_CC_MAIN] = target >> 8;

  target += PERIOD;
}

extern "C" void RTC1_IRQHandler() {
  // Main execution will fire
  if (NRF_RTC1->EVENTS_COMPARE[TIMER_CC_MAIN]) {
    NRF_RTC1->EVENTS_COMPARE[TIMER_CC_MAIN] = 0;
    setup_next_main_exec();
    main_execution();
  }

  // Light management loop
  for (int i = 0; i <= 3; i++) {
    if (NRF_RTC1->EVENTS_COMPARE[i]) {
      NRF_RTC1->EVENTS_COMPARE[i] = 0;

      Backpack::update(i);
    }
  }
}
