#ifndef __ADC_H
#define __ADC_H

#include "messages.h"

enum ADC_CHANNEL {
  ADC_VEXT,
  ADC_VMAIN,
  ADC_BUTTON,
  ADC_TEMP,
  ADC_VREF,
  ADC_CHANNELS
};

namespace Analog {
  extern bool on_charger;
  void init(void);
  void tick(void);
  void stop(void);
  void transmit(BodyToHead* data);
  void receive(HeadToBody* data);
  void setPower(bool power);
};

#endif
