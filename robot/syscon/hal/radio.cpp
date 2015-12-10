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
static const int ACCESSORY_TIMEOUT = 20;          // 100ms timeout before accessory is considered lost

static const int PACKET_SIZE = 17; 
static const int MAX_ACCESSORIES = TICK_LOOP;

// Advertising settings
static const uint8_t ROBOT_TO_CUBE_PREFIX = 0x42;
static const uint8_t CUBE_TO_ROBOT_PREFIX = 0x52;

static const uint32_t ADVERTISE_BASE = 0xC2C2C2C2;

#define ADVERTISE_PREFIX    {0xC2, CUBE_TO_ROBOT_PREFIX, ROBOT_TO_CUBE_PREFIX, 1, 2, 3, 4, 5}

static const int ROBOT_PAIR_PIPE = 2;
static const int CUBE_ADVERT_PIPE = 1;

static const uint8_t ADVERTISING_PREFIX[8] = ADVERTISE_PREFIX;

static const int ADV_CHANNEL = 81;

// This is for initial channel selection (do not use advertisement channel)
static const int MAX_TX_CHANNEL = 80;

static const int RADIO_INTERVAL_DELAY = 0xB6;
static const int RADIO_TIMEOUT_MSB = 15;
static const int RADIO_WAKEUP_OFFSET = 8;

static const int MAX_ADDRESS_BIT_RUN = 3;
static const uint8_t PIPE_VALUES[] = {ROBOT_TO_CUBE_PREFIX, 0x95, 0x97, 0x99, 0xA3, 0xA5, 0xA7, 0xA9};
static const int BASE_PIPE = 1;

// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Current status values of cubes/chargers
static RadioState        radioState;

static uint32_t targetAddress;
static uint32_t targetChannel;
static AccessorySlot accessories[MAX_ACCESSORIES];

// State memory
static union {
  // State memory for Channel location
  struct {
    uint8_t currentNoiseLevel[MAX_TX_CHANNEL+1];
    int locateTimeout;
  };
};

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
    ADV_CHANNEL,
    PACKET_SIZE,
    5,  // Address length
    ADVERTISE_BASE,
    ADVERTISE_BASE,       // This will change to the target address once the pairing step finishes
    ADVERTISE_PREFIX,
    0xFF,
    1
  };

  // Generate target address for the robot
  Random::get(&targetAddress, sizeof(targetAddress));
  targetAddress = fixAddress(targetAddress);

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));

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
  uesb_set_rf_channel(ADV_CHANNEL);
  uesb_set_address(UESB_ADDRESS_BASE1, &ADVERTISE_BASE);
  uesb_set_address(UESB_ADDRESS_PREFIX, ADVERTISING_PREFIX);
}

static void TalkCommunicate(void) {
  uesb_set_rf_channel(targetChannel);
  uesb_set_address(UESB_ADDRESS_BASE1, &targetAddress);
  uesb_set_address(UESB_ADDRESS_PREFIX, PIPE_VALUES);
}

static void EnterState(RadioState state) { 
  radioState = state;

  switch (state) {
    case RADIO_FIND_CHANNEL:
      targetAddress = 0;
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

extern "C" void uesb_event_handler(void)
{
  uint32_t rf_interrupts = uesb_get_clear_interrupts();
  static bool turn_around = false;
  int slot;
  
  // Recevied a packet
  if(rf_interrupts & UESB_INT_RX_DR_MSK) {
    uesb_payload_t rx_payload;
    uesb_read_rx_payload(&rx_payload);

    switch (radioState) {
    case RADIO_FIND_CHANNEL:
      currentNoiseLevel[targetChannel]++;
      break ;
    case RADIO_PAIRING:
      slot = FreeAccessory();
      
      if (rx_payload.pipe == CUBE_ADVERT_PIPE && slot >= 0) {
        AdvertisePacket packet;
        memcpy(&packet, &rx_payload.data, sizeof(AdvertisePacket));
        ReleaseAccessory(packet.id);

        // We don't want more than one charger paired (sorry about it)
        if (AccType(packet.id) == ACCESSORY_CHARGER && CountAccessories(ACCESSORY_CHARGER) >= 1) {
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

        pair.target_channel = targetChannel;
        pair.interval_delay = RADIO_INTERVAL_DELAY;
        pair.prefix = PIPE_VALUES[BASE_PIPE+slot];
        pair.base = targetAddress;
        pair.timeout_msb = RADIO_TIMEOUT_MSB;
        pair.wakeup_offset = RADIO_WAKEUP_OFFSET;

        uesb_write_tx_payload(ROBOT_PAIR_PIPE, &pair, sizeof(CapturePacket));
      }
      
      break ;
      
    case RADIO_TALKING:
      /*
      SADLY, THIS WILL BE A PAIN IN THE DICK, SINCE IT ALWAYS COMES TO PIPE 0
      uint8_t addr = rx_payload.data[sizeof(AcceleratorPacket)] - CUBE_BASE_ADDR;
      if (addr < MAX_CUBES) {
        memcpy((uint8_t*)&cubeRx[addr], rx_payload.data, sizeof(AcceleratorPacket));
      }
      */
      
      static const uint8_t dummy_data = 0xC2;
      uesb_write_tx_payload(ROBOT_PAIR_PIPE, &dummy_data, sizeof(dummy_data));
      turn_around  = true;
    
      break ;
    }
  }
  
  if(rf_interrupts & UESB_INT_TX_SUCCESS_MSK) {
    if (turn_around) {
      turn_around  = false;
      TalkAdvertise();
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
    uesb_stop();
    if (locateTimeout-- <= 0) {
      locateTimeout = LOCATE_TIMEOUT;

      if (currentNoiseLevel[targetChannel] == 0) {
        // Found a quiet place to sleep in
        EnterState(RADIO_PAIRING);
      } else if (targetChannel >= MAX_TX_CHANNEL) {
        // We've reached the end of the usable frequency range, simply
        // pick quietest spot
        uint8_t noiseLevel = ~0;
        
        // Run to the quietest channel
        for (int i = 0; i <= MAX_TX_CHANNEL; i++) {
          if (currentNoiseLevel[i] < noiseLevel) {
            targetChannel = i;
            noiseLevel = currentNoiseLevel[i];
          }
        }
        
        EnterState(RADIO_PAIRING);
      } else {
        // Probe the next channel
        uesb_set_rf_channel(++targetChannel);
      }
    }
    uesb_start();
    break ;
  
  default:
    // Transmit to accessories round-robin
    static uint8_t currentAccessory = 0;
    currentAccessory = (currentAccessory + 1) % TICK_LOOP;
    
    if (currentAccessory >= MAX_ACCESSORIES) break ;

    AccessorySlot* acc = &accessories[currentAccessory];
    if (acc->active) {
      if (++acc->last_received < ACCESSORY_TIMEOUT) {
        EnterState(RADIO_TALKING);
        uesb_write_tx_payload(BASE_PIPE+currentAccessory, &acc->tx_state, sizeof(LEDPacket));
      } else {
        acc->active = false;
      }
    } else {
      // Timeslice is empty
      uesb_stop();
      EnterState(RADIO_PAIRING);
      uesb_start();
    }

    break ;
  }
}
