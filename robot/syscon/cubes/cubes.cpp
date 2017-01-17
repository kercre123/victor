#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "protocol.h"
#include "hardware.h"
#include "cubes.h"
#include "head.h"
#include "lights.h"
#include "messages.h"
#include "portable.h"

#include "clad/robotInterface/messageFromActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

//#define AXIS_DEBUGGER
//#define CUBE_HOP

using namespace Anki::Cozmo;

static void radio_stop(void);
static void ReportUpAxisChanged(uint8_t id, UpAxis a);
static void SendObjectConnectionState(int slot);
static void UpdatePropTaps(uint8_t id, int shocks, uint8_t tapTime, int8_t tapNeg, int8_t tapPos);
static void UpdatePropMotion(uint8_t id, int ax, int ay, int az);

// Accelerometer IIR filter vars
static const Fixed one = TO_FIXED(1.0);
static const Fixed filter_coeff = TO_FIXED(0.1);

// Motion computation thresholds and vars
static const u8 START_MOVING_COUNT_THRESH = 5;    // Determines number of motion tics that much be observed before Moving msg is sent
static const u8 STOP_MOVING_COUNT_THRESH = 5;     // Determines number of no-motion tics that much be observed before StoppedMoving msg is sent
static const u8 ACC_MOVE_THRESH = 3;              // If current value differs from filtered value by this much, block is considered to be moving.

static const u8 UPAXIS_STABLE_COUNT_THRESH = 15;  // How long the candidate upAxis must be observed for before it is reported

extern "C" const CubeFirmware CurrentCubeFirmware;

// Axes are swapped coming out of the cube so swap them here before reporting to engine
static const UpAxis idxToUpAxis[3][2] = { {YNegative, YPositive},
                                          {ZPositive, ZNegative},
                                          {XNegative, XPositive} };
extern GlobalDataToHead g_dataToHead;

// TX/RX FIFO
static union {
  // Possible inbound packets
  AccessoryHandshake  accessoryHandshake;
  AdvertisePacket     advertisePacket;

  // Possible outbound packets
  RobotHandshake      robotHandshake;
  CapturePacket       capturePacket;
  OTAFirmwareBlock    firmwareBlock;

  // Raw data pointer
  uint8_t             radio_data[];
};

// Current status values of cubes/chargers
RadioMainstate        radio_mainstate;
static bool           tx_queued;
static AccessorySlot  accessories[MAX_ACCESSORIES];
static uint8_t        currentAccessory;
static int            light_gamma;
static uint8_t        _tapTime;
static bool           sendDiscovery;
static bool           wasTapped;
static unsigned int   lastSlot = MAX_ACCESSORIES;
static bool           handshakeReceived = false;
static int            handshakeSlot = 0;

// Which slot should accelerometer data be streaming for
static int streamAccelSlot = -1;

#ifdef CUBE_HOP
static int8_t         _wifiChannel;
#endif

// Function that swaps the bits within each byte in a uint32. Used to convert from nRF24L type addressing to nRF51 type addressing
static inline uint32_t bytewise_bit_swap(uint32_t inp)
{
  inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
  inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
  return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}

static inline uint8_t random() {
  static uint8_t c = 0xFF;
  c = (c >> 1) ^ (c & 1 ? 0x2D : 0);
  return c;
}

void Radio::init() {
  light_gamma = 0x100;

  // Firmware is invalid (or simply not embedded)
  // This is a crappy assert
  while (CurrentCubeFirmware.magic != CUBE_FIRMWARE_MAGIC) ;
}

