#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

namespace Backpack {
  void init();
  void update(void);
  void manage();
  void flash();
  void setLights(const LightState* lights);
  void lightsOff();
}

#endif /* LIGHTS_H */
