#include <string.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "sha1.h"
#include "timer.h"
#include "recovery.h"
#include "../../hal/hardware.h"

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

void StopDevices(void) {
  NRF_RADIO->POWER = 0; // Power down the radio (it uses DMA)
  MicroWait(1000);      // Give it some time to power down
}

void PowerOn(void) {
  // Enable charger
  nrf_gpio_pin_set(PIN_CHARGE_EN);
  nrf_gpio_cfg_output(PIN_CHARGE_EN);
 
  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_PWR_EN);
  nrf_gpio_cfg_output(PIN_PWR_EN);

  // Encoder and headboard power
  nrf_gpio_pin_set(PIN_VDDs_EN);
  nrf_gpio_cfg_output(PIN_VDDs_EN);
}

// This is the remote entry point recovery mode
extern "C" void SVC_Handler(void) {
  __disable_irq();
  StopDevices();
  PowerOn();
  EnterRecovery();
  NVIC_SystemReset();
}

int main (void) {  
  TimerInit();

  if (!CheckSig()) {
    SVC_Handler();
  }
  
  IMAGE_HEADER->entry_point();
}
