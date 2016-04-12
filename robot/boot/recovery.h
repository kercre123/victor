#include <stdint.h>

#ifndef __RECOVERY_H
#define __RECOVERY_H

struct BootLoaderSignature {
  uint32_t  sig;
  void (*entry_point)(void);
  uint32_t  vector_tbl;
  uint8_t   *rom_start;
  uint32_t  rom_length;
  uint32_t  checksum;
};

static const int          BOOT_LOADER_LENGTH = 0x1000;
static const uint32_t     HEADER_SIGNATURE = 0x304D5A43;

static const BootLoaderSignature* IMAGE_HEADER = (BootLoaderSignature*) BOOT_LOADER_LENGTH;

void EnterRecovery();

#endif
