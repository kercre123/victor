#include <string.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"
#include "nrf_mbr.h"

#include "sha1.h"
#include "timer.h"
#include "recovery.h"
#include "battery.h"
#include "hal/hardware.h"
#include "anki/cozmo/robot/spineData.h"

#include "anki/cozmo/robot/rec_protocol.h"

// These are all the magic numbers for the boot loader
struct BootLoaderSignature {
  uint32_t  sig;
  void (*entry_point)(void);
  uint32_t  vector_tbl;
  uint8_t   *rom_start;
  uint32_t  rom_length;
  uint8_t   checksum[SHA1_BLOCK_SIZE];
};
  
static const int          BOOT_HEADER_LOCATION = 0x18000;
static const uint32_t     HEADER_SIGNATURE = 0x304D5A43;

static const BootLoaderSignature* IMAGE_HEADER = (BootLoaderSignature*) BOOT_HEADER_LOCATION;

__attribute((at(0x1FFD0))) static const uint32_t AES_KEY[] = {
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF
};

__attribute((at(NRF_UICR_BASE + 0x14))) static const uint32_t BOOTLOADERADDR = 0x1F000;

uint32_t* MAGIC_LOCATION = (uint32_t*) 0x20003FFC;

// Boot loader info
bool CheckSig(void) {
  if (IMAGE_HEADER->sig != HEADER_SIGNATURE) return false;
  
  // Compute signature length
  SHA1_CTX ctx;
  sha1_init(&ctx);
  sha1_update(&ctx, IMAGE_HEADER->rom_start, IMAGE_HEADER->rom_length);

  uint8_t sig[SHA1_BLOCK_SIZE];
  sha1_final(&ctx, sig);

  for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
    if (sig[i] != IMAGE_HEADER->checksum[i]) return false;
  }

  return true;
}

int main (void) {
  TimerInit();

  if (*MAGIC_LOCATION == SPI_ENTER_RECOVERY || !CheckSig()) {
    *MAGIC_LOCATION = 0;
    
    Battery::init();
    EnterRecovery();
    NVIC_SystemReset();
  }

  __enable_irq();
  sd_mbr_command_t cmd;
  cmd.command = SD_MBR_COMMAND_INIT_SD;
  sd_mbr_command(&cmd);

  sd_softdevice_vector_table_base_set(IMAGE_HEADER->vector_tbl);
  IMAGE_HEADER->entry_point();
}
