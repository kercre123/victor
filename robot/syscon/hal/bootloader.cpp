#include <stdint.h>

#include "nrf.h"

#include "rtos.h"
#include "battery.h"
#include "bootloader.h"

extern "C" const uint32_t BOOTLOADER_LENGTH;
extern "C" const uint8_t BOOTLOADER_UPDATE[];
static uint8_t* BOOTLOADER = (uint8_t*)0;

void schedule(void);
void recover(void* userdata);

static const int CAPTURE_OFFSET = CYCLES_MS(5000.0f);

void Bootloader::init(void) {
  for (int i = 0; i < BOOTLOADER_LENGTH; i++) {
    if (BOOTLOADER_UPDATE[i] != BOOTLOADER[i]) {    
      schedule();
      return ;
    }
  }
}

void schedule(void) {
  RTOS::schedule(recover, CAPTURE_OFFSET, NULL, false);
}

void flash_data(uint8_t* target, const uint8_t* source, int size)
{
  const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
  
  // Erase the sectors and continue
  for (int i = 0; i < size; i += FLASH_BLOCK_SIZE) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->ERASEPAGE = (int)&target[i];
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }

  // Reflash the memory
  uint32_t* a = (uint32_t*) target;
  uint32_t* b = (uint32_t*) source;
  
  for (int i = 0; i < size; i += sizeof(uint32_t)) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    *(a++) = *(b++);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }
}

void recover(void* userdata) {
  // Refuse to upgrade bootloader until we are on charge contacts
  if (!Battery::onContacts) {
    schedule();
    return ;
  }

  __disable_irq();
  flash_data(BOOTLOADER, BOOTLOADER_UPDATE, BOOTLOADER_LENGTH);
  NVIC_SystemReset();
}
