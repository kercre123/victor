#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

#include "protocol.h"
#include "clad/types/ledTypes.h"
#include "timer.h"
#include "clad/robotInterface/messageFromActiveObject.h"

#define CYCLES(ms) (int)((ms) * 32768.0f / 1000.0f + 0.5f)

static const int NUM_PROP_LIGHTS = 4;
static const int MAX_ACCESSORIES = 4;

static const int TICK_LOOP = 7;
static const int SCHEDULE_PERIOD = CYCLES(5.0f);
static const int RADIO_TOTAL_PERIOD = SCHEDULE_PERIOD * TICK_LOOP;

static const int ROTATE_PERIOD = CYCLES(1000.0f / 30.0f); // 30 FPS
static const int SILENCE_PERIOD = CYCLES(1.0f);
static const int NEXT_CYCLE_FUDGE = 84;

static const int ACCESSORY_TIMEOUT = 100;     // 1s timeout before accessory is considered lost

static const int ADV_CHANNEL = 81;

static const int MAX_ACK_TIMEOUTS = 250;  // Send the packet every 5ms for 1s until we get a reply
static const int MAX_OTA_FAILURES = 5;    // Maximum number of cube reconnects before giving up on OTA
static const int SEND_BATTERY_TIME = 300; // Approximately every 10 seconds

static const int CUBE_ADC_TO_CENTIVOLTS = 410; // NOTE: 360 (3.6V) is the correct scaling value, but we have to fudge due to hardware error

// Radio configuration constants
static const uint32_t RF_BITRATE = RADIO_MODE_MODE_Nrf_1Mbit;
static const uint32_t TX_OUTPUT_POWER = RADIO_TXPOWER_TXPOWER_Neg4dBm;
static const uint8_t RF_ADDR_LENGTH = 4;

// Advertising settings
static const uint32_t ADVERTISE_ADDRESS = 0x533ab5b7; // 0xCA5CADED bit swapped

enum RadioTimerCompare {
  PREPARE_COMPARE,
  RESUME_COMPARE,
};

enum RadioMainstate {
  UESB_STATE_IDLE,
  UESB_STATE_PTX,
  UESB_STATE_PRX
};

struct AccessorySlot {
  int                   last_received;
  int                   failure_count;
  int                   powerCountdown;
  int                   ota_block_index;
  int                   messages_sent;
  int                   messages_received;
  uint32_t              id;

  uint16_t              model;

  uint8_t               rf_channel;
  uint8_t               hopIndex;
  int8_t                hopBlackout;
  uint8_t               hopChannel;
  uint8_t               shockCount;

  s32                   accFilt[3];

  Anki::Cozmo::UpAxis   prevMotionDir;    // Last sent motion direction
  Anki::Cozmo::UpAxis   prevUpAxis;       // Last sent upAxis
  Anki::Cozmo::UpAxis   nextUpAxis;       // Candidate next upAxis to send
  uint8_t               nextUpAxisCount;  // Number of times candidate next upAxis is observed
  uint8_t               rotationPeriod;
  uint8_t               rotationOffset;
  uint8_t               rotationNext;
  uint8_t               movingTimeoutCtr;

  bool                  updating;
  bool                  active;
  bool                  allocated;
  bool                  hopSkip;
  bool                  isMoving;
  bool                  dataReceived;
};

__packed struct CubeFirmware {
  uint32_t            magic;
  uint16_t            dataLen;
  uint16_t            patchLevel;
  uint8_t             patchStart;
  uint8_t             hwVersion;
  uint8_t             _reserved[6];
  CubeFirmwareBlock   data[];
};

namespace Radio {
  void init();
  void advertise();
  void shutdown();
  void prepare();
  void rotate();

  void setPropLightsID(unsigned int slot, uint8_t rotationPeriod);
  void setPropLights(const Anki::Cozmo::LightState *state);
  void assignProp(unsigned int slot, uint32_t accessory);
  void setLightGamma(uint8_t gamma);
  void setWifiChannel(int8_t wifiChannel);
  void enableDiscovery(bool enable);
  
  void enableAccelStreaming(unsigned int slot, bool enable);
}

#endif
