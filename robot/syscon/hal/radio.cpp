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

#ifdef DUMP_DISCOVER
#include "head.h"
#endif


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

struct LEDPacket {
  uint8_t ledStatus[16]; // 4-LEDs, three colors
  uint8_t ledDark;       // Dark byte
};

struct AccessorySlot {
  bool              active;
  bool              allocated;
  int               last_received;
  uint32_t          id;
  LEDPacket         tx_state;
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

static const int TICK_LOOP = TOTAL_PERIOD / SCHEDULE_PERIOD;

static const int LOCATE_TIMEOUT = TICK_LOOP;      // One entire cozmo period
static const int ACCESSORY_TIMEOUT = 400;         // 2s timeout before accessory is considered lost
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
static const int CUBE_TALK_PIPE = 0;

static const int ADV_CHANNEL = 81;

// This is for initial channel selection (do not use advertisement channel)
static const int MAX_TX_CHANNEL = 80;

static const int RADIO_INTERVAL_DELAY = 0xB6;
//static const int RADIO_TIMEOUT_MSB = 15;
static const int RADIO_TIMEOUT_MSB = 35;
static const int RADIO_WAKEUP_OFFSET = 18;

static const int MAX_ADDRESS_BIT_RUN = 3;
static const int BASE_PIPE = 1;

// Global head / body sync values
extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Current status values of cubes/chargers
static RadioState        radioState;

static const uesb_address_desc_t AdvertiseAddress = {
  ADV_CHANNEL,
  UNUSED_BASE, 
  ADVERTISE_BASE,
  ADVERTISE_PREFIX,
  0xFF
};

static uesb_address_desc_t TalkAddress = {
  0,
  ADVERTISE_BASE,
  UNUSED_BASE,
  COMMUNICATE_PREFIX,
  0xFF
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

// Integer square root calculator
uint32_t isqrt(uint32_t a_nInput)
{
    uint32_t op  = a_nInput;
    uint32_t res = 0;
    uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type


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

void Radio::init() {
  const uesb_config_t uesb_config = {
    UESB_BITRATE_1MBPS,
    UESB_CRC_8BIT,
    UESB_TX_POWER_0DBM,
    PACKET_SIZE,
    5,    // Address length
    2     // Service speed doesn't need to be that fast (prevent blocking encoders)
  };

  // Generate target address for the robot
  //Random::get(&TalkAddress.base1, sizeof(TalkAddress.base1));
  TalkAddress.base1 = fixAddress(TalkAddress.base1);

  // Clear our our states
  memset(accessories, 0, sizeof(accessories));
  currentAccessory = 0;
  
  // Start the radio stack
  uesb_init(&uesb_config);
  EnterState(RADIO_FIND_CHANNEL);
  uesb_start();

  RTOS::schedule(Radio::manage, SCHEDULE_PERIOD);

  // Set all the cubes to white until something better comes along
  for (int slot = 0; slot < MAX_ACCESSORIES; slot++) {
    static const uint16_t reset_state[] = {
      0x001F,
      0x03E0,
      0x7C00,
      0x7FFF
    };
    setPropState(slot, reset_state);
  }
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
    case RADIO_FIND_CHANNEL:
      locateTimeout = LOCATE_TIMEOUT;  
      TalkAddress.rf_channel = 0x20;
      memset(currentNoiseLevel, 0, sizeof(currentNoiseLevel));
      uesb_set_rx_address(&TalkAddress);
      break ;
    case RADIO_PAIRING:
      uesb_set_rx_address(&AdvertiseAddress);
      break;
    case RADIO_TALKING:
      uesb_set_rx_address(&TalkAddress);
      break ;
  }
}

static void send_dummy_byte(void) {
  // This just send garbage and return to pairing mode when finished
  EnterState(RADIO_PAIRING);
  uesb_write_tx_payload(&TalkAddress, 0, NULL, 0);
}

static void send_capture_packet(void* userdata) {
  int slot = (int) userdata;

  // Send a pairing packet
  CapturePacket pair;

  pair.target_channel = TalkAddress.rf_channel;
  pair.interval_delay = RADIO_INTERVAL_DELAY;
  pair.prefix = TalkAddress.prefix[BASE_PIPE+slot];
  memcpy(&pair.base, &TalkAddress.base1, sizeof(TalkAddress.base1));
  pair.timeout_msb = RADIO_TIMEOUT_MSB;
  pair.wakeup_offset = RADIO_WAKEUP_OFFSET;

  // Tell this accessory to come over to my side
  uesb_write_tx_payload(&AdvertiseAddress, ROBOT_PAIR_PIPE, &pair, sizeof(CapturePacket));
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
      if (rx_payload.pipe != CUBE_ADVERT_PIPE) {
        break ;
      }

      AdvertisePacket packet;
      memcpy(&packet, &rx_payload.data, sizeof(AdvertisePacket));

      // Attempt to locate existing accessory and repair
      slot = LocateAccessory(packet.id);
      if (slot < 0) {
        SpineProtocol msg;
        msg.opcode = PROP_DISCOVERED;
        msg.PropDiscovered.prop_id = packet.id;
                
        Spine::enqueue(msg);
        
        #ifdef DUMP_DISCOVER
        if (packet.id < 0x8000) UART::print("%x\r\n", (uint16_t) packet.id);
        #endif
        
        // Attempt to allocate a slot for it
        slot = FreeAccessory();

        // We cannot find a place for it
        if (slot < 0) {
          break ;
        }
      }

      // We are loading the slot
      accessories[slot].active = true;
      accessories[slot].id = packet.id;
      accessories[slot].last_received = 0;

      // Schedule a one time capture for this slot
      RTOS::schedule(send_capture_packet, CYCLES_MS(0.5f), (void*) slot, false);
      break ;
      
    case RADIO_TALKING:
      if (rx_payload.pipe != CUBE_TALK_PIPE) {
        break ;
      }
      
      AccessorySlot* acc = &accessories[currentAccessory];
      acc->last_received = 0;

      AcceleratorPacket* ap = (AcceleratorPacket*) &rx_payload.data;

      SpineProtocol msg;
      msg.opcode = GET_PROP_STATE;
      msg.GetPropState.x = ap->x;
      msg.GetPropState.y = ap->y;
      msg.GetPropState.z = ap->z;
      msg.GetPropState.shockCount = ap->shockCount;
      Spine::enqueue(msg);

      send_dummy_byte();
      break ;
    }
  }
}

