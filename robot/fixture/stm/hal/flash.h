#ifndef FLASH_H
#define FLASH_H

#include "portable.h"

#define MAGIC_ANKI                0x494b4e41    // {'A','N','K','I'}
#define MAGIC_IKNA                0x414E4b49    // {'I','K','N','A'} or 'ANKI'

#define FACTORY_MAGIC_BLOCK_1     MAGIC_IKNA
#define FACTORY_MAGIC_BLOCK_2     MAGIC_ANKI

#define FLASH_BOOTLOADER_LENGTH   0x00004000    // 16 KB
#define FLASH_BLOCK_LENGTH        0x000A0000    // 640KB ought to be enough
#define FLASH_CODE_LENGTH         0x00020000    // 128KB at the front is reserved for code

#define FLASH_BOOTLOADER          0x08000000
#define FLASH_BOOTLOADER_SERIAL   0x08003FFC    // Last 4 bytes of bootloader flash
#define FLASH_PARAMS              0x08004000    // 16KB page of flash parameters

#define FLASH_SERIAL_BITS         0x08010000    // 524288 serial numbers
  
#define FLASH_BLOCK_BOOT          FLASH_Sector_0
#define FLASH_BLOCK_PARAMS        FLASH_Sector_1

// The first 128KB of the binary must contain all the code - we keep a backup in B to prevent bricked updates
// This could have been 768KB, since we never used 0x08020000
#define FLASH_BLOCK_A             0x08060000
#define FLASH_BLOCK_A_SECTOR_0    FLASH_Sector_7    // 640KB total
#define FLASH_BLOCK_SECTOR_1      FLASH_Sector_8
#define FLASH_BLOCK_SECTOR_2      FLASH_Sector_9
#define FLASH_BLOCK_SECTOR_3      FLASH_Sector_10
#define FLASH_BLOCK_SECTOR_4      FLASH_Sector_11
#define FLASH_BLOCK_A_RESET       (void (*)())(*(uint32_t*)(FLASH_BLOCK_A + 4))
#define FLASH_BLOCK_A_MAGIC       (*(uint32_t*)FLASH_BLOCK_A)
#define FLASH_BLOCK_B             0x08040000
#define FLASH_BLOCK_B_SECTOR_0    FLASH_Sector_6

// Used by bootloader to check validity of firmware
#define IS_BLOCK_A_VALID          (FLASH_BLOCK_A_MAGIC == MAGIC_ANKI)

#define FIXTURE_SERIAL            (*(u32*)FLASH_BOOTLOADER_SERIAL)
#define FIXTURE_VERSION           (*(u32*)(FLASH_BLOCK_A + 28))

void FlashProgress(int percent);
void DecodeAndFlash(void);
void WritePreTestData(void);
void WriteFactoryBlockErrorCode(int errorCode);
void PrintFactoryBlockInfo(void);

// This is a list of parameters stored in flash memory
// Modify the RAM copy in g_flashParams, then call StoreParams();
typedef struct
{
  int fixtureTypeOverride;
} FlashParams;

extern FlashParams g_flashParams;
void StoreParams(void);

#endif
