#ifndef __ADC_H
#define __ADC_H

#include "messages.h"

enum ADC_CHANNEL {
  ADC_VEXT,
  ADC_VBAT,
  ADC_BUTTON,
  ADC_CHANNELS
};

namespace Analog {
  extern volatile uint16_t values[ADC_CHANNELS];
  extern bool button_pressed;

  #ifndef BOOTLOADER
  void transmit(BodyToHead* data);
  #endif

  void init(void);
  void tick(void);
  void stop(void);
};

#endif
