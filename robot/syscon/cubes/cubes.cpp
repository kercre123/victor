#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"

#include "micro_esb.h"
  
#include "protocol.h"
#include "hardware.h"
#include "cubes.h"
#include "head.h"
#include "lights.h"
#include "messages.h"

#include "clad/robotInterface/messageFromActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

//#define NATHAN_CUBE_JUNK
//#define CUBE_HOP

#define TOTAL_BLOCKS(x) ((x)->dataLen / sizeof(CubeFirmwareBlock))

using namespace Anki::Cozmo;

static void EnterState(RadioState state);

enum TIMER_COMPARE {
  PREPARE_COMPARE,
  RESUME_COMPARE,
  RESET_COMPARE
};

// Global head / body sync values
extern uesb_mainstate_t  m_uesb_mainstate;

// This is our internal copy of the cube firmware update
extern "C" const CubeFirmware __CUBE_XS;
extern "C" const CubeFirmware __CUBE_XS6;
static const CubeFirmware* ValidPerfs[] = {
  &__CUBE_XS,
  &__CUBE_XS6,
  (const CubeFirmware*) NULL
};

#ifdef CUBE_HOP
static const int wifiChannel = 1;
#endif

static const uesb_address_desc_t PairingAddress = {
  ADV_CHANNEL,
  ADVERTISE_ADDRESS,
  sizeof(AdvertisePacket)
};

static const uesb_address_desc_t NoiseAddress = {
  1,
  0xE7E7E7E7
};


// These are variables used for handling OTA
static uesb_address_desc_t OTAAddress = { 0x63, 0, sizeof(OTAFirmwareBlock) };
static const CubeFirmware* ota_device;
static int ota_block_index;
static int ack_timeouts;
static int light_gamma;

// OTA Timeout values
static int ota_timeout_countdown;
static bool ota_pending;

// Variables for talking to an accessory
static uint8_t currentAccessory;
static AccessorySlot accessories[MAX_ACCESSORIES];

// Current status values of cubes/chargers
static RadioState        radioState;

static uint8_t _tapTime = 0;

void Radio::init() {
  light_gamma = 0x100;
}

void Radio::advertise(void) {
  const uesb_config_t uesb_config = {
    RADIO_MODE_MODE_Nrf_1Mbit,
    UESB_CRC_16BIT,
    RADIO_TXPOWER_TXPOWER_Neg4dBm,
    4,              // Address length
    RADIO_PRIORITY  // Service speed doesn't need to be that fast (prevent blocking encoders)
  };

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;

  LightState black[NUM_PROP_LIGHTS];
  memset(&black, 0, sizeof(black));

  for (int c = 0; c < MAX_ACCESSORIES; c++) {
    Radio::setPropLights(c, black);
  }
  
  uesb_init(&uesb_config);

  // Timer scheduling
  NRF_TIMER0->POWER = 1;
  
  NRF_TIMER0->TASKS_STOP = 1;
  NRF_TIMER0->TASKS_CLEAR = 1;
  
  NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER0->PRESCALER = 0;
  NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
  
  NRF_TIMER0->INTENCLR = ~0;
  NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk |
                         TIMER_INTENSET_COMPARE1_Msk;

  NVIC_EnableIRQ(TIMER0_IRQn);
  NVIC_SetPriority(TIMER0_IRQn, RADIO_TIMER_PRIORITY);

  NRF_TIMER0->CC[PREPARE_COMPARE] = SCHEDULE_PERIOD;
  NRF_TIMER0->CC[RESUME_COMPARE] = SILENCE_PERIOD;
  NRF_TIMER0->CC[RESET_COMPARE] = SCHEDULE_PERIOD;
  
  NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE2_CLEAR_Msk;

  NRF_TIMER0->TASKS_START = 1;
}

void Radio::setLightGamma(uint8_t gamma) {
  light_gamma = gamma + 1;
}

void Radio::shutdown(void) {
  NRF_TIMER0->TASKS_STOP = 1;
  NVIC_DisableIRQ(TIMER0_IRQn);

  uesb_disable();
}

static int LocateAccessory(uint32_t id) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (!accessories[i].allocated) continue ;
    if (accessories[i].id == id) return i;
  }

  return -1;
}

#ifdef NATHAN_CUBE_JUNK
static int AllocateAccessory(uint32_t id) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (accessories[i].allocated) continue ;
    accessories[i].allocated = true;
    accessories[i].id = id;
    return i;
  }
  
  return -1;
}
#endif


