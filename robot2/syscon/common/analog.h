#ifndef __ADC_H
#define __ADC_H

#include "messages.h"

#define ADC_VOLTS(v) ((uint16_t)(v * 610.0f))
#define ADC_WINDOW(low, high) (((high & 0xFFF) << 16) | (low & 0xFFF))

enum ADC_CHANNEL {
  ADC_VEXT,
  ADC_VBAT,
  ADC_BUTTON,
  ADC_VREF,
  ADC_CHANNELS
};

namespace Analog {
  extern volatile uint16_t values[ADC_CHANNELS];
  extern bool button_pressed;

  void init(void);
  void tick(void);
  void stop(void);
  void transmit(BodyToHead* data);
  void allowCharge(bool);
  void delayCharge();
};

#endif
