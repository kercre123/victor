#ifndef FLASH_H
#define FLASH_H

#include "hal/portable.h"

#define MAGIC_ANKI                0x494b4e41    // {'A','N','K','I'}
#define MAGIC_IKNA                0x414E4b49    // {'I','K','N','A'} or 'ANKI'

#define FACTORY_MAGIC_BLOCK_1     MAGIC_IKNA
#define FACTORY_MAGIC_BLOCK_2     MAGIC_ANKI

#define FLASH_BOOTLOADER_LENGTH   0x00004000    // 16 KB
#define FLASH_BLOCK_LENGTH        0x00060000    // 384 KB

#define FLASH_BOOTLOADER          0x08000000
#define FLASH_BOOTLOADER_SERIAL   0x08003FFC    // Last 4 bytes of bootloader flash
#define FLASH_PARAMS              0x08004000    // 16KB page of flash parameters

#define FLASH_SERIAL_BITS         0x08010000    // 524288 serial numbers
  
#define FLASH_BLOCK_BOOT          FLASH_Sector_0
#define FLASH_BLOCK_PARAMS        FLASH_Sector_1
#define FLASH_BLOCK_A             0x080A0000
#define FLASH_BLOCK_A_SECTOR_0    FLASH_Sector_9
#define FLASH_BLOCK_A_SECTOR_1    FLASH_Sector_10
#define FLASH_BLOCK_A_SECTOR_2    FLASH_Sector_11
#define FLASH_BLOCK_A_RESET       (void (*)())(*(uint32_t*)(FLASH_BLOCK_A + 4))
#define FLASH_BLOCK_A_MAGIC       (*(uint32_t*)FLASH_BLOCK_A)
#define FLASH_BLOCK_B             0x08040000
#define FLASH_BLOCK_B_SECTOR_0    FLASH_Sector_6
#define FLASH_BLOCK_B_SECTOR_1    FLASH_Sector_7
#define FLASH_BLOCK_B_SECTOR_2    FLASH_Sector_8

// Used by bootloader to check validity of firmware
#define IS_BLOCK_A_VALID          (FLASH_BLOCK_A_MAGIC == MAGIC_ANKI)

#define FIXTURE_SERIAL            (*(u32*)FLASH_BOOTLOADER_SERIAL)
#define FIXTURE_VERSION           (*(u32*)(FLASH_BLOCK_A + 28))

void DecodeAndFlash(void);
void WritePreTestData(void);
void WriteFactoryBlockErrorCode(error_t errorCode);
void PrintFactoryBlockInfo(void);

// This is a list of parameters stored in flash memory
// Modify the RAM copy in g_flashParams, then call StoreParams();
typedef struct
{
} FlashParams;

extern FlashParams g_flashParams;
void StoreParams(void);

#endif
