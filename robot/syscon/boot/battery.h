#ifndef BATTERY_H
#define BATTERY_H

#include <stddef.h>
#include <stdint.h>

#include "hal/hardware.h"
#include "anki/cozmo/robot/spineData.h"


const int32_t V_REFERNCE_MV       = 1200; // 1.2V Bandgap reference
const int32_t V_PRESCALE          = 3;
const int32_t V_SCALE             = 0x3ff; // 10 bit ADC

#define CALC_LEVEL(v, s)          (FIXED_DIV(TO_FIXED((float)v / V_REFERNCE_MV / V_PRESCALE * V_SCALE), s) / 64)

const int32_t VEXT_SCALE          = TO_FIXED(2.0); // Cozmo 4.1 voltage divider
const int32_t VEXT_CONTACT_LEVEL  = CALC_LEVEL(4.5, VEXT_SCALE);

namespace Battery {
  // Initialize the charge pins and sensing
  void init();

  // Update the state of the battery
  void manage();
  int32_t read(void);
  void powerOn();
}

#endif
