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

extern void enterOperatingMode(Anki::Cozmo::BodyRadioMode mode);

extern GlobalDataToBody g_dataToBody;

using namespace Anki::Cozmo;

enum QueueSlotState {
  QUEUE_INACTIVE,
  QUEUE_READY
};

struct QueueSlot{
  CladBufferUp buffer;
  volatile QueueSlotState state;
};

static const int QUEUE_DEPTH = 4;
static QueueSlot queue[QUEUE_DEPTH];

static void Process_diffieHellmanResults(const DiffieHellmanResults& msg) {
  Bluetooth::diffieHellmanResults(msg);
}

static void Process_setBackpackLights(const RobotInterface::BackpackLights& msg)
{
  Backpack::setLights(msg.lights);
}

static void Process_setCubeLights(const CubeLights& msg)
{
  Radio::setPropLights(msg.objectID, msg.lights);
}

static void Process_setCubeGamma(const SetCubeGamma& msg)
{
  Radio::setLightGamma(msg.gamma);
}

static void Process_setPropSlot(const SetPropSlot& msg)
{
  Radio::assignProp(msg.slot, msg.factory_id);
}

static void Process_bodyRestart(const RobotInterface::OTA::BodyRestart& msg) {
  NVIC_SystemReset();
}

static void Process_setBodyRadioMode(const SetBodyRadioMode& msg) {
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

static void Process_nvReadToBody(const RobotInterface::NVReadResultToBody& msg)
{
}

static void Process_nvOpResultToBody(const RobotInterface::NVOpResultToBody& msg)
{
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

void Spine::processMessage(void* buf) {
  using namespace Anki::Cozmo;
  RobotInterface::EngineToRobot& msg = *reinterpret_cast<RobotInterface::EngineToRobot*>(buf);

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

void Spine::dequeue(CladBufferUp* dest) {
  static int queue_exit = 0;

  dest->length = 0;

  if (queue[queue_exit].state != QUEUE_READY) {
    return ;
  }

  // Dequeue and deallocate slot
  memcpy(dest, &queue[queue_exit].buffer, sizeof(CladBufferUp));
  queue[queue_exit].state = QUEUE_INACTIVE;
  queue_exit = (queue_exit + 1) % QUEUE_DEPTH;
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
  int index = queue_enter;
  queue_enter = (queue_enter + 1) % QUEUE_DEPTH;
  __enable_irq();

  queue[index].buffer.length = size + 1;
  queue[index].buffer.msgID = msgID;
  memcpy(&queue[index].buffer.data, buffer, size);

  // Message is stage and ready for transmission
  queue[index].state = QUEUE_READY;
  return true;
}
