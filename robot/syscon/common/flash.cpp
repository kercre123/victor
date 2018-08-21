#include "vectors.h"
#include "flash.h"
#include "power.h"

#include "stm32f0xx.h"

static void waitForFlash(void) {
  while (FLASH->SR & FLASH_SR_BSY) ;
  FLASH->SR = FLASH_SR_EOP;
}

static void unlockFlash(void) {
  waitForFlash();

  if (FLASH->CR & FLASH_CR_LOCK) {
    FLASH->KEYR = FLASH_FKEY1;
    FLASH->KEYR = FLASH_FKEY2;
  }
}

static void lockFlash(void) {
  FLASH->CR |= FLASH_CR_LOCK;
}

void Flash::writeFlash(const void* targ, const void* src, int size) {
  unlockFlash();
  
  FLASH->CR |= FLASH_CR_PG;
  
  const uint16_t* source = (const uint16_t*) src;
  uint16_t* target = (uint16_t*) targ;
  
  while(size > 0) {
    *(target++) = *(source++);
    size -= sizeof(uint16_t);
    waitForFlash();

    uint32_t SR = FLASH->SR;
    uint32_t WRPR = FLASH->WRPR;

    if (SR & FLASH_SR_WRPRTERR) {
      break ;
    }
  }

  FLASH->CR &= ~FLASH_CR_PG;
  lockFlash();
}

extern "C" void EraseFlash() {
  Flash::eraseApplication();
}

void Flash::eraseApplication(void) {
  unlockFlash();
  FLASH->CR |= FLASH_CR_PER;

  for (uint32_t i = 0; i < COZMO_APPLICATION_SIZE; i += FLASH_PAGE_SIZE) {
    uint32_t* target = (uint32_t*)(COZMO_APPLICATION_ADDRESS + i);
    for (int idx = 0; idx < FLASH_PAGE_SIZE / sizeof(uint32_t); idx ++) {
      // Word is erased
      if (~target[idx] == 0) {
        continue;
      }
      
      // Erase this page
      FLASH->AR = (uint32_t) target;
      FLASH->CR |= FLASH_CR_STRT;
      
      waitForFlash();
      break ;
    }
  }

  FLASH->CR &= ~FLASH_CR_PER;

  lockFlash();
}

void Flash::writeFaultReason(FaultType reason) {
  unlockFlash();

  // Write to first available fault slot
  for (int i = 0; i < MAX_FAULT_COUNT; i++) {
    if (APP->faultCounter[i] != FAULT_NONE) continue ;
    writeFlash(&APP->faultCounter[i], &reason, sizeof(reason));
    return ;
  }
}
