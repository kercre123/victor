#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

#include "protocol.h"
#include "rtos.h"
#include "micro_esb.h"
#include "clad/types/ledTypes.h"

static const int NUM_PROP_LIGHTS = 4;

static const int RADIO_TOTAL_PERIOD = CYCLES_MS(35.0f);
static const int SCHEDULE_PERIOD = CYCLES_MS(5.0f);
static const int SILENCE_PERIOD = CYCLES_MS(1.0f);
static const int NEXT_CYCLE_FUDGE = 78;

static const int TICK_LOOP = RADIO_TOTAL_PERIOD / SCHEDULE_PERIOD;

static const int ACCESSORY_TIMEOUT = 50;         // 0.5s timeout before accessory is considered lost
static const int MAX_ACCESSORIES = TICK_LOOP;

static const int ADV_CHANNEL = 81;

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
  uint16_t              model;
  int                   last_received;

  bool                  hopSkip;
  uint8_t               hopIndex;
  int8_t                hopBlackout;
  uint8_t               hopChannel;
  
  uint32_t              id;
  
  uesb_address_desc_t   address;
};


namespace Radio {
  void init();
  void advertise();
  void shutdown();

  void sendTestPacket(void) ;
  void discover();
  void setPropLights(unsigned int slot, const Anki::Cozmo::LightState *state);
  void assignProp(unsigned int slot, uint32_t accessory);
  void setLightGamma(uint8_t gamma);
  
  void prepare(void* userdata);
  void resume(void* userdata);
  void manage();
}

#endif
