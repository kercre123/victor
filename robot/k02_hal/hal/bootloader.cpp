// SAFELY apply binary patch(es) to the bootloader by flipping ONLY ONE BIT in the existing code
// If the bit is not flipped, old bootloader runs - once flipped, new, patched, bootloader runs
#include <stdint.h>
#include "MK02F12810.h"
#include "portable.h"

const int FSTAT_ERROR = FTFA_FSTAT_FPVIOL_MASK | FTFA_FSTAT_ACCERR_MASK | FTFA_FSTAT_FPVIOL_MASK;

// Patch old bootloaders to pulldown UART_TX at all times
const int CHECK_ADDR = 0xAA0, CHECK_DATA = 0x2000FEA7;  // Check that main()/UART::init are where we need
static const int BOOT_PATCH[] =  
{ 
  // Patch starts at 0xEAE, taking control just before MicroWait(5000) in Power::init()
  0xEAC, 0xB006FFFF,    // ADD sp,sp,#0x18                    Exit Power::init() back to main(), by silently popping (r4-r8,pc)
  0xEB0, 0xFC9EF7FF,    // BL.W UART::init (0x7F0)            UART::init();  Next in main(), leaves r0/r1 set for PORTD_PCR7/UART_TX
  0xEB4, 0x61C11C89,    // ADDS r1,r1,#2 | STR r1,[r0,#0x1C]  PORTD_PCR7 = PORT_PCR_MUX(3) + SourcePullDown;  (+2)
  0xEB8, 0xE5F2E7FF,    // NOP/NextPatch | B 0xAA2            Resume main() just after UART::init();
  // NOTE:  For future patches, make a 1-bit change from E7FF (NOP) to E7BF (B 0xE3A)
  
  // VERY LAST THING:  Change ONE BIT to jump-to-patch, by overwriting MicroWait(5000) in Power::init()
  0xA28, 0x3088E241,    // B 0xEAE                            Jump to patch (was 0x3088F241 - mov r0,#5000)
  0                     // End of patch
};

// Must wait for flash while looping in RAM, to prevent crashes
CODERAM void FlashCommand(uint16_t timeout)
{
  FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;             // Kick off command
  while (--timeout && (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT))  ;   // Wait for done
}

// Apply a bootloader patch as safely as we can
void UpdateBootloader()
{
  // Make sure this is the right bootloader for BOOT_PATCH
  if (*(int*)CHECK_ADDR != CHECK_DATA)
    return;
  
  // Verify the whole patch every time - only flash if something's missing
  const int *patch = BOOT_PATCH;
  int attempts = 25;             // Don't loop forever
  while (*patch && --attempts)
  {
    int addr = patch[0], data = patch[1];
    int before = *(volatile int*)addr;
 
    // If not possible to program this word, abort the patch!
    if ((before & data) != data)
      return;
          
    // If already patched, skip this word and check the next
    if (before == data)
    {
      patch += 2;
    } else {
      // Patch the indicated word - we'll recheck it next iteration, so we can retry in case of error
      volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;
      flashcmd[0] = addr | 0x06000000;          // Write word (0x6)
      flashcmd[1] = ~before | data;             // Avoid double-programming 0s (per datasheet): ~1->0 = 0
      FTFA->FSTAT = FSTAT_ERROR;                // Clear errors
      FlashCommand(145 * 100);                  // Max time is 145uS at 100MHz
      FMC->PFB0CR = BM_FMC_PFB0CR_CINV_WAY;     // Invalidate the flash cache
    }
  }
}
