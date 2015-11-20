#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/hal.h"
#include "MK02F12810.h"
#include "hardware.h"

#include "dac.h"

void Anki::Cozmo::HAL::DACInit(void) {
  #ifdef EP1_HEADBOARD
  SOURCE_SETUP(GPIO_AUDIO_STANDBY, SOURCE_AUDIO_STANDBY, SourceGPIO);
  GPIO_SET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  GPIO_OUT(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  #endif

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
