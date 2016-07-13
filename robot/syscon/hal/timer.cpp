#include "timer.h"
#include "nrf.h"

#include "hardware.h"
#include "backpack.h"
#include "cubes.h"

// This is the IRQ handler for various power modes
static void (*irq_handler)(void);

static const int PRESCALAR = 0x10;

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
  NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE0_Msk |
                       RTC_INTENSET_COMPARE1_Msk |
                       RTC_INTENSET_COMPARE2_Msk;
  
  lowPowerMode(true);

  // Configure lower-priority realtime trigger
  NVIC_SetPriority(SWI0_IRQn, RTOS_PRIORITY);
  NVIC_EnableIRQ(SWI0_IRQn);

  NVIC_SetPriority(RTC1_IRQn, TIMER_PRIORITY);
  NVIC_EnableIRQ(RTC1_IRQn);
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

static void low_power_timer(void) {
  NRF_RTC1->EVENTS_COMPARE[3] = 0;
  NRF_RTC1->CC[3] += PRESCALAR;

  NVIC_SetPendingIRQ(SWI0_IRQn);
}

static void high_power_timer(void) {
  NRF_RTC1->EVENTS_TICK = 0;

  Radio::manage();
  NVIC_SetPendingIRQ(SWI0_IRQn);
}

void Timer::lowPowerMode(bool lowPower) {
  // Do not manage radio in low power mode
  NRF_RTC1->TASKS_STOP = 1;

  // Clear existing interrupt mode
  NRF_RTC1->INTENCLR = RTC_INTENCLR_COMPARE3_Msk | RTC_INTENCLR_TICK_Msk;
  
  // Setup new mode (compare used for prescalar)
  if (lowPower) {
    NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE3_Msk;
    NRF_RTC1->EVENTS_COMPARE[3] = 0;
    NRF_RTC1->CC[3] = NRF_RTC1->COUNTER + PRESCALAR;

    irq_handler = low_power_timer;
  } else {
    NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;
    NRF_RTC1->EVENTS_TICK = 0;

    irq_handler = high_power_timer;
  }

  NRF_RTC1->TASKS_START = 1;
}

extern "C" void RTC1_IRQHandler() {
  for (int i = 0; i < 3; i++) {
    if (NRF_RTC1->EVENTS_COMPARE[i]) {
      NRF_RTC1->EVENTS_COMPARE[i] = 0;
      
      Backpack::update(i);
    }
  }

  irq_handler();
}
