#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"

// Imports from assembly
struct OTA_Program {
  void*     load_address;
  uint32_t  program_size;
  uint8_t   program[];
};

extern "C" {
  extern const OTA_Program OTA_IMAGE;
  void JumpToOTA(void);
}

void EnterOTA(void) {
  __disable_irq();

  // Setup our in-memory function to allow us to OTA without rebooting
  memcpy(OTA_IMAGE.load_address, OTA_IMAGE.program, OTA_IMAGE.program_size);
  JumpToOTA();
}
