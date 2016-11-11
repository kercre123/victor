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
//#define AXIS_DEBUGGER
//#define CUBE_HOP

#define TOTAL_BLOCKS(x) ((x)->dataLen / sizeof(CubeFirmwareBlock))

using namespace Anki::Cozmo;

static void EnterState(RadioState state);
static void ClearActiveObjectData(uint8_t slot);

enum TIMER_COMPARE {
  PREPARE_COMPARE,
  RESUME_COMPARE,
  ROTATE_COMPARE
};

// Global head / body sync values
extern uesb_mainstate_t  m_uesb_mainstate;

// This is our internal copy of the cube firmware update
extern "C" const CubeFirmware __CUBE_XS6;

static const uesb_address_desc_t PairingAddress = {
  ADV_CHANNEL,
  ADVERTISE_ADDRESS,
  sizeof(AdvertisePacket)
};

// These are variables used for handling OTA
static uesb_address_desc_t OTAAddress = { 0x63, 0, sizeof(OTAFirmwareBlock) };
static const CubeFirmware* const ota_device = &__CUBE_XS6;
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

// Motion computation thresholds and vars
static const u8 START_MOVING_COUNT_THRESH = 2; // Determines number of motion tics that much be observed before Moving msg is sent
static const u8 STOP_MOVING_COUNT_THRESH = 5;  // Determines number of no-motion tics that much be observed before StoppedMoving msg is sent
static const u8 ACC_MOVE_THRESH = 5;           // If current value differs from filtered value by this much, block is considered to be moving.
static u8 movingTimeoutCtr[MAX_ACCESSORIES];
static bool isMoving[MAX_ACCESSORIES];

static UpAxis prevMotionDir[MAX_ACCESSORIES];  // Last sent motion direction
static UpAxis prevUpAxis[MAX_ACCESSORIES];     // Last sent upAxis
static UpAxis nextUpAxis[MAX_ACCESSORIES];     // Candidate next upAxis to send
static u8 nextUpAxisCount[MAX_ACCESSORIES] = {0};          // Number of times candidate next upAxis is observed
static const u8 UPAXIS_STABLE_COUNT_THRESH = 15;     // How long the candidate upAxis must be observed for before it is reported
static const UpAxis idxToUpAxis[3][2] = { {XPositive, XNegative},
                                          {YPositive, YNegative},
                                          {ZPositive, ZNegative} };
extern GlobalDataToHead g_dataToHead;

static bool sendDiscovery;

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
  
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    ClearActiveObjectData(i);
  }

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
  
  sendDiscovery = true;
}

void Radio::setWifiChannel(int8_t channel) {
  // Stub for now
}