void Radio::advertise(void) {
  // Clear our our states
  currentAccessory = 0;
  memset(accessories, 0, sizeof(accessories));
  tx_queued = false;
  wasTapped = false;
  handshakeReceived = false;
  sendDiscovery = true;
  streamAccelSlot = -1;

  // ==== Configure Radio
  NRF_RADIO->POWER = 1;

  // Framing
  NRF_RADIO->PCNF0 =  (0 << RADIO_PCNF0_S0LEN_Pos) | 
                      (0 << RADIO_PCNF0_LFLEN_Pos) | 
                      (0 << RADIO_PCNF0_S1LEN_Pos);

  // TX power
  NRF_RADIO->TXPOWER = TX_OUTPUT_POWER << RADIO_TXPOWER_TXPOWER_Pos;

  // RF bitrate
  NRF_RADIO->MODE    = RF_BITRATE << RADIO_MODE_MODE_Pos;

  // CRC configuration
  NRF_RADIO->CRCCNF  = RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos;
  NRF_RADIO->CRCINIT = 0xFFFFUL;      // Initial value
  NRF_RADIO->CRCPOLY = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1
  
  // Address configuration
  NRF_RADIO->TXADDRESS   = 0;
  NRF_RADIO->RXADDRESSES = 1;

  NRF_RADIO->INTENCLR    = 0xFFFFFFFF;
  NRF_RADIO->SHORTS      = RADIO_SHORTS_READY_START_Msk | 
                           RADIO_SHORTS_END_DISABLE_Msk |
                           RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                           RADIO_SHORTS_DISABLED_RSSISTOP_Msk;
  NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;

  NVIC_SetPriority(RADIO_IRQn, RADIO_SERVICE_PRIORITY);

  radio_mainstate = UESB_STATE_IDLE;

  // ==== Timer scheduling
  NRF_RTC0->POWER = 1;

  NRF_RTC0->TASKS_STOP = 1;
  NRF_RTC0->TASKS_CLEAR = 1;

  NRF_RTC0->PRESCALER = 0;

  NRF_RTC0->INTENCLR = ~0;
  NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk |
                       RTC_INTENSET_COMPARE1_Msk;

  int sync_ticks = NRF_RTC1->CC[0] - NRF_RTC1->COUNTER;
  NRF_RTC0->CC[PREPARE_COMPARE] = sync_ticks + SCHEDULE_PERIOD;
  NRF_RTC0->CC[RESUME_COMPARE] = sync_ticks + SILENCE_PERIOD;

  NVIC_SetPriority(RTC0_IRQn, RADIO_TIMER_PRIORITY);
  NVIC_EnableIRQ(RTC0_IRQn);

  NRF_RTC0->TASKS_START = 1;
}

static void configure_rf(void* packet, uint32_t address, uint8_t channel, uint32_t payload_length) {
  // Packet format
  NRF_RADIO->PCNF1 =  (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
                      (RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
                      ((RF_ADDR_LENGTH - 1)         << RADIO_PCNF1_BALEN_Pos)   |
                      (payload_length               << RADIO_PCNF1_STATLEN_Pos) |
                      (payload_length               << RADIO_PCNF1_MAXLEN_Pos);

  // Physical addresses
  NRF_RADIO->PREFIX0 = address >> 24;
  NRF_RADIO->BASE0   = address << 8;
  NRF_RADIO->FREQUENCY = channel;

  NRF_RADIO->PACKETPTR   = (uint32_t)packet;
}

static void Radio::resume() {
  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn);

  if (tx_queued) {
    // Dequeue fifo
    radio_mainstate = UESB_STATE_PTX;
    NRF_RADIO->TASKS_TXEN  = 1;
  } else {
    radio_mainstate = UESB_STATE_PRX;
    // Pre-configure addresses for receive
    NRF_RADIO->TASKS_RXEN  = 1;
  }

  return ;
}

void Radio::shutdown(void) {
  // === Teardown timer
  NRF_RTC0->TASKS_STOP = 1;
  NVIC_DisableIRQ(RTC0_IRQn);

  // === Shutdown radio
  radio_stop();
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->EVENTS_RSSIEND = 0;
  NRF_RADIO->EVENTS_BCMATCH = 0;;

  NRF_RADIO->SHORTS = 0;
  
  NRF_RADIO->INTENCLR = 0xFFFFFFFF;
  
  NVIC_SetPriority(RADIO_IRQn, 0);
}

static void radio_stop(void)
{
  NVIC_DisableIRQ(RADIO_IRQn);
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED) ;

  // Clear out all the possible events
  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->EVENTS_ADDRESS = 0;
  NRF_RADIO->EVENTS_PAYLOAD = 0;
}

static int LocateAccessory(uint32_t id) {
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (!accessories[i].allocated) continue ;
    if (accessories[i].id == id) return i;
  }

  return -1;
}

#ifdef AXIS_DEBUGGER
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


