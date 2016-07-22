#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

enum DefaultBackpackPattern {
  LIGHTS_BOOTING,
  LIGHTS_LOW_POWER
};

namespace Backpack {
  void init();
  void manage();
  
  void update(int channel);
  void blink(void);

  void defaultPattern(DefaultBackpackPattern pattern);
  void setLights(const LightState* lights);
}

#endif /* LIGHTS_H */
