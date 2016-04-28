#include <stdint.h>

#ifndef __RECOVERY_H
#define __RECOVERY_H

struct BootLoaderSignature {
  void (*entry_point)(void);
  uint32_t  sig;
  uint32_t  vector_tbl;
  uint8_t   *rom_start;
  uint32_t  rom_length;
  uint32_t  checksum;
  uint32_t  evil_word;
};

static const int          BOOT_LOADER_LENGTH = 0x1000;
static const uint32_t     HEADER_SIGNATURE = 0x304D5A43;

static const int FLASH_BLOCK_SIZE = 0x800;
static const uint32_t recovery_value = 0xCAFEBABE;
static uint32_t * const recovery_word = (uint32_t*) 0x20001FFC;

static const BootLoaderSignature * const IMAGE_HEADER = (BootLoaderSignature*) BOOT_LOADER_LENGTH;

void EnterRecovery();

#endif
