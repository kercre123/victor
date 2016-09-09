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

//#define NATHAN_DEBUG
//#define NATHAN_CUBE_JUNK
//#define CUBE_HOP

#define TOTAL_BLOCKS(x) ((x)->dataLen / sizeof(CubeFirmwareBlock))

using namespace Anki::Cozmo;

static void EnterState(RadioState state);

enum TIMER_COMPARE {
  PREPARE_COMPARE,
  RESUME_COMPARE,
  ROTATE_COMPARE
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

static int wifiChannel = 1;

static const uesb_address_desc_t PairingAddress = {
  ADV_CHANNEL,
  ADVERTISE_ADDRESS,
  sizeof(AdvertisePacket)
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

static unsigned int lastSlot = MAX_ACCESSORIES;
static uint8_t rotationPeriod[MAX_ACCESSORIES];
static uint8_t rotationOffset[MAX_ACCESSORIES];
static uint8_t rotationNext[MAX_ACCESSORIES];

void Radio::init() {
  light_gamma = 0x100;
}

void Radio::advertise(void) {
  const uesb_config_t uesb_config = {
    RADIO_MODE_MODE_Nrf_1Mbit,
    UESB_CRC_16BIT,
    RADIO_TXPOWER_TXPOWER_Neg4dBm,
    4,              // Address length
    RADIO_SERVICE_PRIORITY  // IRQ priority for processing inbound messages
  };

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;

  #ifdef NATHAN_CUBE_JUNK
  light_gamma = 0x80;
  LightState colors[NUM_PROP_LIGHTS];
  memset(&colors, 0, sizeof(colors));

  colors[0].onColor = colors[0].offColor = 0x7C00;
  colors[1].onColor = colors[1].offColor = 0x03E0;
  colors[2].onColor = colors[2].offColor = 0x001F;
  colors[3].onColor = colors[3].offColor = 0x7FE0;

  for (int c = 0; c < MAX_ACCESSORIES; c++) {
    Radio::setPropLightsID(c, 20);
    Radio::setPropLights(colors);
  }
  #endif

  uesb_init(&uesb_config);

  // Timer scheduling
  NRF_RTC0->POWER = 1;

  NRF_RTC0->TASKS_STOP = 1;
  NRF_RTC0->TASKS_CLEAR = 1;

  NRF_RTC0->PRESCALER = 0;

  NRF_RTC0->INTENCLR = ~0;
  NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk |
                       RTC_INTENSET_COMPARE1_Msk |
                       RTC_INTENSET_COMPARE2_Msk;

  NRF_RTC0->CC[PREPARE_COMPARE] = SCHEDULE_PERIOD;
  NRF_RTC0->CC[RESUME_COMPARE] = SILENCE_PERIOD;
  NRF_RTC0->CC[ROTATE_COMPARE] = ROTATE_PERIOD;

  NVIC_SetPriority(RTC0_IRQn, RADIO_TIMER_PRIORITY);
  NVIC_EnableIRQ(RTC0_IRQn);

  NRF_RTC0->TASKS_START = 1;
}

void Radio::setWifiChannel(int8_t channel) {
  if (channel > 0) {
    wifiChannel = channel;
  }
}

void Radio::setLightGamma(uint8_t gamma) {
  light_gamma = gamma + 1;
}

void Radio::shutdown(void) {
  NRF_RTC0->TASKS_STOP = 1;
  NVIC_DisableIRQ(RTC0_IRQn);

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
  ObjectConnectionStateToRobot msg;
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
  pair.ticksPerBeat = SCHEDULE_PERIOD;    // 32768/164 = 200Hz
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
    }

    break ;
  case RADIO_PAIRING:
    {
      AdvertisePacket advert;
      memcpy(&advert, &rx_payload.data, sizeof(AdvertisePacket));

      #ifdef NATHAN_DEBUG
      if (advert.id != 0xca11ab1e) {
        return ;
      }
      #endif

      // Attempt to locate existing accessory and repair
      int slot = LocateAccessory(advert.id);

      #ifdef NATHAN_DEBUG
      if (slot < 0) {
        slot = AllocateAccessory(advert.id);
      }
      #elif defined(NATHAN_CUBE_JUNK)
      if (slot < 0 && rx_payload.rssi < 60 && rx_payload.rssi > -60) {
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

      // Note: This is inaccurate, requires nathan to be here with his magic cube to fix
      int clocks_remaining = (int)((NRF_RTC0->CC[RESUME_COMPARE] << 8) - (NRF_RTC0->COUNTER << 8)) >> 8;
      int ticks_to_next = (target_slot * SCHEDULE_PERIOD) - clocks_remaining;

      if (clocks < SCHEDULE_PERIOD) {
        clocks += RADIO_TOTAL_PERIOD;
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

      pair.ticksPerBeat = SCHEDULE_PERIOD;
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

void Radio::setPropLightsID(unsigned int slot, uint8_t period)
{
  if (slot >= MAX_ACCESSORIES) {
    return ;
  }
  
  lastSlot = slot;

  rotationPeriod[slot] = period;
  rotationNext[slot] = period;
  rotationOffset[slot] = 0;
}

void Radio::setPropLights(const LightState *state) {
  if (lastSlot >= MAX_ACCESSORIES) {
    return ;
  }
  
  for (int c = 0; c < NUM_PROP_LIGHTS; c++) {
    Lights::update(lightController.cube[lastSlot][c], &state[c]);
  }
  
  lastSlot = MAX_ACCESSORIES;
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
    return ;
  }

  ota_send_next_block();
}

static void radio_prepare(void) {
  // Schedule our next radio prepare
  uesb_stop();

  // Manage OTA timeouts
  if (radioState == RADIO_OTA) {
    if (ota_pending && --ota_timeout_countdown <= 0) {
      ota_timeout();
    }
    return ;
  }

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
      int index = light + rotationOffset[currentAccessory];
      
      if (index >= num_lights) {
        index -= num_lights;
      }
      
      uint8_t* rgbi = (uint8_t*) &lightController.cube[currentAccessory][channel_order[index]].values;

      for (int ch = 0; ch < 3; ch++) {
        tx_state.ledStatus[tx_index++] = (rgbi[ch] * light_gamma) >> 8;
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
  }
}

extern "C" void RTC0_IRQHandler(void) {
  // We are in bluetooth mode, do not do this
  if (m_uesb_mainstate == UESB_STATE_UNINITIALIZED) {
    return ;
  }

  if (NRF_RTC0->EVENTS_COMPARE[PREPARE_COMPARE]) {
    NRF_RTC0->EVENTS_COMPARE[PREPARE_COMPARE] = 0;
    NRF_RTC0->CC[PREPARE_COMPARE] += SCHEDULE_PERIOD;

    radio_prepare();
  }

  if (NRF_RTC0->EVENTS_COMPARE[RESUME_COMPARE]) {
    NRF_RTC0->EVENTS_COMPARE[RESUME_COMPARE] = 0;
    NRF_RTC0->CC[RESUME_COMPARE] += SCHEDULE_PERIOD;

    uesb_start();
  }
  
  if (NRF_RTC0->EVENTS_COMPARE[ROTATE_COMPARE]) {
    NRF_RTC0->EVENTS_COMPARE[ROTATE_COMPARE] = 0;
    NRF_RTC0->CC[ROTATE_COMPARE] += ROTATE_PERIOD;

    for (int i = 0; i < MAX_ACCESSORIES; i++) {
      // Not rotating
      if (rotationPeriod[i] <= 0) {
        continue ;
      }
      
      // Have we underflowed ?
      if (--rotationNext[i] <= 0) {
        // Rotate the light
        if (++rotationOffset[i] >= NUM_PROP_LIGHTS) {
          rotationOffset[i] = 0;
        }
        
        rotationNext[i] = rotationPeriod[i];
      }
    }
  }
}
