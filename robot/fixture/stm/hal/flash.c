#include <string.h>
#include "app.h"
#include "console.h"
#include "crypto/crypto.h"
#include "flash.h"
#include "fixture.h"
#include "timer.h"

FlashParams g_flashParams;

static u8 m_bitMap[TARGET_MAX_BLOCK];
static u8 m_decryptedBuffer[SAFE_BLOCK_SIZE];
static u32 m_guid = 0;
static u32 m_blocksDone = 0;

void FlashProgress(int percent)
{
}

void DecodeAndFlash(void)
{
  u8* buffer = (u8*)&app_global_buffer[0];
  memcpy(m_decryptedBuffer, buffer, SAFE_BLOCK_SIZE);
  safe_header* header = (safe_header*)m_decryptedBuffer;
  
  if (m_blocksDone == 0)
  {
    m_guid = header->guid;
  }
  
  // Verify this is a valid block
  int blockIndex = header->blockFlags & BLOCK_FLAG_BLOCK;
  if (blockIndex < TARGET_MAX_BLOCK &&
      !m_bitMap[blockIndex] &&
      header->guid == m_guid &&
      !opensafeDecryptBlock(header))
  {
    u32 i;
    
    // Write the decrypted program to flash
    // The code is backed up in block B, but the rest of the payload goes to block A
    u32 address = (SAFE_PAYLOAD_SIZE * blockIndex);
    if (address >= FLASH_CODE_LENGTH)
      address += FLASH_BLOCK_A;
    else
      address += FLASH_BLOCK_B;
    //SlowPrintf("start: %08X @ %d\n", address, Timer::get());
    
    if (!(blockIndex & 0x1f))
      FlashProgress(50 + (blockIndex / 32) * 5);
    
    __disable_irq();
    FLASH_Unlock();

    for (i = 0; i < SAFE_PAYLOAD_SIZE; i += 4, address += 4)
    {
      //FLASH_ProgramByte(address, m_decryptedBuffer[i + SAFE_HEADER_SIZE]);
      FLASH_ProgramWord(address, *((u32*)(m_decryptedBuffer + i + SAFE_HEADER_SIZE)));
    }
    FLASH_Lock();
    __enable_irq();

    // Acknowledge back to the PC
    ConsolePutChar('1');
    
    if (header->blockFlags & BLOCK_FLAG_LAST)
    {
      // Zero out 'ANKI'
      __disable_irq();
      FLASH_Unlock();
      FLASH_ProgramByte(FLASH_BLOCK_A, 0);
      FLASH_ProgramByte(FLASH_BLOCK_A+1, 0);
      FLASH_ProgramByte(FLASH_BLOCK_A+2, 0);
      FLASH_ProgramByte(FLASH_BLOCK_A+3, 0);
      FLASH_Lock();
      __enable_irq();
      
      FlashProgress(100);
      //SlowPutString("Resetting\n\n");
      
      // Force reset
      SCB->VTOR = FLASH_BOOTLOADER;
      NVIC_SystemReset();
    }
    m_bitMap[blockIndex] = 1;
    m_blocksDone++;
  } else {
    //SlowPutString("Bad flash block\n");
  }
}
