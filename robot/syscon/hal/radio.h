#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

namespace Radio {
  void init();
  void manage(void* userdata = NULL);
  void discover();
  void setPropLights(unsigned int slot, const LightState *state);
  void assignProp(unsigned int slot, uint32_t accessory);
  void sendPropConnectionState();
}

#endif
