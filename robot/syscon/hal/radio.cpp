#include <stdint.h>
#include <string.h>

#include "tests.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"

#include "micro_esb.h"
  
#include "hardware.h"
#include "rtos.h"
#include "debug.h"
#include "radio.h"
#include "timer.h"
#include "head.h"
#include "rng.h"
#include "spine.h"
#include "led.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

#define ABS(x)   ((x < 0) ? -x : x)

static const int NUM_PROP_LIGHTS = 4;

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
  LightState  lights[NUM_PROP_LIGHTS];
  uint32_t    lightPhase[NUM_PROP_LIGHTS];
  
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

static void EnterState(RadioState state);

static const int TOTAL_PERIOD = CYCLES_MS(35.0f);
static const int SCHEDULE_PERIOD = CYCLES_MS(5.0f);
static const int CAPTURE_OFFSET = CYCLES_MS(0.5f);

static const int TICK_LOOP = TOTAL_PERIOD / SCHEDULE_PERIOD;

static const int ACCESSORY_TIMEOUT = 400;         // 2s timeout before accessory is considered lost
static const int PACKET_SIZE = 17;
static const int MAX_ACCESSORIES = TICK_LOOP;

// Advertising settings
static const uint8_t ROBOT_TO_CUBE_PREFIX = 0x42;
static const uint8_t CUBE_TO_ROBOT_PREFIX = 0x52;

static const uint32_t UNUSED_BASE = 0xE6E6E6E6;
static const uint32_t ADVERTISE_BASE = 0xC2C2C2C2;

#define ADVERTISE_PREFIX    {0, ROBOT_TO_CUBE_PREFIX, CUBE_TO_ROBOT_PREFIX}
#define COMMUNICATE_PREFIX  {0, CUBE_TO_ROBOT_PREFIX}

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

// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Current status values of cubes/chargers
static RadioState        radioState;

static const uesb_address_desc_t PairingAddress = {
  ADV_CHANNEL,
  UNUSED_BASE, 
  ADVERTISE_BASE,
  ADVERTISE_PREFIX,
  0xFF
};

static const uesb_address_desc_t TalkingAddress = {
  0,
  UNUSED_BASE,
  ADVERTISE_BASE,
  COMMUNICATE_PREFIX,
  0x03
};

static AccessorySlot accessories[MAX_ACCESSORIES];

// Variables for talking to an accessory
static uint8_t currentAccessory;

// Integer square root calculator
uint8_t isqrt(uint32_t op)
{
  if (op >= 0xFC04) {
    return 0xFE;
  }
  
  uint32_t res = 0;
  uint32_t one = 1uL << 18; // Second to top bit (255^2 * 16)

  // "one" starts at the highest power of four <= than the argument.
  while (one > op)
  {
      one >>= 2;
  }

  while (one != 0)
  {
      if (op >= res + one)
      {
          op = op - (res + one);
          res = res +  2 * one;
      }
      res >>= 1;
      one >>= 2;
  }
  return res;
}

static void createAddress(uesb_address_desc_t& address) {
  uint8_t data[4];
  
  // Generate random values
  Random::get(&data, sizeof(data));
  Random::get(&address.prefix[0], address.prefix[0]);
  address.base0 = 0xE7E7E7E7;
  
  // Create a random RF channel
  Random::get(&address.rf_channel, sizeof(address.rf_channel));
  address.rf_channel %= MAX_TX_CHANNELS;
}

// This will move to the next frequency (channel hopping)
#ifdef CHANNEL_HOP
static inline uint8_t next_channel(uint8_t channel) {
  return (channel >> 1) ^ ((channel & 1) ? 0x2D : 0);
}
#endif

void Radio::init() {
  const uesb_config_t uesb_config = {
    UESB_BITRATE_1MBPS,
    UESB_CRC_8BIT,
    UESB_TX_POWER_0DBM,
    PACKET_SIZE,
    5,    // Address length
    2     // Service speed doesn't need to be that fast (prevent blocking encoders)
  };

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;

  // Generate target address for the robot
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    accessories[i].address = TalkingAddress;
    createAddress(accessories[i].address);
  }

  // Start the radio stack
  uesb_init(&uesb_config);
  EnterState(RADIO_PAIRING);
  uesb_start();

  RTOS::schedule(Radio::manage, SCHEDULE_PERIOD);
}

static int LocateAccessory(uint32_t id) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (!accessories[i].allocated) continue ;
    if (accessories[i].id == id) return i;
  }

  return -1;
}

static int FreeAccessory(void) {
  #ifdef AUTO_GATHER
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (!accessories[i].allocated) return i;
  }
  #endif

  return -1;
}

static void EnterState(RadioState state) { 
  radioState = state;

  switch (state) {
    case RADIO_PAIRING:
      uesb_set_rx_address(&PairingAddress);
      break;
    case RADIO_TALKING:
      uesb_set_rx_address(&accessories[currentAccessory].address);
      break ;
  }
}

static void send_dummy_byte(void) {
  // This just send garbage and return to pairing mode when finished
  EnterState(RADIO_PAIRING);
  uesb_write_tx_payload(&accessories[currentAccessory].address, 1, NULL, 0);
}

