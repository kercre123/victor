#include <stdint.h>
#include <string.h>

#include "tests.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "micro_esb.h"
  
#include "hardware.h"
#include "debug.h"
#include "radio.h"
#include "timer.h"
#include "head.h"
#include "rng.h"

#include "anki/cozmo/robot/spineData.h"

#define ABS(x)   ((x < 0) ? -x : x)

enum AccessoryType {
  ACCESSORY_CUBE    = 0x00,
  ACCESSORY_CHARGER = 0x80
};

enum RadioState {
  RADIO_FIND_CHANNEL,   // We are locating an empty channel to broadcast on
  RADIO_PAIRING,        // We are listening for pairing results
  RADIO_TALKING         // We are communicating to cubes
};

struct AccessorySlot {
  bool              active;
  int               last_received;
  uint32_t          id;
  LEDPacket         tx_state;
  AcceleratorPacket rx_state;
};

struct AdvertisePacket {
  uint32_t id;
};

struct CapturePacket {
  uint8_t target_channel;
  uint8_t interval_delay;
  uint8_t prefix;
  uint32_t base;
  uint8_t timeout_msb;
  uint8_t wakeup_offset;
};

static void EnterState(RadioState state);

static const int TICK_LOOP = 7;                   // 35ms period
static const int LOCATE_TIMEOUT = TICK_LOOP;      // One entire cozmo period
static const int ACCESSORY_TIMEOUT = 30;          // 150ms timeout before accessory is considered lost

static const int PACKET_SIZE = 17; 
static const int MAX_ACCESSORIES = TICK_LOOP;

// Advertising settings
static const uint8_t ROBOT_TO_CUBE_PREFIX = 0x42;
static const uint8_t CUBE_TO_ROBOT_PREFIX = 0x52;

static const uint32_t UNUSED_BASE = 0xE6E6E6E6;
static const uint32_t ADVERTISE_BASE = 0xC2C2C2C2;

#define ADVERTISE_PREFIX    {0xE6, ROBOT_TO_CUBE_PREFIX, CUBE_TO_ROBOT_PREFIX, 1, 2, 3, 4, 5}
#define COMMUNICATE_PREFIX  {CUBE_TO_ROBOT_PREFIX, 0x95, 0x97, 0x99, 0xA3, 0xA5, 0xA7, 0xA9}

static const int ROBOT_PAIR_PIPE = 1;
static const int CUBE_ADVERT_PIPE = 2;

static const int ADV_CHANNEL = 81;

// This is for initial channel selection (do not use advertisement channel)
static const int MAX_TX_CHANNEL = 80;

static const int RADIO_INTERVAL_DELAY = 0xB6;
static const int RADIO_TIMEOUT_MSB = 15;
static const int RADIO_WAKEUP_OFFSET = 8;

static const int MAX_ADDRESS_BIT_RUN = 3;
static const uint8_t PIPE_VALUES[] = COMMUNICATE_PREFIX;
static const int BASE_PIPE = 1;

static const int MAX_CHARGERS = 1;
static const int MAX_CUBES = 6;


// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Current status values of cubes/chargers
static RadioState        radioState;

static const uesb_address_desc_t AdvertiseAddress = {
  ADV_CHANNEL,
  UNUSED_BASE, 
  ADVERTISE_BASE,
  ADVERTISE_PREFIX
};
static uesb_address_desc_t TalkAddress = { 
  0,
  ADVERTISE_BASE,
  0, 
  COMMUNICATE_PREFIX 
};

static AccessorySlot accessories[MAX_ACCESSORIES];

// Variables for locating a quiet channel
static uint8_t currentNoiseLevel[MAX_TX_CHANNEL+1];
static uint8_t currentAccessory;
static int locateTimeout;

// This verifies there are not bit strings too long
static inline uint32_t fixAddress(uint32_t word) {
  int run = 0;
  bool bit;
  
  // Trim MSB
  word &= 0xFFFFFFFE;
  
  for (int i = 1; i < 32; i++) {
    bool found = (word >> i) & 1;
    
    if (found != bit) {
      bit = found;
      run = 1;
    } else if (++run > MAX_ADDRESS_BIT_RUN) {
      word ^= 1 << i;
      bit = !bit;
      run = 1;
    }
  }
  
  return word;
}

