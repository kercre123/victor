#include <string.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "../hal/hardware.h"

extern "C" const uint32_t LowerSector;

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
}


__attribute((section("RAMCODE"))) void turn_on(uint32_t pin) {
  nrf_gpio_pin_clear(PIN_LED1);
  nrf_gpio_pin_clear(PIN_LED2);
  nrf_gpio_pin_clear(PIN_LED3);
  nrf_gpio_pin_clear(PIN_LED4);

  nrf_gpio_pin_set(pin);

  nrf_gpio_cfg_output(PIN_LED1);
  nrf_gpio_cfg_output(PIN_LED2);
  nrf_gpio_cfg_output(PIN_LED3);
  nrf_gpio_cfg_output(PIN_LED4);
}

__attribute((section("RAMCODE"))) void FlashSector()
{
  const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
  const int TOTAL_UPLOAD_SIZE = 0x2000;
  const int TOTAL_WORD_COUNT = TOTAL_UPLOAD_SIZE / sizeof(uint32_t);
  volatile uint32_t* original = (uint32_t*)0;
  volatile uint32_t* BOOTADDR = (uint32_t*)(NRF_UICR_BASE + 0x14);
  const uint32_t* source = &LowerSector;
  
  turn_on(PIN_LED3);

  // Configure the bootloader UICR
  if (*BOOTADDR == 0xFFFFFFFF) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    *BOOTADDR = 0x1F000;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }
  
  // Erase the sectors
  for (int location = 0; location < TOTAL_UPLOAD_SIZE; location += FLASH_BLOCK_SIZE) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->ERASEPAGE = location;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }

  // Copy memory into boot sector
  for (int i = 0; i < TOTAL_WORD_COUNT; i++) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    original[i] = source[i];
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }

  turn_on(PIN_LED4);

  NVIC_SystemReset();
}

int main (void) {
  turn_on(PIN_LED2);
  TimerInit();
  for(int i = 1000000; i > 0; i--) __asm { nop };  

  FlashSector();
}
