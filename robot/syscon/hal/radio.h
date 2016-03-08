#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

#include "rtos.h"
#include "clad/types/ledTypes.h"

using namespace Anki::Cozmo;

static const int NUM_PROP_LIGHTS = 4;

static const int RADIO_TOTAL_PERIOD = CYCLES_MS(35.0f);
static const int SCHEDULE_PERIOD = CYCLES_MS(5.0f);
static const int CAPTURE_OFFSET = CYCLES_MS(0.5f);

static const int TICK_LOOP = RADIO_TOTAL_PERIOD / SCHEDULE_PERIOD;

static const int ACCESSORY_TIMEOUT = 50;         // 0.5s timeout before accessory is considered lost
static const int PACKET_SIZE = 17;
static const int MAX_ACCESSORIES = TICK_LOOP;

namespace Radio {
  void init();
  void advertise();
  void shutdown();

  void manage(void* userdata = NULL);
  void discover();
  void setPropLights(unsigned int slot, const LightState *state);
  void assignProp(unsigned int slot, uint32_t accessory);
}

#endif
