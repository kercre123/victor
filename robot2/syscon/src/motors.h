#ifndef __MOTORS_H
#define __MOTORS_H

#include <stdint.h>

#include "hardware.h"
#include "messages.h"

namespace Motors {
  void receive(HeadToBody *payload);
  void transmit(BodyToHead *payload);

  void init();
  void tick();
}

#endif
