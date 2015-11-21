#include <string.h>
#include <stdint.h>

#include "sha1.h"
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
  uint8_t   checksum[SHA1_BLOCK_SIZE];
};

static const int          BOOT_LOADER_LENGTH = 0x1000;
static const uint32_t     HEADER_SIGNATURE = 0x304D5A43;

static const BootLoaderSignature* IMAGE_HEADER = (BootLoaderSignature*) BOOT_LOADER_LENGTH;

extern "C" void init_data_bss(void);
extern "C" void GoSlow(void);
extern void PowerInit();

static inline void StopDevices() {
  // Magic numbers taken from inital boot settings
  SIM_SCGC4 = 0xF0100030;
  SIM_SCGC5 = 0x00040380 |
    SIM_SCGC5_PORTA_MASK |
    SIM_SCGC5_PORTB_MASK |
    SIM_SCGC5_PORTC_MASK |
    SIM_SCGC5_PORTD_MASK |
    SIM_SCGC5_PORTE_MASK;
  SIM_SCGC6 = 0x00000001;
  SIM_SCGC7 = 0x00000002;
}

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

// This is the remote entry point recovery mode
extern "C" void SVC_Handler(void) { 
  __disable_irq();
  StopDevices();
  GoSlow();
  PowerInit();
  init_data_bss();
  MicroWait(2000000);
  EnterRecovery();
  NVIC_SystemReset();
}

int main (void) {
  TimerInit();

  if (!CheckSig()) {
    SVC_Handler();
  }
  
  SCB->VTOR = (uint32_t) IMAGE_HEADER->vector_tbl;
  IMAGE_HEADER->entry_point();
}
