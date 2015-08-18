#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cathodes[3];
};

enum charlieChannels_e
{
  RGB1,
  RGB2,
  RGB3,
  RGB4,
  numCharlieChannels
};

namespace Lights {
  void init();
  void manage(volatile uint32_t *);
}

#endif /* LIGHTS_H */