static void EnterState(RadioState state) { 
  radioState = state;

  ota_pending = false;

  switch (state) {
    case RADIO_PAIRING:
      uesb_set_rx_address(&PairingAddress);
      break;
    case RADIO_TALKING:   
      uesb_set_rx_address(&accessories[currentAccessory].address);
      break ;
    case RADIO_OTA:
      uesb_set_rx_address(&OTAAddress);
      break ;
  }
}

static void SendObjectConnectionState(int slot)
{
  ObjectConnectionState msg;
  msg.objectID = slot;
  msg.factoryID = accessories[slot].id;
  msg.connected = accessories[slot].active;
  msg.device_type = accessories[slot].model;
  RobotInterface::SendMessage(msg);
}

static void ota_send_next_block() {
  OTAFirmwareBlock msg;
  
  msg.messageId = 0xF0 | ota_block_index;
  memcpy(msg.block, ota_device->data[ota_block_index], sizeof(CubeFirmwareBlock));
  
  uesb_write_tx_payload(&OTAAddress, &msg, sizeof(OTAFirmwareBlock));

  ota_timeout_countdown = OTA_ACK_TIMEOUT;
  ota_pending = true;
}

static uint8_t random() {
  static uint8_t c = 0xFF;
  c = (c >> 1) ^ (c & 1 ? 0x2D : 0);
  return c;
}

static void OTARemoteDevice(const uint32_t id) {
  // Reset our block count
  ota_block_index = 0;
  ack_timeouts = 0;
  
  // This is address
  OTAAddress.address = id;
  OTAAddress.rf_channel = (random() & 0x3F) + 4;

  EnterState(RADIO_OTA);

  // Tell our device to begin - must set EVERY field
  CapturePacket pair;
  pair.ticksUntilStart = 132; // Lowest legal value
  pair.hopIndex = 0;
  pair.hopBlackout = OTAAddress.rf_channel;
  pair.ticksPerBeat = CLOCKS(SCHEDULE_PERIOD);    // 32768/164 = 200Hz
  pair.beatsPerHandshake = TICK_LOOP; // 1 out of 7 beats handshakes with this cube
  pair.ticksToListen = 0;     // Currently unused
  pair.beatsPerRead = 4;
  pair.beatsUntilRead = 4;    // Should be computed to synchronize all tap data
  pair.patchStart = 0;

  uesb_address_desc_t address = { ADV_CHANNEL, id };  
  uesb_write_tx_payload(&address, &pair, sizeof(CapturePacket));

  ota_send_next_block();
}

