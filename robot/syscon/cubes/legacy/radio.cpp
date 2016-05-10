#include <stdint.h>
#include <string.h>

#include "tests.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"

#include "micro_esb.h"
  
#include "random.h"
#include "hardware.h"
#include "rtos.h"
#include "debug.h"
#include "radio.h"
#include "timer.h"
#include "head.h"
#include "crypto.h"
#include "lights.h"
#include "messages.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

static void EnterState(RadioState state);
static void send_capture_packet(void* userdata);

// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Current status values of cubes/chargers
static RadioState        radioState;

extern uesb_mainstate_t  m_uesb_mainstate;

static RTOS_Task*        capture_task;

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

//static struct {
  static AccessorySlot accessories[MAX_ACCESSORIES];

  // Variables for talking to an accessory
  static uint8_t currentAccessory;
//} __attribute((at(0x20000004));

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

  while (one != 0) {
    if (op >= res + one) {
      op -= res + one;
      res += 2 * one;
    }

    res >>= 1;
    one >>= 2;
  }
  return res;
}

static void createAddress(uesb_address_desc_t& address) { 
  // Generate random values
  gen_random(&address.prefix[0], 1);
  address.base0 = 0xE7E7E7E7;

  // Create a random RF channel
  gen_random(&address.rf_channel, sizeof(address.rf_channel));
  address.rf_channel %= MAX_TX_CHANNELS;
}

// This will move to the next frequency (channel hopping)
#ifdef CHANNEL_HOP
static inline uint8_t next_channel(uint8_t channel) {
  return (channel >> 1) ^ ((channel & 1) ? 0x2D : 0);
}
#endif

void Radio::init() {
  // Create a task for capturing cubes
  capture_task = RTOS::create(send_capture_packet, false);
}

void Radio::advertise(void) {
  const uesb_config_t uesb_config = {
    RADIO_MODE_MODE_Nrf_1Mbit,
    UESB_CRC_8BIT,
    RADIO_TXPOWER_TXPOWER_Pos4dBm,
    PACKET_SIZE,
    5,    // Address length
    RADIO_PRIORITY // Service speed doesn't need to be that fast (prevent blocking encoders)
  };

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;

  // Generate target address for the robot
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    accessories[i].address = TalkingAddress;
    createAddress(accessories[i].address);
  }

  uesb_init(&uesb_config);
}

void Radio::shutdown(void) {
  RTOS::stop(capture_task);
  uesb_disable();
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

void uesb_event_handler(uint32_t flags)
{
  // Only respond to receive interrupts
  if(~flags & UESB_INT_RX_DR_MSK) {
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
      msg.rssi = rx_payload.rssi;
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
    RTOS::start(capture_task, CAPTURE_OFFSET, (void*) slot);
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

    EnterState(RADIO_PAIRING);
    break ;
  }
}

void Radio::setPropLights(unsigned int slot, const LightState *state) {
  if (slot >= MAX_ACCESSORIES) {
    return ;
  }

 for (int c = 0; c < NUM_PROP_LIGHTS; c++) {
   Lights::update(CUBE_LIGHT_INDEX_BASE + CUBE_LIGHT_STRIDE * slot + c, &state[c]);
 }
}

void Radio::assignProp(unsigned int slot, uint32_t accessory) {
  if (slot >= MAX_ACCESSORIES) {
    return ;
  }
  
  AccessorySlot* acc = &accessories[slot];
  if (accessory != 0)
  {
    acc->allocated = true;
    acc->id = accessory;
  }
  else
  {
    acc->allocated = false;
    acc->active    = false;
    if (acc->id != 0)
    {
      SendObjectConnectionState(slot);
      acc->id = 0;
    }
  }
}

void Radio::updateLights() {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    AccessorySlot* acc = &accessories[currentAccessory];
    
    if (!acc->active) continue ;
    
    // Update the color status of the lights
    int sum = 0;
    for (int c = 0; c < NUM_PROP_LIGHTS; c++) {
      static const uint8_t light_index[NUM_PROP_LIGHTS][4] = {
        {  0,  1,  2, 12},
        {  3,  4,  5, 13},
        {  6,  7,  8, 14},
        {  9, 10, 11, 15}
      };

      int group = CUBE_LIGHT_INDEX_BASE + CUBE_LIGHT_STRIDE * currentAccessory + c;
      uint8_t* rgbi = Lights::state(group);

      for (int i = 0; i < 4; i++) {
        acc->tx_state.ledStatus[light_index[c][i]] = rgbi[i];
        sum += rgbi[i] * rgbi[i];
      }
    }

    acc->tx_state.ledDark = 0xFF - isqrt(sum);
  }
}

void Radio::prepare(void* userdata) {
  uesb_stop();

  // Transmit to accessories round-robin
  if (++currentAccessory >= TICK_LOOP) {
    currentAccessory = 0;
  }

  if (currentAccessory >= MAX_ACCESSORIES) return ;

  AccessorySlot* acc = &accessories[currentAccessory];

  if (acc->active && ++acc->last_received < ACCESSORY_TIMEOUT) {
    // We send the previous LED state (so we don't get jitter on radio)
    uesb_address_desc_t& address = accessories[currentAccessory].address;

    // Broadcast to the appropriate device
    EnterState(RADIO_TALKING);
    memcpy(&acc->tx_state.ledStatus[12], &acc->id, 4); // XXX: THIS IS A HACK FOR NOW
    uesb_prepare_tx_payload(&address, ROBOT_TALK_PIPE, &acc->tx_state, sizeof(LEDPacket));

    #ifdef CHANNEL_HOP
    // Hop to next frequency (NOTE: DISABLED UNTIL CUBES SUPPORT IT)
    address.rf_channel = next_channel(address.rf_channel);
    #endif
  } else {
    // Timeslice is empty, send a dummy command on the channel so people know to stay away
    if (acc->active)
    {
      // Spread the remaining accessories forward as a patch fix
      // Simply reset the timeout of all accessories
      for (int i = 0; i < MAX_ACCESSORIES; i++) {
        acc->last_received = 0;
      }

      acc->active = false;
      SendObjectConnectionState(currentAccessory);
    }
    
    // This just send garbage and return to pairing mode when finished
    EnterState(RADIO_PAIRING);
    uesb_prepare_tx_payload(&accessories[currentAccessory].address, 1, NULL, 0);
  }
}

void Radio::resume(void* userdata) {
  // Reenable the radio
  uesb_start();
}

// THIS IS A TEMPORARY HACK AND I HATE IT
void Radio::manage(void) {
  // We are in bluetooth mode, do not do this
  if (m_uesb_mainstate == UESB_STATE_UNINITIALIZED) {
    return ;
  }

  static int next_prepare = GetCounter() + SCHEDULE_PERIOD;
  static int next_resume  = next_prepare + SILENCE_PERIOD;
  int count = GetCounter();
    
  if (next_prepare - count < 0) {
    prepare(NULL);
    next_prepare += SCHEDULE_PERIOD;
  }
  
  if (next_resume - count < 0) {
    resume(NULL);
    next_resume += SCHEDULE_PERIOD;
  }
}
