#include <stdint.h>
#include "timer.h"
#include "nrf.h"

void TimerInit()
{
  // The synthesized LFCLK requires the 16MHz HFCLK to be running, since there's no
  // external crystal/oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  
  // Wait for the external oscillator to start
  while (!NRF_CLOCK->EVENTS_HFCLKSTARTED)
    ;
  
  // Enabling constant latency as indicated by PAN 11 "HFCLK: Base current with HFCLK 
  // running is too high" found at Product Anomaly document found at
  // https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822/#Downloads
  
  // This setting will ensure correct behaviour when routing TIMER events through 
  // PPI and low power mode simultaneously.
  NRF_POWER->TASKS_CONSTLAT = 1;
  
  // Change the source for LFCLK
  NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_Synth;
  
  // Start the LFCLK
  NRF_CLOCK->TASKS_LFCLKSTART = 1;
  
  // Power on the peripheral
  NRF_RTC1->POWER = 1;
  
  // Stop the RTC so it can be reset
  NRF_RTC1->TASKS_STOP = 1;
  NRF_RTC1->TASKS_CLEAR = 1;
  
  // NOTE: When using the LFCLK with prescaler = 0, we only get 30.517 us
  // resolution. This should still provide enough for this chip/board.  
  NRF_RTC1->PRESCALER = 0;
  
  // Start the RTC
  NRF_RTC1->TASKS_START = 1;
}

__asm void MicroWait(uint32_t microseconds)
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