int8_t getWifiChannel()
{
  // Stub for now
  return 1;
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

static void ClearActiveObjectData(uint8_t slot)
{
  movingTimeoutCtr[slot] = 0;
  isMoving[slot] = false;
  
  prevMotionDir[slot] = Unknown;
  prevUpAxis[slot] = Unknown;
  nextUpAxis[slot] = Unknown;
  nextUpAxisCount[slot] = 0;
}

static void ReportUpAxisChanged(uint8_t id, UpAxis a)
{
  ObjectUpAxisChanged m;
  m.timestamp = g_dataToHead.timestamp;
  m.objectID = id;
  m.upAxis = a;
  RobotInterface::SendMessage(m);
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

void Radio::enableDiscovery(bool enable) {
  sendDiscovery = enable;
}


void UpdatePropState(uint8_t id, int ax, int ay, int az, int shocks,
                uint8_t tapTime, int8_t tapNeg, int8_t tapPos) {
  // Tap detection
  uint8_t count = shocks - accessories[id].shockCount;

  accessories[id].shockCount = shocks;

  u32 currTime_ms = g_dataToHead.timestamp;
  if (count > 0 && !accessories[id].ignoreTaps) {
    ObjectTapped m;
    m.timestamp = currTime_ms;
    m.numTaps = count;
    m.objectID = id;
    m.robotID = 0;
    m.tapTime = tapTime;
    m.tapNeg = tapNeg;
    m.tapPos = tapPos;
    RobotInterface::SendMessage(m);
  }
  accessories[id].ignoreTaps = false;

  // Accumulate IIR filter for each axis
  static s32 acc_filt[MAX_ACCESSORIES][3];
  static const s32 filter_coeff = 20;
  acc_filt[id][0] = ((filter_coeff * ax) + ((100 - filter_coeff) * acc_filt[id][0])) / 100;
  acc_filt[id][1] = ((filter_coeff * ay) + ((100 - filter_coeff) * acc_filt[id][1])) / 100;
  acc_filt[id][2] = ((filter_coeff * az) + ((100 - filter_coeff) * acc_filt[id][2])) / 100;

  // Check for motion
  bool xMoving = ABS(acc_filt[id][0] - ax) > ACC_MOVE_THRESH;
  bool yMoving = ABS(acc_filt[id][1] - ay) > ACC_MOVE_THRESH;
  bool zMoving = ABS(acc_filt[id][2] - az) > ACC_MOVE_THRESH;
  bool isMovingNow = xMoving || yMoving || zMoving;

  if (isMovingNow) {
    // Incremenent counter if block is not yet considered moving, or
    // if it is already considered moving and the moving count is less than STOP_MOVING_COUNT_THRESH
    // so that it doesn't need to decrement so much before it's considered stop moving again.
    if (!isMoving[id] || (movingTimeoutCtr[id] < STOP_MOVING_COUNT_THRESH)) {
      ++movingTimeoutCtr[id];
    }
  } else if (movingTimeoutCtr[id] > 0) {
    --movingTimeoutCtr[id];
  }

  // Compute motion direction change. i.e. change in dominant acceleration vector
  s8 maxAccelVal = 0;
  UpAxis motionDir = Unknown;

  for (int i=0; i < 3; ++i) {
    s8 absAccel = ABS(acc_filt[id][i]);
    if (absAccel > maxAccelVal) {
      motionDir = idxToUpAxis[i][acc_filt[id][i] > 0 ? 0 : 1];
      maxAccelVal = absAccel;
    }
  }
  bool motionDirChanged = (prevMotionDir[id] != Unknown) && (prevMotionDir[id] != motionDir);
  prevMotionDir[id] = motionDir;


  // Check for stable up axis
  // Send ObjectUpAxisChanged msg if it changes
  if (motionDir == nextUpAxis[id]) {
    //s32 accSum = ax + ay + az;  // magnitude of 64 is approx 1g

    if (nextUpAxis[id] != prevUpAxis[id]) {
      if (++nextUpAxisCount[id] >= UPAXIS_STABLE_COUNT_THRESH) {
        ReportUpAxisChanged(id, nextUpAxis[id]);
        prevUpAxis[id] = nextUpAxis[id];
      }
    } else {
      // Next is same as what was already reported so no need to accumulate count
      nextUpAxisCount[id] = 0;
    }
  } else {
    nextUpAxisCount[id] = 0;
    nextUpAxis[id] = motionDir;
  }

  // Send ObjectMoved message if object was stationary and is now moving or if motionDir changes
  if (motionDirChanged ||
      ((movingTimeoutCtr[id] >= START_MOVING_COUNT_THRESH) && !isMoving[id])) {
    ObjectMoved m;
    m.timestamp = g_dataToHead.timestamp;
    m.objectID = id;
    m.robotID = 0;
    m.accel.x = ax;
    m.accel.y = ay;
    m.accel.z = az;
    m.axisOfAccel = motionDir;
    RobotInterface::SendMessage(m);
    isMoving[id] = true;
    movingTimeoutCtr[id] = STOP_MOVING_COUNT_THRESH;
  } else if ((movingTimeoutCtr[id] == 0) && isMoving[id]) {

    // If there is an upAxis change to report, then report it.
    // Just make more logical sense if it gets sent before the stopped message.
    if (nextUpAxis[id] != prevUpAxis[id]) {
      ReportUpAxisChanged(id, nextUpAxis[id]);
      prevUpAxis[id] = nextUpAxis[id];
    }
    
    // Send stopped message
    ObjectStoppedMoving m;
    m.timestamp = g_dataToHead.timestamp;
    m.objectID = id;
    m.robotID = 0;
    RobotInterface::SendMessage(m);
    isMoving[id] = false;
  }
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

void uesb_event_handler(uesb_payload_t& rx_payload)
{
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

      // Attempt to locate existing accessory and repair
      int slot = LocateAccessory(advert.id);

      #if defined(NATHAN_CUBE_JUNK)
      if (slot < 0 && ABS(rx_payload.rssi) < 50) {
        slot = AllocateAccessory(advert.id);
      }
      #endif

      if (slot < 0) {
        if (sendDiscovery) {
          ObjectDiscovered msg;
          msg.device_type = advert.model;
          msg.factory_id = advert.id;
          msg.rssi = rx_payload.rssi;
          RobotInterface::SendMessage(msg);
        }

        // Do not auto allocate the cube
        break ;
      }

      // Unrecognized device
      if (ota_device->magic != CUBE_FIRMWARE_MAGIC) {
        return ;
      }

      // This is an invalid hardware version and we should not try to do anything with it
      if (advert.hwVersion != ota_device->hwVersion) {
        return ;
      }

      // Check if the device firmware is out of date
      if (advert.patchLevel & ~ota_device->patchLevel) {
        OTARemoteDevice(advert.id);
        return ;
      }
      
      // We are loading the slot
      AccessorySlot* acc = &accessories[slot]; 
      acc->id = advert.id;
      acc->last_received = 0;
      acc->model = advert.model;
      acc->ignoreTaps = true;
      acc->powerCountdown = 0;
      
      if (acc->active == false)
      {
        acc->allocated = true;
        acc->active = true;
        
        SendObjectConnectionState(slot);
        ClearActiveObjectData(slot);
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
      
      pair.ticksUntilStart = 0x4000;//132; // Lowest legal value
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
      acc->hopBlackout = (getWifiChannel() * 5) - 9;
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

      pair.ticksToListen = 5;     // Currently unused
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
      
      if (slot < 0 || slot >= MAX_ACCESSORIES) {
        return ;
      }

      AccessorySlot* acc = &accessories[slot];
      
      acc->last_received = 0;

      if (--acc->powerCountdown <= 0) {
        acc->powerCountdown = SEND_BATTERY_TIME;

        ObjectPowerLevel m;
        m.objectID = slot;
        m.batteryLevel = (361*ap->batteryLevel)>>8;
        RobotInterface::SendMessage(m);
      }
 
      UpdatePropState(slot, ap->x, ap->y, ap->z, ap->tap_count, ap->tapTime, ap->tapNeg, ap->tapPos);

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
  // Manage OTA timeouts
  if (radioState == RADIO_OTA) {
    if (ota_pending && --ota_timeout_countdown <= 0) {
      ota_timeout();
    }
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

    for (int light = 0; light < NUM_PROP_LIGHTS; light++) {
      int index = (light + rotationOffset[currentAccessory]) % NUM_PROP_LIGHTS;

      #ifndef AXIS_DEBUGGER
      uint8_t* rgbi = (uint8_t*) &lightController.cube[currentAccessory][channel_order[index]].values;
      #else
      static const uint8_t COLORS[][3] = {
        { 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF,    0 },
        { 0xFF,    0,    0 },
        { 0xFF,    0, 0xFF },
        {    0,    0, 0xFF },
        {    0,    0,    0 },
        {    0, 0xFF,    0 },
        {    0, 0xFF, 0xFF },

      };
      int color = (light & 2) ? prevUpAxis[currentAccessory] : (target->shockCount & 7);
      const uint8_t* rgbi = COLORS[color];
      #endif

      for (int ch = 0; ch < 3; ch++) {
        tx_state.ledStatus[tx_index++] = (rgbi[ch] * light_gamma) >> 8;
      }
    }

    if (target->model == OBJECT_CHARGER) {
      memset(tx_state.ledStatus, 0, 3);
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
        rotationOffset[i]++;
        rotationNext[i] = rotationPeriod[i];
      }
    }
  }
}