static void onAdvertisePacket() {
  // Our radio is busy, make no attempt to respond to out of loop devices
  if(tx_queued) {
    return ;
  }

  // This is an invalid hardware version and we should not try to do anything with it
  if (advertisePacket.hwVersion != CurrentCubeFirmware.hwVersion) {
    return ;
  }
  
  // Attempt to locate existing accessory and repair
  const uint32_t advertId = bytewise_bit_swap(advertisePacket.id);
  int slot = LocateAccessory(advertId);

  #if not defined(FACTORY) && not defined(RELEASE)
  // This is the magical cube of silence
  if (advertId == 0xde682fd0) {
    static const SetAudioVolume msg = { 0 };
    RobotInterface::SendMessage(msg);
  }
  #endif

  #if defined(AXIS_DEBUGGER)
  if (slot < 0 && NRF_RADIO->RSSISAMPLE < 50) {
    slot = AllocateAccessory(advertId);
  }
  #endif

  if (slot < 0) {
    if (sendDiscovery) {
      ObjectDiscovered msg;
      msg.device_type = advertisePacket.model;
      msg.factory_id = advertId;
      msg.rssi = NRF_RADIO->RSSISAMPLE;
      RobotInterface::SendMessage(msg);
    }

    // Do not auto allocate the cube
    return ;
  }

  // We are loading the slot
  AccessorySlot* acc = &accessories[slot]; 

  acc->id = advertId;
  acc->last_received = 0;
  acc->model = advertisePacket.model;
  acc->shockCount = 0;
  acc->powerCountdown = 0;

  // Clear stateful flags of newly allocated object
  if (acc->active == false)
  {
    acc->allocated = true;
    acc->active = true;
    acc->movingTimeoutCtr = 0;
    acc->isMoving = false;
    acc->prevMotionDir = Unknown;
    acc->prevUpAxis = Unknown;
    acc->nextUpAxis = Unknown;
    acc->nextUpAxisCount = 0;
    
    SendObjectConnectionState(slot);
  }

  // This is where the cube shall live
  acc->id = advertId;

  if (advertisePacket.patchLevel & ~CurrentCubeFirmware.patchLevel) {
    // Reset our block count
    acc->ota_block_index = 0;
    acc->updating = true;
    
    // Select a random channel to perform OTA on
    acc->rf_channel = (random() & 0x3F) + 4;

    // Tell our device to begin - must set EVERY field
    capturePacket.ticksUntilStart = 132; // Lowest legal value
    capturePacket.hopIndex = 0;
    capturePacket.hopBlackout = acc->rf_channel;
    capturePacket.patchStart = 0;
  } else {
    acc->updating = false;

    #ifndef CUBE_HOP
    acc->rf_channel = (random() & 0x3F) + 4;

    capturePacket.ticksUntilStart = 0x4000;//132; // Lowest legal value
    capturePacket.hopIndex = 0;
    capturePacket.hopBlackout = acc->rf_channel;
    #else
    // Find the next time accessory will be contacted
    int target_slot = slot - currentAccessory;

    if (target_slot <= 0) {
      target_slot += TICK_LOOP;
    }

    // Note: This is inaccurate, requires nathan to be here with his magic cube to fix
    int clocks = (target_slot * SCHEDULE_PERIOD) + (NRF_RTC0->CC[RESUME_COMPARE] << 8) - (NRF_RTC0->COUNTER << 8);

    if (clocks < SCHEDULE_PERIOD) {
      clocks += RADIO_TOTAL_PERIOD;
      acc->hopSkip = true;
    } else {
      acc->hopSkip = false;
    }

    acc->hopIndex = (random() & 0xF) + 0x12;
    acc->hopBlackout = (_wifiChannel * 5) - 9;
    if (acc->hopBlackout < 0) {
      acc->hopBlackout = 0;
    }
    acc->hopChannel = 0;

    capturePacket.ticksUntilStart = (clocks >> 8) - NEXT_CYCLE_FUDGE;
    capturePacket.hopIndex = acc->hopIndex;
    capturePacket.hopBlackout = acc->hopBlackout;
    #endif

    capturePacket.patchStart = CurrentCubeFirmware.patchStart;
  }

  capturePacket.ticksPerBeat = SCHEDULE_PERIOD;    // 32768/164 = 200Hz
  capturePacket.beatsPerHandshake = TICK_LOOP; // 1 out of 7 beats handshakes with this cube
  capturePacket.ticksToListen = 5;     // Ticks to listen for
  capturePacket.beatsPerRead = 4;
  capturePacket.beatsUntilRead = 4;    // Should be computed to synchronize all tap data

  // Send a pairing packet
  tx_queued = true;
  configure_rf(&radio_data, advertId, ADV_CHANNEL, sizeof(CapturePacket));
}

