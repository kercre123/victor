#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cathodes[3];
};

namespace Lights {
  void init();
  void manage(void*);
  void setLights(const LightState* lights);
}

#endif /* LIGHTS_H */
