#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cathodes[3];
};

namespace Lights {
  void init();
  void manage(volatile uint32_t *);
}

#endif /* LIGHTS_H */
