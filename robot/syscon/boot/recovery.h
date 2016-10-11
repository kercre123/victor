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
  
static const int          BOOT_HEADER_LOCATION = 0x18000;
static const uint32_t     HEADER_SIGNATURE = 0x304D5A43;

static const BootLoaderSignature* const IMAGE_HEADER = (BootLoaderSignature*) BOOT_HEADER_LOCATION;

static const int MAX_TIMEOUT = 1000000;

extern "C" void EnterRecovery(void);

#endif
