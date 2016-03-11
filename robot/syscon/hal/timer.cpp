#include "timer.h"
#include "nrf.h"

#include "hardware.h"
#include "backpack.h"
#include "rtos.h"

void Timer::init()
{
  // Enabling constant latency as indicated by PAN 11 "HFCLK: Base current with HFCLK 
  // running is too high" found at Product Anomaly document found at
  // https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822/#Downloads
  
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
  
  // Configure the interrupts
  NRF_RTC1->EVTENSET = RTC_EVTENCLR_TICK_Msk;
  NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;

  NVIC_SetPriority(RTC1_IRQn, TIMER_PRIORITY);
  NVIC_EnableIRQ(RTC1_IRQn);
}

void Timer::start(void) {
  // Start the RTC
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
  if (!NRF_RTC1->EVENTS_TICK)
    return ;

  NRF_RTC1->EVENTS_TICK = 0;
  
  RTOS::manage();
  Backpack::update();
}
