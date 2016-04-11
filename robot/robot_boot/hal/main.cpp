#include <string.h>
#include <stdint.h>
#include "spi.h"

#include "power.h"
#include "crc32.h"
#include "timer.h"
#include "recovery.h"
#include "MK02F12810.h"

// Basic abstractions for low level I/O on our processor
#define GPIO_SET(gp, pin)                (gp)->PSOR = (pin)
#define GPIO_RESET(gp, pin)              (gp)->PCOR = (pin)
#define GPIO_READ(gp)                    (gp)->PDIR

// Note:  These are not interrupt-safe, so do not use in main thead except during init()
#define GPIO_IN(gp, pin)                 (gp)->PDDR &= ~(pin)
#define GPIO_OUT(gp, pin)                (gp)->PDDR |= (pin)

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

extern "C" void init_data_bss(void);
extern "C" void GoSlow(void);
extern void PowerInit();

bool CheckSig(void) {
  if (IMAGE_HEADER->sig != HEADER_SIGNATURE) return false;

  unsigned int addr = (unsigned int) IMAGE_HEADER->rom_start;
  unsigned int length = IMAGE_HEADER->rom_length;
  
  if (addr + length > 0x10000) {
    return false;
  }

  // Compute signature length
  uint32_t crc = calc_crc32(IMAGE_HEADER->rom_start, IMAGE_HEADER->rom_length);
	
	return crc == IMAGE_HEADER->checksum;
}

static uint32_t* recovery_word = (uint32_t*) 0x20001FFC;
static const uint32_t recovery_value = 0xCAFEBABE;

// This is the remote entry point recovery mode
int main (void) {
	using namespace Anki::Cozmo::HAL;
	
	__disable_irq();

	TimerInit();
	Power::init();
	SPI::init();

  if (*recovery_word == recovery_value || !CheckSig()) {
		*recovery_word = 0;
		EnterRecovery();
  }
  
	SCB->VTOR = (uint32_t) IMAGE_HEADER->vector_tbl;
  IMAGE_HEADER->entry_point();
}