static void processHandshakePacket() {
  if (!handshakeReceived) {
    return ;
  }
  handshakeReceived = false;

  AccessorySlot* acc = &accessories[handshakeSlot];

  // Handshake
  if (accessoryHandshake.batteryLevel && --acc->powerCountdown <= 0) {
    acc->powerCountdown = SEND_BATTERY_TIME;

    ObjectPowerLevel m;
    m.objectID = handshakeSlot;
    m.missedPackets = acc->messages_sent - acc->messages_received;
    m.batteryLevel = (CUBE_ADC_TO_CENTIVOLTS*accessoryHandshake.batteryLevel)>>8;
    RobotInterface::SendMessage(m);
  }

  UpdatePropTaps(handshakeSlot, accessoryHandshake.tapCount, accessoryHandshake.tapTime, accessoryHandshake.tapNeg, accessoryHandshake.tapPos);
  UpdatePropMotion(handshakeSlot, accessoryHandshake.x, accessoryHandshake.y, accessoryHandshake.z);
}

static bool onHandshakePacket() {
  const uint32_t source_address = (NRF_RADIO->PREFIX0 << 24) | (NRF_RADIO->BASE0 >> 8);
  
  int slot = LocateAccessory(source_address);
  
  if (slot < 0) {
    return false;
  }

  // We will be listening for cubes next
  configure_rf(&radio_data, ADVERTISE_ADDRESS, ADV_CHANNEL, sizeof(AdvertisePacket));

  AccessorySlot* acc = &accessories[slot];
  acc->last_received = 0;

  if (acc->updating) {
    // Send noise and return to pairing
    if (++acc->ota_block_index < CurrentCubeFirmware.dataLen / sizeof(CubeFirmwareBlock)) {
    } else {
      acc->updating = false;
      acc->active = false;
    }
  } else {
    handshakeSlot = slot;
    handshakeReceived = true;
    
    processHandshakePacket();
    
    acc->messages_received++;
  }
  
  return true;
}

static void transmitOTA() {
  AccessorySlot* target = &accessories[currentAccessory];

  // Give up, we didn't receive any acks soon enough
  if (++target->last_received >= MAX_ACK_TIMEOUTS) {
    if (target->failure_count++ > MAX_OTA_FAILURES) {
      target->allocated = false;
      target->active = false;
      target->model = OBJECT_OTA_FAIL;
      
      SendObjectConnectionState(currentAccessory);
    }

    return ;
  }

  // Transmit the next firmware block
  firmwareBlock.messageId = 0xF0 | target->ota_block_index;
  memcpy(firmwareBlock.block, CurrentCubeFirmware.data[target->ota_block_index], sizeof(CubeFirmwareBlock));

  tx_queued = true;
  configure_rf(&radio_data, target->id, target->rf_channel, sizeof(OTAFirmwareBlock));
}