static void send_capture_packet(void* userdata) {
  int slot = (int) userdata;

  uesb_address_desc_t& address = accessories[slot].address;
  
  // Send a pairing packet
  CapturePacket pair;

  pair.target_channel = address.rf_channel;
  pair.interval_delay = RADIO_INTERVAL_DELAY;
  pair.prefix = address.prefix[ROBOT_TALK_PIPE];
  memcpy(&pair.base, &address.base0, sizeof(address.base0));
  pair.timeout_msb = RADIO_TIMEOUT_MSB;
  pair.wakeup_offset = RADIO_WAKEUP_OFFSET;

  // Tell this accessory to come over to my side
  uesb_write_tx_payload(&PairingAddress, ROBOT_PAIR_PIPE, &pair, sizeof(CapturePacket));
}

void SendObjectConnectionState(int slot)
{
  ObjectConnectionState msg;
  msg.objectID = slot;
  msg.factoryID = accessories[slot].id;
  msg.connected = accessories[slot].active;
  RobotInterface::SendMessage(msg);
}

extern "C" void uesb_event_handler(void)
{
  // Only respond to receive interrupts
  if(~uesb_get_clear_interrupts() & UESB_INT_RX_DR_MSK) {
    return ;
  }

  uesb_payload_t rx_payload;
  uesb_read_rx_payload(&rx_payload);

  int slot;

  switch (radioState) {
  case RADIO_PAIRING:      
    if (rx_payload.pipe != CUBE_PAIR_PIPE) {
      break ;
    }

    AdvertisePacket packet;
    memcpy(&packet, &rx_payload.data, sizeof(AdvertisePacket));

    // Attempt to locate existing accessory and repair
    slot = LocateAccessory(packet.id);
    if (slot < 0) {
      ObjectDiscovered msg;
      msg.factory_id = packet.id;
      RobotInterface::SendMessage(msg);
            
      // Attempt to allocate a slot for it
      slot = FreeAccessory();

      // We cannot find a place for it
      if (slot < 0) {
        break ;
      }
    }

    // We are loading the slot
    accessories[slot].id = packet.id;
    accessories[slot].last_received = 0;
    if (accessories[slot].active == false)
    {
      accessories[slot].active = true;
      SendObjectConnectionState(slot);
    }

    // Schedule a one time capture for this slot
    RTOS::schedule(send_capture_packet, CAPTURE_OFFSET, (void*) slot, false);
    break ;
    
  case RADIO_TALKING:
    if (rx_payload.pipe != CUBE_TALK_PIPE) {
      break ;
    }

    AccessorySlot* acc = &accessories[currentAccessory];

    // XXX: START HACK
    uint32_t id;
    memcpy(&id, &rx_payload.data[12], 4);
    if (id != acc->id) break ;
    // XXX: END HACK

    AcceleratorPacket* ap = (AcceleratorPacket*) &rx_payload.data;

    acc->last_received = 0;

    PropState msg;
    msg.slot = currentAccessory;
    msg.x = ap->x;
    msg.y = ap->y;
    msg.z = ap->z;
    msg.shockCount = ap->shockCount;
    RobotInterface::SendMessage(msg);

    send_dummy_byte();
    break ;
  }
}

void Radio::setPropLights(unsigned int slot, const LightState *state) {
  if (slot > MAX_ACCESSORIES) {
    return ;
  }

  AccessorySlot* acc = &accessories[slot];
  memcpy(acc->lights, state, sizeof(acc->lights));
}

void Radio::assignProp(unsigned int slot, uint32_t accessory) {
  if (slot > MAX_ACCESSORIES) {
    return ;
  }
  
  AccessorySlot* acc = &accessories[slot];
  acc->id = accessory;
  acc->allocated = true;
  acc->active = false;
}

void Radio::manage(void* userdata) {
  // Transmit to accessories round-robin
  currentAccessory = (currentAccessory + 1) % TICK_LOOP;
  
  if (currentAccessory >= MAX_ACCESSORIES) return ;

  AccessorySlot* acc = &accessories[currentAccessory];

  if (acc->active && ++acc->last_received < ACCESSORY_TIMEOUT) {
    // We send the previous LED state (so we don't get jitter on radio)
    uesb_address_desc_t& address = accessories[currentAccessory].address;
    
    // Broadcast to the appropriate device
    EnterState(RADIO_TALKING);
    memcpy(&acc->tx_state.ledStatus[12], &acc->id, 4); // XXX: THIS IS A HACK FOR NOW
    uesb_write_tx_payload(&address, ROBOT_TALK_PIPE, &acc->tx_state, sizeof(LEDPacket));

    #ifdef CHANNEL_HOP
    // Hop to next frequency (NOTE: DISABLED UNTIL CUBES SUPPORT IT)
    address.rf_channel = next_channel(address.rf_channel);
    #endif

    // Update the color status of the 
    uint32_t currentFrame = GetFrame();

    int sum = 0;
    for (int c = 0; c < NUM_PROP_LIGHTS; c++) {
      static const uint8_t light_index[NUM_PROP_LIGHTS][4] = {
        {  0,  1,  2, 12},
        {  3,  4,  5, 13},
        {  6,  7,  8, 14},
        {  9, 10, 11, 15}
      };

      uint8_t rgbi[4];
      CalculateLEDColor(rgbi, acc->lights[c], currentFrame, acc->lightPhase[c]);

      for (int i = 0; i < 4; i++) {
        acc->tx_state.ledStatus[light_index[c][i]] = rgbi[i];
        sum += rgbi[i] * rgbi[i];
      }
    }

    // THIS IS PROBABLY WRONG
    acc->tx_state.ledDark = 0xFF - isqrt(sum);
  } else {
    // Timeslice is empty, send a dummy command on the channel so people know to stay away
    if (acc->active)
    {
      acc->active = false;
      SendObjectConnectionState(currentAccessory);
    }
    send_dummy_byte();
  }
}
