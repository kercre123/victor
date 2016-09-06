#ifndef LIGHTS_H
#define LIGHTS_H

extern "C" {
  #include "nrf51.h"
}

#include <stdint.h>
#include "clad/types/ledTypes.h"
#include "battery.h"

using namespace Anki::Cozmo;

static const int BACKPACK_LIGHTS = 5;

enum BackpackLayer {
  BPL_ANIMATION,
  BPL_USER,
  BPL_IMPULSE,
  BACKPACK_LAYERS
};

enum DefaultBackpackPattern {
  LIGHTS_OFF,
  LIGHTS_CHARGING,
  LIGHTS_CHARGED,
  LIGHTS_SLEEPING,
  LIGHTS_CHARGER_OOS
};

namespace Backpack {
  void init();
  void manage();
  
  void setLayer(BackpackLayer);

  void useTimer();
  void detachTimer();
  
  void lightsOff();
  void setLowBattery(bool batteryLow);
  void setChargeState(CurrentChargeState state);

  void setLights(BackpackLayer, const LightState* lights);
  void setLightsMiddle(BackpackLayer, const LightState* lights);
  void setLightsTurnSignals(BackpackLayer, const LightState* lights);
}

#endif /* LIGHTS_H */
