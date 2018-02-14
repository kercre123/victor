#ifndef __ADC_H
#define __ADC_H

#include "messages.h"

// This is invalid for button channel (button is 2x)
#define ADC_VOLTS(v) ((uint16_t)(v * (2048.0f / 2.8f)))
#define ADC_WINDOW(low, high) (((high & 0xFFF) << 16) | (low & 0xFFF))

enum ADC_CHANNEL {
  ADC_VEXT,
  ADC_VMAIN,
  ADC_BUTTON,
  ADC_TEMP,
  ADC_VREF,
  ADC_CHANNELS
};

namespace Analog {
  extern volatile uint16_t values[ADC_CHANNELS];
  extern bool button_pressed;
  extern uint16_t battery_voltage;

  void init(void);
  void tick(void);
  void stop(void);
  void transmit(BodyToHead* data);
  void allowCharge(bool);
  bool delayCharge();
};

#endif
