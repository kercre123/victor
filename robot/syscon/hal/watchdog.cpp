#include "nrf.h"

#include "watchdog.h"

void Watchdog::init(void) {
  // Setup the watchdog
  NRF_WDT->POWER = 1;
  NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
  NRF_WDT->CRV = 0x8000*5; // 5 seconds before everything explodes
  NRF_WDT->RREN = wdog_channel_mask;
  NRF_WDT->TASKS_START = 1;
}

void Watchdog::kick(uint8_t channel) {
  NRF_WDT->RR[channel] = WDT_RR_RR_Reload;
}
