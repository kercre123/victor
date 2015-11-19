#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/hal.h"
#include "MK02F12810.h"

#include "dac.h"

void Anki::Cozmo::HAL::DACInit(void) {
  /* THIS IS FOR EP1
  GPIO_PIN_SOURCE(STANDBY, PTE, 25);
  SOURCE_SETUP(PTE, 25, SourceGPIO);
  GPIO_SET(GPIO_STANDBY, PIN_STANDBY);
  GPIO_OUT(GPIO_STANDBY, PIN_STANDBY);
  */

  SIM_SCGC6 |= SIM_SCGC6_DAC0_MASK;
  DAC0_C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;
}

void Anki::Cozmo::HAL::DACTone(void) {
  unsigned short k[220];
  
  for (int i = 0; i < 220; i++) {
    k[i] = 0x800 + 0x7FF * sinf(i * 2.0f * 3.14159f / 220);
  }

  for(int i = 0; i < 80; i++) {
    for(int i = 0; i < 220; i++) {
      unsigned short g = k[i];
      DAC0_DAT0L = g;
      DAC0_DAT0H = g >> 8;
      
      Anki::Cozmo::HAL::MicroWait(1);
    }
  }
}
