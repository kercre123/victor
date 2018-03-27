#include "hal/flash.h"

int main(void)
{
  // Flash Block A new firmware in Block B
  if (!IS_BLOCK_A_VALID)
  {
    s32 i;
    u8* blockB = (u8*)FLASH_BLOCK_B;
    
    FLASH_Unlock();
    
    // Clear all flash pages in Block A (@ 128 KB each)
    // Voltage Range 1 is used because the POR controller does not reset until 2V - cheap insurance!
    FLASH_EraseSector(FLASH_BLOCK_A_SECTOR_0, VoltageRange_1);
    
    // Write 32-bits at a time from the end
    for (i = FLASH_CODE_LENGTH - 1; i >= 0; i -= 1)
    {
      FLASH_ProgramByte(FLASH_BLOCK_A + i, blockB[i]);
    }
    
    FLASH_Lock();
  }
  
  // Change Vector Table Offset Register to match execution in Block A
  SCB->VTOR = FLASH_BLOCK_A;
  void (*BlockAReset)() = FLASH_BLOCK_A_RESET;
  BlockAReset();
}
