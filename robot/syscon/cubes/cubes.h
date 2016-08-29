#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

#include "protocol.h"
#include "micro_esb.h"
#include "clad/types/ledTypes.h"
#include "timer.h"

#define CYCLES(ms) (int)((ms) * 32768.0f / 1000.0f + 0.5f)

static const int NUM_PROP_LIGHTS = 4;
static const int MAX_ACCESSORIES = 4;

static const int TICK_LOOP = 7;
static const int SCHEDULE_PERIOD = CYCLES(5.0f);
static const int RADIO_TOTAL_PERIOD = SCHEDULE_PERIOD * TICK_LOOP;

static const int ROTATE_PERIOD = CYCLES(1000.0f / 30.0f); // 30 FPS
static const int SILENCE_PERIOD = CYCLES(1.0f);
static const int NEXT_CYCLE_FUDGE = 84;

static const int ACCESSORY_TIMEOUT = 100;       // 1s timeout before accessory is considered lost

static const int ADV_CHANNEL = 81;

static const int OTA_ACK_TIMEOUT = 5;
static const int MAX_ACK_TIMEOUTS = 50;
static const int MAX_OTA_FAILURES = 5;

// Advertising settings
static const uint32_t ADVERTISE_ADDRESS = 0xCA5CADED;

enum RadioState {
  RADIO_PAIRING,        // We are listening for pairing results
  RADIO_TALKING,        // We are communicating to cubes
  RADIO_OTA             // We are updating a remote device firmware
};

struct AccessorySlot {
  bool                  active;
  bool                  allocated;
  int                   last_received;
  int                   failure_count;

  bool                  hopSkip;
  uint8_t               hopIndex;
  int8_t                hopBlackout;
  uint8_t               hopChannel;
  uint16_t              model;

  uint32_t              id;

  uesb_address_desc_t   address;
};


namespace Radio {
  void init();
  void advertise();
  void shutdown();

  void setPropLightsID(unsigned int slot, uint8_t rotationPeriod);
  void setPropLights(const Anki::Cozmo::LightState *state);
  void assignProp(unsigned int slot, uint32_t accessory);
  void setLightGamma(uint8_t gamma);
}

#endif
