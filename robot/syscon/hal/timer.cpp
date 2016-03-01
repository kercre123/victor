#include "timer.h"
#include "nrf.h"

void Timer::init()
{
  // Enabling constant latency as indicated by PAN 11 "HFCLK: Base current with HFCLK 
  // running is too high" found at Product Anomaly document found at
  // https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822/#Downloads
  
  // This setting will ensure correct behaviour when routing TIMER events through 
  // PPI and low power mode simultaneously.

  NRF_POWER->TASKS_CONSTLAT = 1;
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
