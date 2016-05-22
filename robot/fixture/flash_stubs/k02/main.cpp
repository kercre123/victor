#include "MK02F12810.h"

#define FLASH_BLOCK_SIZE 0x800

void SendCommand()
{
  const int FSTAT_ERROR = FTFA_FSTAT_FPVIOL_MASK | FTFA_FSTAT_ACCERR_MASK | FTFA_FSTAT_FPVIOL_MASK;

  // Wait for idle and clear errors
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;
  FTFA->FSTAT = FSTAT_ERROR;

  // Start flash command (and wait for completion)
  FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;

  // Wait forever if there was an error - this will cause an SWD timeout/fixture fault
  while (FTFA->FSTAT & FSTAT_ERROR)
    ;
}

void FlashSector(int target, const uint32_t* data)
{
  volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;

  // Erase block
  FTFA->FCCOB0 = 0x09;
  FTFA->FCCOB1 = target >> 16;
  FTFA->FCCOB2 = target >> 8;
  FTFA->FCCOB3 = target;
  SendCommand();
  
  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t)) {
    // Write program word
    flashcmd[0] = (target & 0xFFFFFF) | 0x06000000;
    flashcmd[1] = data[i];
    SendCommand();
  }
  
  // Invalidate all ways in the flash cache
  FMC->PFB0CR = BM_FMC_PFB0CR_CINV_WAY;
}

#define address_  (*(volatile int*)((0x20001000 + FLASH_BLOCK_SIZE)))
#define block_    (const uint32_t*)(0x20001000)

int main(void)
{
  address_ = -1;
  while (1)
  {
    if (address_ != -1)
    {
      if (address_ == -2)
        NVIC_SystemReset();
      FlashSector(address_, block_);
      address_ = -1;
    }
  }
}
