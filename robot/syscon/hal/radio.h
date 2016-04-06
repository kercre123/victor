#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

#include "rtos.h"
#include "micro_esb.h"
#include "clad/types/ledTypes.h"

static const int NUM_PROP_LIGHTS = 4;

static const int RADIO_TOTAL_PERIOD = CYCLES_MS(35.0f);
static const int SCHEDULE_PERIOD = CYCLES_MS(5.0f);
static const int CAPTURE_OFFSET = CYCLES_MS(0.5f);

static const int TICK_LOOP = RADIO_TOTAL_PERIOD / SCHEDULE_PERIOD;

static const int ACCESSORY_TIMEOUT = 50;         // 0.5s timeout before accessory is considered lost
static const int PACKET_SIZE = 17;
static const int MAX_ACCESSORIES = TICK_LOOP;

// These are the pipes allocated to communication
static const int ROBOT_PAIR_PIPE = 1;
static const int CUBE_PAIR_PIPE = 2;

static const int ROBOT_TALK_PIPE = 0;
static const int CUBE_TALK_PIPE = 1;

static const int ADV_CHANNEL = 81;

// This is for initial channel selection (do not use advertisement channel)
static const int MAX_TX_CHANNELS = 64;

static const int RADIO_INTERVAL_DELAY = 0xB6;
static const int RADIO_TIMEOUT_MSB = 20;

static const int RADIO_WAKEUP_OFFSET = 18;


// 1/10th the time should be silence
static const int SILENCE_PERIOD = CYCLES_MS(1.0f);

// Advertising settings
static const uint8_t ROBOT_TO_CUBE_PREFIX = 0x42;
static const uint8_t CUBE_TO_ROBOT_PREFIX = 0x52;

static const uint32_t UNUSED_BASE = 0xE6E6E6E6;
static const uint32_t ADVERTISE_BASE = 0xC2C2C2C2;

#define ADVERTISE_PREFIX    {0, ROBOT_TO_CUBE_PREFIX, CUBE_TO_ROBOT_PREFIX}
#define COMMUNICATE_PREFIX  {0, CUBE_TO_ROBOT_PREFIX}

enum AccessoryType {
  ACCESSORY_CUBE    = 0x00,
  ACCESSORY_CHARGER = 0x80
};

enum RadioState {
  RADIO_PAIRING,        // We are listening for pairing results
  RADIO_TALKING         // We are communicating to cubes
};

struct LEDPacket {
  uint8_t ledStatus[16]; // 4-LEDs, three colors
  uint8_t ledDark;       // Dark byte
};

struct AcceleratorPacket {
  int8_t    x, y, z;
  uint8_t   shockCount;
  uint16_t  timestamp;
};

struct AccessorySlot {
  bool        active;
  bool        allocated;
  int         last_received;
  uint32_t    id;
  LEDPacket   tx_state;
  
  uesb_address_desc_t       address;
};

struct AdvertisePacket {
  uint32_t id;
};

struct CapturePacket {
  uint8_t target_channel;
  uint8_t interval_delay;
  uint8_t prefix;
  uint8_t base[4];
  uint8_t timeout_msb;
  uint8_t wakeup_offset;
};

namespace Radio {
  void init();
  void advertise();
  void shutdown();

  void discover();
  void setPropLights(unsigned int slot, const Anki::Cozmo::LightState *state);
  void assignProp(unsigned int slot, uint32_t accessory);

  void prepare(void* userdata);
  void resume(void* userdata);
  void manage();
}

#endif