// Calculate the weight of a 16-bit color word
static inline int colorWeight(uint16_t w) {
  static const int squared[] = {
    0, 64, 256, 576, 
    1089, 1681, 2401, 3249, 
    4356, 5476, 6724, 8100, 
    9801, 11449, 13225, 15129, 
    17424, 19600, 21904, 24336, 
    27225, 29929, 32761, 35721, 
    39204, 42436, 45796, 49284, 
    53361, 57121, 61009, 65025
  };

  return 
    squared[(w >> 10) & 0x1F] +
    squared[(w >>  5) & 0x1F] +
    squared[(w >>  0) & 0x1F] +
    squared[(w & 0x8000) ? 0x1F : 0];
}

void Radio::setPropState(unsigned int slot, const uint16_t *state) {
  if (slot > MAX_ACCESSORIES) {
    return ;
  }

  int sum = 0;
  for (int i = 0; i < 4; i++) {
    sum += colorWeight(state[i]);
  }

  int sq_sum = isqrt(sum);

  LEDPacket packet = {
    {
      UNPACK_COLORS(state[0]),
      UNPACK_COLORS(state[1]),
      UNPACK_COLORS(state[2]),
      UNPACK_COLORS(state[3]),
      UNPACK_IR(state[0]),
      UNPACK_IR(state[1]),
      UNPACK_IR(state[2]),
      UNPACK_IR(state[3])
    },
    (sq_sum >= 0xFF) ? 1 : (0x255 - sq_sum)
  };

  AccessorySlot* acc = &accessories[slot];
  memcpy(&acc->tx_state, &packet, sizeof(LEDPacket));
}

void Radio::assignProp(unsigned int slot, uint32_t accessory) {
  if (slot > MAX_ACCESSORIES) {
    return ;
  }
  
  AccessorySlot* acc = &accessories[slot];
  acc->id = accessory;
  acc->allocated = true;
}

void Radio::manage(void* userdata) {
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
    // Transmit to accessories round-robin
    currentAccessory = (currentAccessory + 1) % TICK_LOOP;
    
    if (currentAccessory >= MAX_ACCESSORIES) break ;

    AccessorySlot* acc = &accessories[currentAccessory];

    if (acc->active && ++acc->last_received < ACCESSORY_TIMEOUT) {
      // Broadcast to the appropriate device
      EnterState(RADIO_TALKING);
      uesb_write_tx_payload(&TalkAddress, BASE_PIPE+currentAccessory, &acc->tx_state, sizeof(LEDPacket));
    } else {
      // Timeslice is empty, send a dummy command on the channel so people know to stay away
      acc->active = false;
      send_dummy_byte();
    }

    break ;
  }
}
