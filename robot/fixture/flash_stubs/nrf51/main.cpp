#include "nrf.h"
#include "nrf_gpio.h"

#define FLASH_BLOCK_SIZE 0x400
#define WAITING_FOR_BLOCK -1

bool FlashSector(int target, const uint32_t* data)
{
  volatile uint32_t* original = (uint32_t*)target;

  // Erase the sector and continue
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->ERASEPAGE = target;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;

  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t))
  {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    original[i] = data[i];
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }

  return true;
}

#define address_  (*(volatile int*)((0x20001000 + FLASH_BLOCK_SIZE)))
#define block_    (const uint32_t*)(0x20001000)

int main(void)
{  
  // Wait for the external oscillator to start (simple crystal test)
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (!NRF_CLOCK->EVENTS_HFCLKSTARTED)
    ;
   
  address_ = WAITING_FOR_BLOCK;

  // Poll for incoming blocks, flash them as they arrive
  while (1)
  {
    if (address_ != WAITING_FOR_BLOCK)
    {
      FlashSector(address_, block_);
      address_ = WAITING_FOR_BLOCK;
    }
  }
}