void Radio::init() {
  const uesb_config_t uesb_config = {
    UESB_BITRATE_1MBPS,
    UESB_CRC_8BIT,
    UESB_TX_POWER_0DBM,
    PACKET_SIZE,
    5,    // Address length
    0xFF,
    2     // Service speed doesn't need to be that fast (prevent blocking encoders)
  };

  // Generate target address for the robot
  Random::get(&TalkAddress.base1, sizeof(TalkAddress.base1));
  TalkAddress.base1 = fixAddress(TalkAddress.base1);

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;
  
  // Start the radio stack
  uesb_init(&uesb_config);
  EnterState(RADIO_FIND_CHANNEL);
  uesb_start();
}

static inline uint8_t AccType(uint32_t id) {
  return id >> 24;
}

static int CountAccessories(uint8_t type) {
  int count;

  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (accessories[i].active && AccType(accessories[i].id) == type)  count++;
  }

  return count;
}

static void ReleaseAccessory(uint32_t id) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (accessories[i].id == id) accessories[i].active = false;
  }
}

static int FreeAccessory(void) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (!accessories[i].active) return i;
  }

  return -1;
}

static void TalkAdvertise(void) {
  uesb_set_rx_address(&AdvertiseAddress);
}

static void TalkCommunicate(void) {
  uesb_set_rx_address(&TalkAddress);
}

static void EnterState(RadioState state) { 
  radioState = state;

  switch (state) {
    case RADIO_FIND_CHANNEL:
      locateTimeout = LOCATE_TIMEOUT;  
      TalkAddress.rf_channel = 0;
      memset(currentNoiseLevel, 0, sizeof(currentNoiseLevel));
      TalkCommunicate();
      break ;
    case RADIO_PAIRING:
      TalkAdvertise();
      break;
    case RADIO_TALKING:
      TalkCommunicate();
      break ;
  }
}

static bool turn_around = false;
static void send_dummy_byte(void) {
  // This just send garbage
  static const uint8_t dummy_data = 0xC2;
  uesb_write_tx_payload(&TalkAddress, 0, &dummy_data, sizeof(dummy_data));
  turn_around  = true;
}

extern "C" void uesb_event_handler(void)
{
  uint32_t rf_interrupts = uesb_get_clear_interrupts();
  int slot;
  
  // Recevied a packet
  if(rf_interrupts & UESB_INT_RX_DR_MSK) {
    uesb_payload_t rx_payload;
    uesb_read_rx_payload(&rx_payload);

    switch (radioState) {
    case RADIO_FIND_CHANNEL:
      currentNoiseLevel[TalkAddress.rf_channel]++;
      break ;
    case RADIO_PAIRING:
      slot = FreeAccessory();
      
      if (rx_payload.pipe != CUBE_ADVERT_PIPE || slot < 0) {
        break ;
      }

      AdvertisePacket packet;
      memcpy(&packet, &rx_payload.data, sizeof(AdvertisePacket));
      ReleaseAccessory(packet.id);

      // We don't want more than one charger paired (sorry about it)
      if ((AccType(packet.id) == ACCESSORY_CHARGER && CountAccessories(ACCESSORY_CHARGER) >= MAX_CHARGERS) ||
          (AccType(packet.id) == ACCESSORY_CUBE && CountAccessories(ACCESSORY_CUBE) >= MAX_CUBES)) {
        break ;
      }

      // We are loading the slot
      accessories[slot].active = true;
      accessories[slot].id = packet.id;
      accessories[slot].last_received = 0;        
      
      memset(&accessories[slot].rx_state, 0, sizeof(accessories[slot].rx_state));
      memset(&accessories[slot].tx_state, 0xFF, sizeof(accessories[slot].tx_state));
      
      // Send a pairing packet
      CapturePacket pair;

      pair.target_channel = TalkAddress.rf_channel;
      pair.interval_delay = RADIO_INTERVAL_DELAY;
      pair.prefix = PIPE_VALUES[BASE_PIPE+slot];
      pair.base = TalkAddress.base1;
      pair.timeout_msb = RADIO_TIMEOUT_MSB;
      pair.wakeup_offset = RADIO_WAKEUP_OFFSET;

      // Tell this accessory to come over to my side
      uesb_write_tx_payload(&AdvertiseAddress, ROBOT_PAIR_PIPE, &pair, sizeof(CapturePacket));
      
      break ;
      
    case RADIO_TALKING:
      AccessorySlot* acc = &accessories[currentAccessory];   
      memcpy(&acc->rx_state, rx_payload.data, sizeof(AcceleratorPacket));

      send_dummy_byte();
      break ;
    }
  }
  
  if(rf_interrupts & UESB_INT_TX_SUCCESS_MSK) {
    if (turn_around) {
      turn_around  = false;
      EnterState(RADIO_PAIRING);
    }
  }
}