static void transmitHandshake(void) {
  AccessorySlot* target = &accessories[currentAccessory];

  if (++target->last_received >= ACCESSORY_TIMEOUT) {
    // Spread the remaining accessories forward as a patch fix
    // Simply reset the timeout of all accessories
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
      target->last_received = 0;
    }

    target->active = false;

    SendObjectConnectionState(currentAccessory);
    
    return ;
  }
  
  #ifdef CUBE_HOP
  if (target->hopSkip) {
    target->hopSkip = false;
    return ;
  }
  #endif

  // We send the previous LED state (so we don't get jitter on radio)    
  robotHandshake.msg_id = 0;
  robotHandshake.tapTime = _tapTime;

  // Update the color status of the lights
  int tx_index = 0;

  for (int light = 0; light < NUM_PROP_LIGHTS; light++) {
    int index = (light + target->rotationOffset) % NUM_PROP_LIGHTS;
    const uint8_t* rgbi = (uint8_t*) &lightController.cube[currentAccessory][3 - index].values;

    #ifdef AXIS_DEBUGGER
    static const uint8_t COLORS[][3] = {
      { 0x7F, 0x7F, 0x7F },
      { 0x7F, 0x7F,    0 },
      { 0x7F,    0,    0 },
      { 0x7F,    0, 0x7F },
      {    0,    0, 0x7F },
      {    0,    0,    0 },
      {    0, 0x7F,    0 },
      {    0, 0x7F, 0x7F },

    };

    rgbi = COLORS[(light & 2) ? target->prevUpAxis : (target->shockCount & 7)];
    #endif

    for (int ch = 0; ch < 3; ch++) {
      robotHandshake.ledStatus[tx_index++] = (rgbi[ch] * light_gamma) >> 8;
    }
  }

  if (target->model == OBJECT_CHARGER) {
    memset(robotHandshake.ledStatus, 0, 3);
  }

  #ifdef CUBE_HOP
  // Perform first RF Hop
  target->hopChannel += target->hopIndex;
  if (target->hopChannel >= 53) {
    target->hopChannel -= 53;
  }
  target->rf_channel = target->hopChannel + 4;
  if (target->rf_channel >= target->hopBlackout) {
    target->rf_channel += 22;
  }
  #endif

  // Broadcast to the appropriate device
  tx_queued = true;
  configure_rf(&radio_data, target->id, target->rf_channel, sizeof(RobotHandshake));
  target->messages_sent++;
}

void Radio::sendTestPacket(void) {
  const AdvertisePacket test_packet = {
    0x0D0B3D09,
    0xFF03,     // I'm a cube 3
    0x0000,     // Don't patch me, I have them all!
    0x06,       // I'm a pilot/production cube
    0xFF
  };

  radio_stop();
  tx_queued = true;
  configure_rf((void*)&test_packet, ADVERTISE_ADDRESS, ADV_CHANNEL, sizeof(test_packet));
  Radio::resume();
}

void Radio::prepare(void) {
  // Transmit to accessories round-robin
  if (++currentAccessory >= TICK_LOOP) {
    currentAccessory = 0;
    ++_tapTime;
  }

  if (currentAccessory >= MAX_ACCESSORIES) {
    return ;
  }

  AccessorySlot* target = &accessories[currentAccessory];

  // Schedule our next radio prepare
  radio_stop();
  configure_rf(&radio_data, ADVERTISE_ADDRESS, ADV_CHANNEL, sizeof(AdvertisePacket));

  // We are talking to an accessory
  if (target->active) {
    if (target->updating) {
      transmitOTA();
    } else {
      transmitHandshake();
    }
  }
}

extern "C" void RADIO_IRQHandler()
{
  // Clear event
  NRF_RADIO->EVENTS_DISABLED = 0;

  switch(radio_mainstate)
  {
  case UESB_STATE_PRX:
    // CRC Failed
    if (NRF_RADIO->CRCSTATUS == 0) {
      break ;
    }

    if (NRF_RADIO->FREQUENCY == ADV_CHANNEL) {
      onAdvertisePacket();
    } else {
      // Don't resume listening if we received an accessory handshake
      if (onHandshakePacket()) {
        return ;
      }
    }

    break ;
  case UESB_STATE_PTX:
    // Transmission complete
    tx_queued = false;

    break ;
  default:
    break ;
  }

  // Start receiving
  Radio::resume();
}

void Radio::rotate(int frames) {
  if (frames <= 0) {
    return ;
  }

  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    AccessorySlot* target = &accessories[i];
    
    // Not rotating
    if (target->rotationPeriod <= 0) {
      continue ;
    }

    // Have we underflowed ?
    target->rotationNext -= frames;
    while (target->rotationNext <= 0) {
      // Rotate the light
      target->rotationOffset++;
      target->rotationNext += target->rotationPeriod;
    }
  }
}

extern "C" void RTC0_IRQHandler(void) {
  if (NRF_RTC0->EVENTS_COMPARE[PREPARE_COMPARE]) {
    NRF_RTC0->EVENTS_COMPARE[PREPARE_COMPARE] = 0;
    NRF_RTC0->CC[PREPARE_COMPARE] += SCHEDULE_PERIOD;

    Radio::prepare();
  }

  if (NRF_RTC0->EVENTS_COMPARE[RESUME_COMPARE]) {
    NRF_RTC0->EVENTS_COMPARE[RESUME_COMPARE] = 0;
    NRF_RTC0->CC[RESUME_COMPARE] += SCHEDULE_PERIOD;

    Radio::resume();
  }
}