void uesb_event_handler(uint32_t flags)
{
  // Only respond to receive interrupts
  if(~flags & UESB_INT_RX_DR_MSK) {
    return ;
  }

  uesb_payload_t rx_payload;
  uesb_read_rx_payload(&rx_payload);

  switch (radioState) {
  case RADIO_OTA:
    // Send noise and return to pairing
    if (++ota_block_index < TOTAL_BLOCKS(ota_device)) {
      ack_timeouts = 0;
      ota_send_next_block();
    } else {
      EnterState(RADIO_PAIRING);
      uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
    }

    break ;
  case RADIO_PAIRING:
    {
      AdvertisePacket advert;
      memcpy(&advert, &rx_payload.data, sizeof(AdvertisePacket));

      // Attempt to locate existing accessory and repair
      int slot = LocateAccessory(advert.id);

      #ifdef NATHAN_CUBE_JUNK
      if (slot < 0 && rx_payload.rssi < 50 && rx_payload.rssi > -50) {
        slot = AllocateAccessory(advert.id);
      }
      #endif
      
      if (slot < 0) {     
        ObjectDiscovered msg;
        msg.device_type = advert.model;
        msg.factory_id = advert.id;
        msg.rssi = rx_payload.rssi;
        RobotInterface::SendMessage(msg);

        // Do not auto allocate the cube
        break ;
      }

      // Radio firmware header is valid
      for (const CubeFirmware** all_perfs = ValidPerfs; all_perfs; all_perfs++) {
        ota_device = *all_perfs;

        // Unrecognized device
        if (ota_device->magic != CUBE_FIRMWARE_MAGIC) {
          return ;
        }

        // This is an invalid hardware version and we should not try to do anything with it
        if (advert.hwVersion != ota_device->hwVersion) {
          continue ;
        }

        // Check if the device firmware is out of date
        if (advert.patchLevel & ~ota_device->patchLevel) {
          OTARemoteDevice(advert.id);
          return ;
        }
        
        // We found the image we want
        break ;
      }

      // We are loading the slot
      AccessorySlot* acc = &accessories[slot]; 
      acc->id = advert.id;
      acc->last_received = 0;
      acc->model = advert.model;
      
      if (acc->active == false)
      {
        acc->allocated = true;
        acc->active = true;
        
        SendObjectConnectionState(slot);
      }

      // This is where the cube shall live
      uesb_address_desc_t* new_addr = &acc->address;

      new_addr->address = advert.id;
      new_addr->payload_length = sizeof(AccessoryHandshake);

      // Tell the cube to start listening
      uesb_address_desc_t address = { ADV_CHANNEL, advert.id };
      CapturePacket pair;
           
      #ifndef CUBE_HOP
      new_addr->rf_channel = (random() & 0x3F) + 0x4;
      
      pair.ticksUntilStart = 132; // Lowest legal value
      pair.hopIndex = 0;
      pair.hopBlackout = new_addr->rf_channel;
      #else
      // Find the next time accessory will be contacted
      int target_slot = slot - currentAccessory;
      
      if (target_slot <= 0) {
        target_slot += TICK_LOOP;
      }
      
      NRF_TIMER0->TASKS_CAPTURE[3] = 1;
      int clocks = (target_slot * SCHEDULE_PERIOD) - NRF_TIMER0->CC[3] + SILENCE_PERIOD;
      int ticks_to_next = (clocks << 5) / ((int)NRF_CLOCK_FREQUENCY >> 10);

      if (ticks_to_next < SCHEDULE_PERIOD) {
        ticks_to_next += RADIO_TOTAL_PERIOD;
        acc->hopSkip = true;
      } else {
        acc->hopSkip = false;
      }

      acc->hopIndex = (random() & 0xF) + 0x12;
      acc->hopBlackout = (wifiChannel * 5) - 9;
      if (acc->hopBlackout < 0) {
        acc->hopBlackout = 0;
      }
      acc->hopChannel = 0;

      pair.ticksUntilStart = ticks_to_next - NEXT_CYCLE_FUDGE;
      pair.hopIndex = acc->hopIndex;
      pair.hopBlackout = acc->hopBlackout;
      #endif

      pair.ticksPerBeat = CLOCKS(SCHEDULE_PERIOD);
      pair.beatsPerHandshake = TICK_LOOP; // 1 out of 7 beats handshakes with this cube

      pair.ticksToListen = 0;     // Currently unused
      pair.beatsPerRead = 4;
      pair.beatsUntilRead = 4;    // Should be computed to synchronize all tap data
      pair.patchStart = ota_device->patchStart;
      
      // Send a pairing packet
      uesb_write_tx_payload(&address, &pair, sizeof(CapturePacket));
    }
    break ;

  case RADIO_TALKING:
    {
      AccessoryHandshake* ap = (AccessoryHandshake*) &rx_payload.data;
      int slot = LocateAccessory(rx_payload.address.address);
      
      AccessorySlot* acc = &accessories[slot];
      
      acc->last_received = 0;

      PropState msg;
      msg.slot = slot;
      msg.x = ap->x;
      msg.y = ap->y;
      msg.z = ap->z;
      msg.shockCount = ap->tap_count;
      msg.tapTime = ap->tapTime;
      msg.tapNeg = ap->tapNeg;
      msg.tapPos = ap->tapPos;
      RobotInterface::SendMessage(msg);

      EnterState(RADIO_PAIRING);
    }

    break ;
  }
}

void Radio::setPropLights(unsigned int slot, const LightState *state) {
  if (slot >= MAX_ACCESSORIES) {
    return ;
  }

  for (int c = 0; c < NUM_PROP_LIGHTS; c++) {
    Lights::update(lightController.cube[slot][c], &state[c]);
  }
}

void Radio::assignProp(unsigned int slot, uint32_t accessory) {
  if (slot >= MAX_ACCESSORIES) {
    return ;
  }
  
  AccessorySlot* acc = &accessories[slot];
  if (accessory != 0)
  {
    // If the slot is active send disconnect message if it's occupied by a different
    // device than the one requested. If it's the same as the one requested, send
    // a connect message.
    if (acc->active) {
      if(acc->id != accessory) {
        acc->active = false;
      }
      SendObjectConnectionState(slot);
    }

    acc->allocated = true;
    acc->id = accessory;
    acc->failure_count = 0;
  }
  else
  {
    acc->allocated = false;
    acc->active    = false;

    // Send disconnect response
    // NOTE: The id is that of the currently connected object if there was one.
    //       Otherwise the id is 0.
    SendObjectConnectionState(slot);
    acc->id = 0;
    
  }
}

