#ifndef LEGACY_CUBES
#include <stdint.h>
#include <string.h>

#include "tests.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"

#include "micro_esb.h"
  
#include "protocol.h"
#include "random.h"
#include "hardware.h"
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

// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

extern uesb_mainstate_t  m_uesb_mainstate;

// This is our internal copy of the cube firmware update
extern "C" const uint32_t     CUBE_FIRMWARE_LENGTH;
extern "C" const CubeFirmware CUBE_UPDATE;

// Number of 16-byte chunks available
static const int CubeFirmwareBlocks = (CUBE_FIRMWARE_LENGTH+15) / 16 - 1;

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
static int ota_block_index;
static RTOS_Task *ota_task;
static void ota_ack_timeout(void* userdata);
static const int OTA_ACK_TIMEOUT = CYCLES_MS(2);
static const int MAX_ACK_TIMEOUTS = CYCLES_MS(500) / OTA_ACK_TIMEOUT;
static int ack_timeouts;

// Variables for talking to an accessory
static uint8_t currentAccessory;
static AccessorySlot accessories[MAX_ACCESSORIES];

// Current status values of cubes/chargers
static RadioState        radioState;

void Radio::init() {
  ota_task = RTOS::create(ota_ack_timeout, false);
}

void Radio::advertise(void) {
  const uesb_config_t uesb_config = {
    RADIO_MODE_MODE_Nrf_1Mbit,
    UESB_CRC_16BIT,
    RADIO_TXPOWER_TXPOWER_Pos4dBm,
    4,    // Address length
    RADIO_PRIORITY // Service speed doesn't need to be that fast (prevent blocking encoders)
  };

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;

  uesb_init(&uesb_config);
}

void Radio::shutdown(void) {
  uesb_disable();
}

static int LocateAccessory(uint32_t id) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (!accessories[i].allocated) continue ;
    if (accessories[i].id == id) return i;
  }

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
  RobotInterface::SendMessage(msg);
}

static void ota_send_next_block() {
  OTAFirmwareBlock msg;
  
  msg.messageId = 0xF0 | ota_block_index;
  memcpy(msg.block, CUBE_UPDATE.data[ota_block_index], sizeof(CubeFirmwareBlock));
  
  uesb_write_tx_payload(&OTAAddress, &msg, sizeof(OTAFirmwareBlock));
  RTOS::start(ota_task, OTA_ACK_TIMEOUT);
}

static void ota_ack_timeout(void* userdata) {
  // Give up, we didn't receive any acks soon enough
  if (++ack_timeouts >= MAX_ACK_TIMEOUTS) {
    EnterState(RADIO_PAIRING);
    uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
    return ;
  }

  ota_send_next_block();
}

static void OTARemoteDevice(uint32_t id) {
  // Reset our block count
  ota_block_index = 0;
  ack_timeouts = 0;
  
  // This is address
  OTAAddress.address = id;
  OTAAddress.rf_channel = (OTAAddress.rf_channel >> 1) ^ (OTAAddress.rf_channel & 1 ? 0x2D : 0);
  OTAAddress.rf_channel = 80;

  EnterState(RADIO_OTA);

  // Tell our device to begin
  CapturePacket pair;
  pair.hopIndex = 0;
  pair.hopBlackout = OTAAddress.rf_channel;
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

  int slot;

  switch (radioState) {
  case RADIO_OTA:
    // Message acked, kill ota_task
    RTOS::stop(ota_task);

    // Send noise and return to pairing
    if (++ota_block_index >= CubeFirmwareBlocks) {
      EnterState(RADIO_PAIRING);
      uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
      break ;
    }
    
    ack_timeouts = 0;
    ota_send_next_block();
    break ;
  case RADIO_PAIRING:      
    AdvertisePacket advert;
    memcpy(&advert, &rx_payload.data, sizeof(AdvertisePacket));

    // Attempt to locate existing accessory and repair
    slot = LocateAccessory(advert.id);
    if (slot < 0) {
      ObjectDiscovered msg;
      msg.factory_id = advert.id;
      msg.rssi = rx_payload.rssi;
      RobotInterface::SendMessage(msg);
            
      // Do not auto allocate the cube
      break ;
    }

    // Radio firmware header is valid
    if (CUBE_UPDATE.magic == CUBE_FIRMWARE_MAGIC) {
      // This is an invalid hardware version and we should not try to do anything with it
      if (advert.hwVersion != CUBE_UPDATE.hwVersion) {
        break ;
      }

      // Check if the device firmware is out of date
      if (advert.patchLevel & ~CUBE_UPDATE.patchLevel) {
        OTARemoteDevice(advert.id);
        break ;
      }
    }


    // We are loading the slot
    accessories[slot].id = advert.id;
    accessories[slot].last_received = 0;
    if (accessories[slot].active == false)
    {
      accessories[slot].allocated = true;
      accessories[slot].active = true;
      SendObjectConnectionState(slot);
    }

    // Send a pairing packet
    // XXX: FIX ALL THIS SO IT'S NOT DUMB
    {
      // This is where the cube shall live
      uesb_address_desc_t* new_addr = &accessories[slot].address;

      new_addr->address = advert.id;
      new_addr->payload_length = sizeof(AccessoryHandshake);
      new_addr->rf_channel = (new_addr->rf_channel >> 1) ^ (new_addr->rf_channel & 1 ? 0 : 0x2D);
      
      // Tell the cube to listen on channel 42
      uesb_address_desc_t address = {
        ADV_CHANNEL,
        advert.id,
      };
      CapturePacket pair;
      
      pair.hopIndex = 0;
      pair.hopBlackout = new_addr->rf_channel;
      pair.patchStart = CUBE_UPDATE.patchStart;
      
      uesb_write_tx_payload(&address, &pair, sizeof(CapturePacket));
    }
    break ;
    
  case RADIO_TALKING:
    AccessorySlot* acc = &accessories[currentAccessory];
    AccessoryHandshake* ap = (AccessoryHandshake*) &rx_payload.data;

    acc->last_received = 0;

    PropState msg;
    msg.slot = currentAccessory;
    msg.x = ap->x;
    msg.y = ap->y;
    msg.z = ap->z;
    msg.shockCount = ap->tap_count;
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
    for (int c = 0; c < NUM_PROP_LIGHTS; c++) {
      static const uint8_t light_index[NUM_PROP_LIGHTS][3] = {
        {  0,  1,  2 },
        {  3,  4,  5 },
        {  6,  7,  8 },
        {  9, 10, 11 }
      };

      int group = CUBE_LIGHT_INDEX_BASE + CUBE_LIGHT_STRIDE * currentAccessory + c;
      uint8_t* rgbi = Lights::state(group);

      for (int i = 0; i < 3; i++) {
        acc->tx_state.ledStatus[light_index[c][i]] = rgbi[i];
      }
    }
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
    uesb_prepare_tx_payload(&address, &acc->tx_state, sizeof(acc->tx_state));
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
    uesb_prepare_tx_payload(&NoiseAddress, NULL, 0);
  }
}

void Radio::resume(void* userdata) {
  // Reenable the radio
  uesb_start();
}

void Radio::manage(void) {
  // We are in OTA mode which is free running
  if (radioState == RADIO_OTA) {
    return ;
  }

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
#endif
