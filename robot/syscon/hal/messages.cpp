#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"
#include "hardware.h"
#include "lights.h"
#include "cubes.h"
#include "messages.h"
#include "bluetooth.h"
#include "backpack.h"
#include "battery.h"
#include "storage.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

extern void ResetMissedLogCount();

extern GlobalDataToBody g_dataToBody;

using namespace Anki::Cozmo;

enum QueueSlotState {
  QUEUE_INACTIVE,
  QUEUE_READY
};

struct QueueSlot {
  union {
    uint8_t raw[SPINE_MAX_CLAD_MSG_SIZE_UP];
    struct {
      uint8_t size;
      uint8_t tag;
      uint8_t payload[];
    };
  };

  volatile QueueSlotState state;
};

static const int QUEUE_DEPTH = 8;
static QueueSlot queue[QUEUE_DEPTH];

static void Process_diffieHellmanResults(const DiffieHellmanResults& msg) {
  Bluetooth::diffieHellmanResults(msg);
}

static void Process_setBackpackLayer(const RobotInterface::BackpackSetLayer &msg)
{
  Backpack::setLayer(msg.layer);
}

static void Process_setBackpackLightsMiddle(const RobotInterface::BackpackLightsMiddle& msg)
{
  Backpack::setLightsMiddle(msg.layer, msg.lights);
}

static void Process_setBackpackLightsTurnSignals(const RobotInterface::BackpackLightsTurnSignals& msg)
{
  Backpack::setLightsTurnSignals(msg.layer, msg.lights);
}

static void Process_setCubeID(const CubeID& msg)
{
  Radio::setPropLightsID(msg.objectID, msg.rotationPeriod_frames);
}

static void Process_setCubeLights(const CubeLights& msg)
{
  Radio::setPropLights(msg.lights);
}

static void Process_setCubeGamma(const SetCubeGamma& msg)
{
  Radio::setLightGamma(msg.gamma);
}

static void Process_setPropSlot(const SetPropSlot& msg)
{
  Radio::assignProp(msg.slot, msg.factory_id);
}

static void Process_bodyEnterOTA(const RobotInterface::OTA::BodyEnterOTA& msg) {
  Battery::setOperatingMode(Anki::Cozmo::BODY_OTA_MODE);
}

static void Process_getMfgInfo(const RobotInterface::GetManufacturingInfo& msg)
{
  RobotInterface::ManufacturingID mfgMsg;
  mfgMsg.esn = BODY_ESN;
  mfgMsg.hw_version = BODY_VER;
  RobotInterface::SendMessage(mfgMsg);
}

static void Process_setBodyRadioMode(const SetBodyRadioMode& msg) {
  if (msg.radioMode == BODY_ACCESSORY_OPERATING_MODE) {
    Radio::setWifiChannel(msg.wifiChannel);
  }

  if (msg.radioMode == BODY_BLUETOOTH_OPERATING_MODE) 
  {
    Battery::setHeadlight(false);
    ResetMissedLogCount();
  }

  Battery::setOperatingMode(msg.radioMode);
}

static void Process_enterPairing(const EnterPairing& msg)
{
  Bluetooth::enterPairing(msg);
}

static void Process_setHeadlight(const RobotInterface::SetHeadlight& msg)
{
  Battery::setHeadlight(msg.enable);
}

static void Process_readBodyStorage(const ReadBodyStorage& msg) {
  const uint8_t* data;
  int length;
  
  data = (const uint8_t*) Storage::get((StorageKey)msg.key, length);

  BodyStorageContents reply;
  length -= msg.offset;

  if (length > sizeof(reply.data)) {
    length = sizeof(reply.data);
  } else if (length < 0) {
    length = 0;
  }

  reply.key = msg.key;
  reply.offset = msg.offset;
  reply.data_length = length;
  memcpy(reply.data, &data[msg.offset], length);
  
  RobotInterface::SendMessage(reply);
}

static void Process_writeBodyStorage(const WriteBodyStorage& msg) {
  Storage::set((StorageKey)msg.key, msg.data, msg.data_length);
}

static void Process_killBodyCode(const KillBodyCode& msg)
{
  // This will destroy the first sector in the application layer
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->ERASEPAGE = 0x18000;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NVIC_SystemReset();
}

void Spine::init(void) {
  for (int i = 0; i < QUEUE_DEPTH; i++) {
    queue[i].state = QUEUE_INACTIVE;
  }
}

void Spine::processMessages(const uint8_t* buf) {
  using namespace Anki::Cozmo;

  RobotInterface::EngineToRobot msg;

  int remaining = SPINE_MAX_CLAD_MSG_SIZE_DOWN;

  while (remaining > 0) {
    int size = *(buf++);
    
    if (size == 0) {
      break ;
    }

    memcpy(msg.GetBuffer(), buf, size);
    buf += size;
    remaining -= size + 1;

    if (msg.tag <= RobotInterface::TO_BODY_END) {
      switch(msg.tag)
      {
        #include "clad/robotInterface/messageEngineToRobot_switch_from_0x01_to_0x27.def"
        case RobotInterface::GLOBAL_INVALID_TAG:
          // pass
          break ;
        default:
          AnkiError( 140, "Head.ProcessMessage.BadTag", 385, "Message to body, unhandled tag 0x%x", 1, msg.tag);
          break ;
      }
    } else {
      AnkiError( 139, "Spine.ProcessMessage", 384, "Body received message %x that seems bound above", 1, msg.tag);
    }
  }
}

void Spine::dequeue(uint8_t* dest) {
  static int queue_exit = 0;
  int remaining = SPINE_MAX_CLAD_MSG_SIZE_UP;
  
  // Dequeue as many messages as we possibly can fit in a bundle
  while (queue[queue_exit].state == QUEUE_READY) {
    QueueSlot *msg = &queue[queue_exit];
    int total_size = msg->size + 1;

    if (total_size > remaining) {
      break ;
    }

    memcpy(dest, msg->raw, total_size);
    dest += total_size;
    remaining -= total_size;

    msg->state = QUEUE_INACTIVE;
    queue_exit = (queue_exit + 1) % QUEUE_DEPTH;
  }

  // We need to clear out trailing garbage
  memset(dest, 0, remaining);
}

bool HAL::RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
{
  // Forward further down the pipe
  if (msgID >= 0x28 && msgID <= 0x2F)
  {
    return Bluetooth::transmit((const uint8_t*)buffer, size, msgID);
  }
  
  // Error cases for spine communication
  if (size > SPINE_MAX_CLAD_MSG_SIZE_UP)
  {
    AnkiError( 128, "Spine.Enqueue.MessageTooLong", 386, "Message %x[%d] too long to enqueue to head. MAX_SIZE = %d", 3, msgID, size, SPINE_MAX_CLAD_MSG_SIZE_UP);
    return false;
  }
  else if (msgID == 0)
  {
    return true;
  }

  static int queue_enter = 0;

  // Slot is not available for dequeueing
  __disable_irq();
  if (queue[queue_enter].state != QUEUE_INACTIVE) {
    __enable_irq();
    return false;
  }

  // Critical section for dequeueing message
  QueueSlot* slot = &queue[queue_enter];
  queue_enter = (queue_enter + 1) % QUEUE_DEPTH;
  __enable_irq();

  slot->size = size + 1;
  slot->tag = msgID;
  memcpy(slot->payload, buffer, size);

  // Message is stage and ready for transmission
  slot->state = QUEUE_READY;
  return true;
}
