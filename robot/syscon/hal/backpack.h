#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

static const int BACKPACK_LIGHT_CHANNELS = 5;

enum DefaultBackpackPattern {
  LIGHTS_RELEASE,
  LIGHTS_CHARGING,
  LIGHTS_CHARGED,
  LIGHTS_SLEEPING,
  LIGHTS_CHARGER_OOS
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