void Radio::enableAccelStreaming(unsigned int slot, bool enable)
{
  if (enable) {
    if (slot < MAX_ACCESSORIES) {
      streamAccelSlot = slot;
    }
  } else if (streamAccelSlot == slot) {
    streamAccelSlot = -1;
  }
}

static void ReportUpAxisChanged(uint8_t id, UpAxis a) {
  ObjectUpAxisChanged m;
  m.timestamp = g_dataToHead.timestamp;
  m.objectID = id;
  m.upAxis = a;
  RobotInterface::SendMessage(m);
}

static void SendObjectConnectionState(int slot) {
  ObjectConnectionStateToRobot msg;
  msg.objectID = slot;
  msg.factoryID = accessories[slot].id;
  msg.connected = accessories[slot].active;
  msg.device_type = accessories[slot].model;
  RobotInterface::SendMessage(msg);
}

// Tap detection
static void UpdatePropTaps(uint8_t id, int shocks, uint8_t tapTime, int8_t tapNeg, int8_t tapPos) {
  AccessorySlot* target = &accessories[id];
  
  uint8_t count = shocks - target->shockCount;
  target->shockCount = shocks;

  // Send unfiltered tap message
  if (count > 0) {
    ObjectTapped m;

    m.timestamp = g_dataToHead.timestamp;
    m.numTaps = count;
    m.objectID = id;
    m.robotID = 0;
    m.tapTime = tapTime;
    m.tapNeg = tapNeg;
    m.tapPos = tapPos;

    RobotInterface::SendMessage(m);
  }

  // Send filtered tap message
  {
    static ObjectTappedFiltered m;

    if (count > 0) {
      // Flush our buffer when transitioning to a new tap
      if (!wasTapped) {
        memset(&m, 0, sizeof(m));
        for (int i = 0; i < MAX_ACCESSORIES; i++) {
          accessories[i].dataReceived = false;
        }
        
        wasTapped = true;
      }

      // Find the highest intensity tap
      uint8_t intensity = tapPos - tapNeg;

      if (m.tapIntensity < intensity) {
        m.timestamp = g_dataToHead.timestamp;
        m.objectID = id;
        m.tapTime = tapTime;
        m.tapIntensity = intensity;
      }
    }

    if (wasTapped) {
      accessories[id].dataReceived = true;

      // Break out if we've not heard from everyone
      for (int i = 0; i < MAX_ACCESSORIES; i++) {
        AccessorySlot *target = &accessories[i];

        // Break when we've not heard from all active cubes
        if (target->active && 
            target->model != OBJECT_CHARGER && 
            !target->dataReceived) {
          return ;
        }
      }

      RobotInterface::SendMessage(m);
      wasTapped = false;
    }
  }
}

