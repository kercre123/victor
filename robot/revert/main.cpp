#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include <stdint.h>

#include "MK02F12810.h"
#include "bootloader.h"

extern "C" const uint32_t BOOTLOADER_LENGTH;
extern "C" const uint8_t BOOTLOADER_UPDATE[];
static uint8_t* BOOTLOADER = (uint8_t*)0x0000;
static const int FLASH_BLOCK_SIZE = 0x800;
static volatile uint32_t* const  flashcmd = (uint32_t*)&FTFA->FCCOB3;

static int SendCommand()
{
  const int FSTAT_ERROR = FTFA_FSTAT_FPVIOL_MASK | FTFA_FSTAT_ACCERR_MASK | FTFA_FSTAT_FPVIOL_MASK;

  // Wait for idle and clear errors
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;
  FTFA->FSTAT = FSTAT_ERROR;

  // Start flash command (and wait for completion)
  FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;

  // Return if there was an error
  return FTFA->FSTAT & FSTAT_ERROR;
}

static inline bool EraseSector(int target) {
  *flashcmd = (target & 0xFFFFFF) | 0x09000000;
    
  // Erase block
  if (SendCommand()) { 
    return false;
  }
  
  return true;
}

static bool FlashSector(int target, const uint32_t* data)
{
  volatile const uint32_t* original = (uint32_t*)target;

  // Test for sector erase nessessary
  if (!EraseSector(target)) {
    return false;
  }

  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t)) {
    // Program word does not need to be written
    if (data[i] == original[i]) {
      continue ;
    }

    // Write program word
    flashcmd[0] = (target & 0xFFFFFF) | 0x06000000;
    flashcmd[1] = data[i];
    
    // Did command succeed
    if (SendCommand()) { 
      return false;
    }
  }

  return true;
}

static void flash_bootloader(void) {
  __disable_irq();
  for (int i = 0; i < BOOTLOADER_LENGTH; i += FLASH_BLOCK_SIZE) {
    FlashSector(i, (const uint32_t*)&BOOTLOADER_UPDATE[i]);
  }
  NVIC_SystemReset();
}

void update_bootloader(void) {
  for (int i = 0; i < BOOTLOADER_LENGTH; i++) {
    if (BOOTLOADER_UPDATE[i] != BOOTLOADER[i])
    {
      flash_bootloader();
    }
  }
}

int main(void) {
  update_bootloader();
  EraseSector(0x1000);
  NVIC_SystemReset();
  return 0;
}
