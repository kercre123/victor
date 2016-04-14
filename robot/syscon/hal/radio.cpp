#include <stdint.h>
#include <string.h>

#include "tests.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"

#include "micro_esb.h"
  
#include "hardware.h"
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

#define AUTO_GATHER

using namespace Anki::Cozmo;

static void EnterState(RadioState state);

// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Current status values of cubes/chargers
static RadioState        radioState;

extern uesb_mainstate_t  m_uesb_mainstate;

static const uesb_address_desc_t PairingAddress = {
  ADV_CHANNEL,
  ADVERTISE_ADDRESS,
	sizeof(AdvertisePacket)
};

static const uesb_address_desc_t NoiseAddress = {
	1,
	0xE7E7E7E7	
};

// Variables for talking to an accessory
static uint8_t currentAccessory;
static AccessorySlot accessories[MAX_ACCESSORIES];

void Radio::init() {
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
      accessories[slot].allocated = true;
      accessories[slot].active = true;
      SendObjectConnectionState(slot);
    }

		// Send a pairing packet		
		{
			
			uesb_address_desc_t address = {
				ADV_CHANNEL,
				packet.id,
			};
			memcpy(&accessories[slot].address, &address, sizeof(address));

			CapturePacket pair;
			// TODO: CONFIGURE HERE

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

			// XXX: THIS IS TEMPORARY
			memset(acc->tx_state.ledStatus, 0x20, sizeof(acc->tx_state.ledStatus));
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

	// XXX: This is temporary
	acc->last_received = 0;
	
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
