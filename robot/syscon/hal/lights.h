#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cath_red;
  uint8_t cath_green;
  uint8_t cath_blue;
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
  static void setPins(charliePlex_s pins, uint32_t color);
}

#endif /* LIGHTS_H */
