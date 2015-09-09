#include <string.h>
#include <stdint.h>
#include <math.h>

#include "board.h"
#include "fsl_debug_console.h"

#include "dac.h"
#include "anki/cozmo/robot/hal.h"

void dac_init() {
  // AHHHHHH DO NO USE!
  SIM_SCGC6 |= SIM_SCGC6_DAC0_MASK;

  DAC0_C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;
  
  for(;;) {
    for (int i = 0; i < 220; i++) {
      unsigned short k = 0x800 + 0x7FF * sinf(i * 2.0f * 3.14159f / 220);
      DAC0_DAT0L = k;
      DAC0_DAT0H = k >> 8;
      
      Anki::Cozmo::HAL::MicroWait(80);
    }
  }
}