static void ota_timeout() {
  ota_pending = false;
  
  // Give up, we didn't receive any acks soon enough
  if (++ack_timeouts >= MAX_ACK_TIMEOUTS) {    
    // Disconnect cube if it has failed to connect
    int slot = LocateAccessory(OTAAddress.address);
    
    if (slot >= 0) {
      AccessorySlot* acc = &accessories[slot];
      
      if (acc->failure_count++ > MAX_OTA_FAILURES) {
        acc->allocated = false;
        acc->active = false;
        acc->model = OBJECT_OTA_FAIL;
        
        SendObjectConnectionState(slot);
      }
    }

    EnterState(RADIO_PAIRING);
    uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
    return ;
  }

  ota_send_next_block();
}

static void radio_prepare(void) {
  if (radioState == RADIO_OTA) {
    return ;
  }

  // Schedule our next radio prepare
  uesb_stop();

    // Transmit to accessories round-robin
  if (++currentAccessory >= TICK_LOOP) {
    currentAccessory = 0;
    ++_tapTime;
  }

  if (currentAccessory >= MAX_ACCESSORIES) return ;

  AccessorySlot* target = &accessories[currentAccessory];
  
  if (target->active && ++target->last_received < ACCESSORY_TIMEOUT) {
    #ifdef CUBE_HOP
    if (target->hopSkip) {
      target->hopSkip = false;
      
      // This just send garbage and return to pairing mode when finished
      EnterState(RADIO_PAIRING);
      uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
      
      return ;
    }
    #endif

    // We send the previous LED state (so we don't get jitter on radio)    
    RobotHandshake tx_state;
    tx_state.msg_id = 0;
    tx_state.tapTime = _tapTime;

    // Update the color status of the lights   
    static const int channel_order[] = { 3, 2, 1, 0 };
    int tx_index = 0;
        
    memset(tx_state.ledStatus, 0, sizeof(tx_state.ledStatus));
    const int num_lights = (target->model == OBJECT_CHARGER) ? 3 : 4;
    
    for (int light = 0; light < num_lights; light++) {
      uint8_t* rgbi = (uint8_t*) &lightController.cube[currentAccessory][channel_order[light]].values;

      for (int ch = 0; ch < 3; ch++) {
        #ifndef NATHAN_CUBE_JUNK
        tx_state.ledStatus[tx_index++] = (rgbi[ch] * light_gamma) >> 8;
        #else
        tx_state.ledStatus[tx_index++] = 0x80;
        #endif
      }
    }

    #ifdef CUBE_HOP
    // Perform first RF Hop
    target->hopChannel += target->hopIndex;
    if (target->hopChannel >= 53) {
      target->hopChannel -= 53;
    }
    target->address.rf_channel = target->hopChannel + 4;
    if (target->address.rf_channel >= target->hopBlackout) {
      target->address.rf_channel += 22;
    }
    #endif

    // Broadcast to the appropriate device
    EnterState(RADIO_TALKING);
    uesb_prepare_tx_payload(&target->address, &tx_state, sizeof(tx_state));
  } else {
    // Timeslice is empty, send a dummy command on the channel so people know to stay away
    if (target->active)
    {
      // Spread the remaining accessories forward as a patch fix
      // Simply reset the timeout of all accessories
      for (int i = 0; i < MAX_ACCESSORIES; i++) {
        target->last_received = 0;
      }

      target->active = false;

      SendObjectConnectionState(currentAccessory);
    }
    
    // This just send garbage and return to pairing mode when finished
    EnterState(RADIO_PAIRING);
    uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
  }
}

static void radio_resume(void) {
  // Reenable the radio
  uesb_start();
}

extern "C" void TIMER0_IRQHandler(void) {
  // We are in bluetooth mode, do not do this
  if (m_uesb_mainstate == UESB_STATE_UNINITIALIZED) {
    return ;
  }
    
  if (ota_pending && ota_timeout_countdown && !--ota_timeout_countdown) {
    ota_timeout();
  }
  
  if (NRF_TIMER0->EVENTS_COMPARE[PREPARE_COMPARE]) {
    NRF_TIMER0->EVENTS_COMPARE[PREPARE_COMPARE] = 0;

    radio_prepare();
  }

  if (NRF_TIMER0->EVENTS_COMPARE[RESUME_COMPARE]) {
    NRF_TIMER0->EVENTS_COMPARE[RESUME_COMPARE] = 0;

    radio_resume();
  }
}
