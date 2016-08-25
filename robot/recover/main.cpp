#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "hal/portable.h"
#include "hal/hardware.h"

extern "C" const uint8_t __BOOTLOADER[];
extern "C" uint32_t __BOOTLOADER_SIZE;
 
extern "C" {
  void* CRASHLOG_POINTER = NULL;
  void HardFault_Handler(void) {
    NVIC_SystemReset();
  };
}

// This silences exception allocations
extern "C" 
void * __aeabi_vec_ctor_nocookie_nodtor(   void* user_array,
                                           void* (*constructor)(void*),
                                           size_t element_size,
                                           size_t element_count) 

{
    size_t ii = 0;
    char *ptr = (char*) (user_array);
    if ( constructor != NULL )
        for( ; ii != element_count ; ii++, ptr += element_size )
            constructor( ptr );
    return user_array;
}

static void EnterRecoveryMode(void) {
  static uint32_t* recovery_word = (uint32_t*) 0x20001FFC;
  static const uint32_t recovery_value = 0xCAFEBABE;

  *recovery_word = recovery_value;
  NVIC_SystemReset();
}

static const int FLASH_BLOCK_SIZE = 0x800;

int SendCommand()
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

int EraseSector(uint32_t target) {
  // Test for sector erase nessessary
  FTFA->FCCOB0 = 0x09;
  FTFA->FCCOB1 = target >> 16;
  FTFA->FCCOB2 = target >> 8;
  FTFA->FCCOB3 = target;
    
  // Erase block
  return SendCommand();
}

int FlashSector(uint32_t target, const void* src) {
  volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;
  const uint32_t* data = (const uint32_t*) src;

  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t)) {
    // Write program word
    flashcmd[0] = (target & 0xFFFFFF) | 0x06000000;
    flashcmd[1] = data[i];
    
    // Did command succeed
    if (SendCommand()) { 
      return -1;
    }
  }
  
  return 0;
}

int main (void)
{
  // Enable reset filtering
  for (uint32_t target = 0; target < __BOOTLOADER_SIZE; target += FLASH_BLOCK_SIZE) {    
    EraseSector(target);
    FlashSector(target, &__BOOTLOADER[target]);
  }

  EnterRecoveryMode();
}
