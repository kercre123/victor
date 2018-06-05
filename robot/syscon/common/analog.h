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
  ADC_CHANNELS
};

namespace Analog {
  extern bool button_pressed;

  void init(void);
  void tick(void);
  void stop(void);
  void transmit(BodyToHead* data);
  void receive(HeadToBody* data);
  void setPower(bool power);
  void inhibitCharge(bool force = false);
};

#endif