// Motion and Axis control
static void UpdatePropMotion(uint8_t id, int ax, int ay, int az) {
  AccessorySlot* target = &accessories[id];

  // Check for motion
  bool xMoving = ABS(FROM_FIXED_FLOOR(target->accFilt[0]) - ax) >= ACC_MOVE_THRESH;
  bool yMoving = ABS(FROM_FIXED_FLOOR(target->accFilt[1]) - ay) >= ACC_MOVE_THRESH;
  bool zMoving = ABS(FROM_FIXED_FLOOR(target->accFilt[2]) - az) >= ACC_MOVE_THRESH;
  bool isMovingNow = xMoving || yMoving || zMoving;

  bool skipFilterStep = false;
  if (isMovingNow) {
    
    // Skip filter accumulation if this is the first few frames of evidence of motion from a non-moving state.
    // This prevents fast high intensity events like taps from influencing the filter.
    skipFilterStep = !target->isMoving && (target->movingTimeoutCtr <= 1);
    
    // Incremenent counter if block is not yet considered moving, or
    // if it is already considered moving and the moving count is less than STOP_MOVING_COUNT_THRESH
    // so that it doesn't need to decrement so much before it's considered to have stopped moving again.
    if (!target->isMoving || (target->movingTimeoutCtr < STOP_MOVING_COUNT_THRESH)) {
      ++target->movingTimeoutCtr;
    }
  } else if (target->movingTimeoutCtr > 0) {
    --target->movingTimeoutCtr;
  }
  
  // Accumulate IIR filter for each axis
  if (!skipFilterStep) {
    target->accFilt[0] = (filter_coeff * ax) + FIXED_MUL(one - filter_coeff, target->accFilt[0]);
    target->accFilt[1] = (filter_coeff * ay) + FIXED_MUL(one - filter_coeff, target->accFilt[1]);
    target->accFilt[2] = (filter_coeff * az) + FIXED_MUL(one - filter_coeff, target->accFilt[2]);
  }

  // Compute motion direction change. i.e. change in dominant acceleration vector
  s8 maxAccelVal = 0;
  UpAxis motionDir = Unknown;

  for (int i=0; i < 3; ++i) {
    s8 absAccel = ABS(FROM_FIXED_FLOOR(target->accFilt[i]));
    if (absAccel > maxAccelVal) {
      motionDir = idxToUpAxis[i][FROM_FIXED_FLOOR(target->accFilt[i]) > 0 ? 0 : 1];
      maxAccelVal = absAccel;
    }
  }
  bool motionDirChanged = (target->prevMotionDir != Unknown) && (target->prevMotionDir != motionDir);
  target->prevMotionDir = motionDir;

  // Check for stable up axis
  // Send ObjectUpAxisChanged msg if it changes
  if (motionDir == target->nextUpAxis) {
    //s32 accSum = ax + ay + az;  // magnitude of 64 is approx 1g

    if (target->nextUpAxis != target->prevUpAxis) {
      if (++target->nextUpAxisCount >= UPAXIS_STABLE_COUNT_THRESH) {
        ReportUpAxisChanged(id, target->nextUpAxis);
        target->prevUpAxis = target->nextUpAxis;
      }
    } else {
      // Next is same as what was already reported so no need to accumulate count
      target->nextUpAxisCount = 0;
    }
  } else {
    target->nextUpAxisCount = 0;
    target->nextUpAxis = motionDir;
  }

  // Send ObjectMoved message if object was stationary and is now moving or if motionDir changes
  if (motionDirChanged ||
      ((target->movingTimeoutCtr >= START_MOVING_COUNT_THRESH) && !target->isMoving)) {
    ObjectMoved m;
    m.timestamp = g_dataToHead.timestamp;
    m.objectID = id;
    m.robotID = 0;
    m.accel.x = -az;   // Flipped for the same reason idxToUpAxis is flipped
    m.accel.y = -ax;
    m.accel.z = ay;
    m.axisOfAccel = motionDir;
    RobotInterface::SendMessage(m);
    target->isMoving = true;
    target->movingTimeoutCtr = STOP_MOVING_COUNT_THRESH;
  } else if ((target->movingTimeoutCtr == 0) && target->isMoving) {

    // If there is an upAxis change to report, then report it.
    // Just make more logical sense if it gets sent before the stopped message.
    if (target->nextUpAxis != target->prevUpAxis) {
      ReportUpAxisChanged(id, target->nextUpAxis);
      target->prevUpAxis = target->nextUpAxis;
    }
    
    // Send stopped message
    ObjectStoppedMoving m;
    m.timestamp = g_dataToHead.timestamp;
    m.objectID = id;
    m.robotID = 0;
    RobotInterface::SendMessage(m);
    target->isMoving = false;
  }
  
  
  // Stream accelerometer data
  if (streamAccelSlot == id) {
    ObjectAccel m;
    m.timestamp = g_dataToHead.timestamp;
    m.objectID = id;
    m.accel.x = -az;   // Flipped for the same reason idxToUpAxis is flipped
    m.accel.y = -ax;
    m.accel.z = ay;
    RobotInterface::SendMessage(m);
  }
}

void Radio::setWifiChannel(int8_t channel) {
  #ifdef CUBE_HOP
  _wifiChannel = channel;
  #endif
}

void Radio::setLightGamma(uint8_t gamma) {
  light_gamma = gamma + 1;
}

void Radio::enableDiscovery(bool enable) {
  sendDiscovery = enable;
}

void Radio::setPropLightsID(unsigned int slot, uint8_t period)
{
  if (slot >= MAX_ACCESSORIES) {
    return ;
  }
  
  lastSlot = slot;

  AccessorySlot* target = &accessories[slot];
  
  target->rotationPeriod = period;
  target->rotationNext = period;
  target->rotationOffset = 0;
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
