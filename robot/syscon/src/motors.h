#ifndef __MOTORS_H
#define __MOTORS_H

#include <stdint.h>

#include "hardware.h"
#include "messages.h"

namespace Motors {
  extern bool head_driven;
  extern bool lift_driven;
  extern bool treads_driven;
  
  void receive(HeadToBody *payload);
  void transmit(BodyToHead *payload);

  void init();
  void stop();
  void tick();

  void resetEncoderHysteresis();
}

#endif
