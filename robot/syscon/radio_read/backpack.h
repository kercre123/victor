#ifndef LIGHTS_H
#define LIGHTS_H

#include <stdint.h>
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

enum LightDriverMode {
  RTC_LEDS,
  TIMER_LEDS
};

struct charliePlex_s
{
  uint8_t anode;
  uint8_t cathode;
};

namespace Backpack {
  void init();
  void update(void);
  void manage();
  void setLights(const LightState* lights);
  void lightsOff();
  void lightMode(LightDriverMode mode);
  void testLight(int index);
}

#endif /* LIGHTS_H */
