#include <string.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"
#include "nrf_mbr.h"

#include "crc32.h"
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
  uint32_t  checksum;
};
  
static const int          BOOT_HEADER_LOCATION = 0x18000;
static const uint32_t     HEADER_SIGNATURE = 0x304D5A43;

static const BootLoaderSignature* IMAGE_HEADER = (BootLoaderSignature*) BOOT_HEADER_LOCATION;

// Boot loader info
bool CheckSig(void) {
  if (IMAGE_HEADER->sig != HEADER_SIGNATURE) return false;
  
  // Compute signature length
	uint32_t crc = calc_crc32(IMAGE_HEADER->rom_start, IMAGE_HEADER->rom_length);
	
	return crc == IMAGE_HEADER->checksum;
}

extern void BlinkALot(void);

int main (void) {
	__disable_irq();

  // Configure our system clock
	TimerInit();

	// Power on the system
  Battery::init();
	Battery::powerOn();

  // Display our reboot pattern
	BlinkALot();

	// Do recovery until our signature is okay
  do
  {
    EnterRecovery();
  } while (!CheckSig());

  __enable_irq();
  sd_mbr_command_t cmd;
  cmd.command = SD_MBR_COMMAND_INIT_SD;
  sd_mbr_command(&cmd);

  sd_softdevice_vector_table_base_set(IMAGE_HEADER->vector_tbl);
  IMAGE_HEADER->entry_point();
}
