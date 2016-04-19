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
static const int SILENCE_PERIOD = CYCLES_MS(1.0f);

static const int TICK_LOOP = RADIO_TOTAL_PERIOD / SCHEDULE_PERIOD;

static const int ACCESSORY_TIMEOUT = 50;         // 0.5s timeout before accessory is considered lost
static const int MAX_ACCESSORIES = TICK_LOOP;

static const int ADV_CHANNEL = 81;

// Advertising settings
static const uint32_t ADVERTISE_ADDRESS = 0xCA5CADED;

enum RadioState {
  RADIO_PAIRING,        // We are listening for pairing results
  RADIO_TALKING         // We are communicating to cubes
};

__packed struct RobotHandshake {
	uint8_t msg_id;
  uint8_t ledStatus[12]; // 4-LEDs, three colors
	uint8_t _reserved[4];
};

__packed struct AccessoryHandshake {
	uint8_t msg_id;
	int8_t 	x,y,z;
	uint8_t tap_count;
	uint8_t _reserved[27];
};

struct AccessorySlot {
  bool        					active;
  bool        					allocated;
  int         					last_received;
  uint32_t    					id;
  RobotHandshake				tx_state;
  
  uesb_address_desc_t   address;
};

__packed struct AdvertisePacket {
  uint32_t id;
  uint16_t type;
  uint8_t  hardware_ver;
  uint8_t	 firmware_ver;
  uint16_t _reserved;
};

__packed struct CapturePacket {
  uint16_t ticksUntilStart;
  uint8_t  hopIndex;
  uint8_t  hopBlackout;
  uint8_t  ticksPerBeat;
  uint8_t  beatsPerHandshake;
  uint8_t  ticksToListen;
  uint8_t  ticksToTurn;
  uint8_t  beatsPerRead;
  uint8_t  beatsUntilRead;
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
  void updateLights();
}

#endif
