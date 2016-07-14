#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

namespace Backpack {
  void init();
  void manage();
  
  void update(int channel);

  void setLights(const LightState* lights);
}

#endif /* LIGHTS_H */