void Radio::manage() {
  // This maintains the spine communication for cube updates (this will eventually be reworked
  if (Head::spokenTo) {
    AccessorySlot* acc = &accessories[g_dataToHead.cubeToUpdate];
    
    // Transmit to cube our line status
    if (g_dataToBody.cubeToUpdate < MAX_ACCESSORIES) {
      memcpy(&g_dataToHead.cubeStatus, &acc->rx_state, sizeof(AcceleratorPacket));    
      memcpy(&acc->tx_state, &g_dataToBody.cubeStatus, sizeof(LEDPacket));
    }

    // Transmit to the head our cube status
    if (++g_dataToHead.cubeToUpdate >= MAX_ACCESSORIES) {
      g_dataToHead.cubeToUpdate = 0;
    }
  }

  // Handle per 5ms channel updates
  switch (radioState) {
  case RADIO_FIND_CHANNEL:
    if (locateTimeout-- <= 0) {
      locateTimeout = LOCATE_TIMEOUT;

      uesb_stop();
      if (currentNoiseLevel[TalkAddress.rf_channel] == 0) {
        // Found a quiet place to sleep in
        EnterState(RADIO_PAIRING);
      } else {
        if ((TalkAddress.rf_channel += 13) > MAX_TX_CHANNEL) {
          // This trys to space the robots apart (the 7 is carefully picked)
          TalkAddress.rf_channel -= MAX_TX_CHANNEL;
        }

        // a zero means a wrap around
        if (!TalkAddress.rf_channel) {
          // We've reached the end of the usable frequency range, simply
          // pick quietest spot
          uint8_t noiseLevel = ~0;
          
          // Run to the quietest channel
          for (int i = 0; i <= MAX_TX_CHANNEL; i++) {
            if (currentNoiseLevel[i] < noiseLevel) {
              TalkAddress.rf_channel = i;
              noiseLevel = currentNoiseLevel[i];
            }
          }

          EnterState(RADIO_PAIRING);
        }
      }
      uesb_start();
    }

    break ;
  
  default:
    uesb_address_desc_t cube_classic = {
      82, 0xE7E7E7E7, 0xC2C2C2C2, {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8}
    };
    LEDPacket packet;
    
    memset(&packet, 0xC0, sizeof(packet));
    uesb_write_tx_payload(&cube_classic, 2, &packet, sizeof(packet));
    
    break ;
    // Transmit to accessories round-robin
    currentAccessory = (currentAccessory + 1) % TICK_LOOP;
    
    if (currentAccessory >= MAX_ACCESSORIES) break ;

    AccessorySlot* acc = &accessories[currentAccessory];
    EnterState(RADIO_TALKING);
    
    if (acc->active && ++acc->last_received < ACCESSORY_TIMEOUT) {
      // Broadcast to the appropriate device
      uesb_write_tx_payload(&TalkAddress, BASE_PIPE+currentAccessory, &acc->tx_state, sizeof(LEDPacket));
    } else {
      // Timeslice is empty, send a dummy command on the channel so people know to stay away
      acc->active = false;
      send_dummy_byte();
    }

    break ;
  }
}
