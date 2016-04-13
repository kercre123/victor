#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"
#include "hardware.h"
#include "lights.h"
#include "radio.h"
#include "messages.h"
#include "bluetooth.h"
#include "backpack.h"

#include "clad/robotInterface/messageEngineToRobot.h"

extern GlobalDataToBody g_dataToBody;

using namespace Anki::Cozmo;

static const int QUEUE_DEPTH = 4;
static CladBufferUp spinebuffer[QUEUE_DEPTH];
static volatile int spine_enter = 0;
static volatile int spine_exit  = 0;

static void Process_setBackpackLights(const RobotInterface::BackpackLights& msg)
{
  Backpack::setLights(msg.lights);
}

static void Process_setCubeLights(const CubeLights& msg)
{
  Radio::setPropLights(msg.objectID, msg.lights);
}

static void Process_setPropSlot(const SetPropSlot& msg)
{
  Radio::assignProp(msg.slot, msg.factory_id);
}

static void Process_configureBluetooth(const RobotInterface::ConfigureBluetooth& msg) {
  enterOperatingMode(msg.enable ? BLUETOOTH_OPERATING_MODE : WIFI_OPERATING_MODE);
}

static void Process_bleRecvHelloMessage(const BLE_RecvHello& msg)
{
  Bluetooth::authChallenge(msg);
}

static void Process_bleEnterPairing(const BLE_EnterPairing& msg)
{
  Bluetooth::enterPairing(msg);
}

static void Process_nvReadToBody(const RobotInterface::NVReadResultToBody& msg)
{
}

static void Process_nvOpResultToBody(const RobotInterface::NVOpResultToBody& msg)
{
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
}

void Spine::ProcessHeadData()
{
  using namespace Anki::Cozmo;
  
  const u8 tag = g_dataToBody.cladBuffer.data[0];
  if (g_dataToBody.cladBuffer.length == 0 || tag == RobotInterface::GLOBAL_INVALID_TAG)
  {
    // pass
  }
  else if (tag > RobotInterface::TO_BODY_END)
  {
    AnkiError( 139, "Spine.ProcessMessage", 384, "Body received message %x that seems bound above", 1, tag);
  }
  else
  {
    ProcessMessage(&g_dataToBody.cladBuffer);
  }
}

void Spine::ProcessMessage(void* buf) {
  RobotInterface::EngineToRobot& msg = *reinterpret_cast<RobotInterface::EngineToRobot*>(buf);
  
  switch(msg.tag)
  {
    #include "clad/robotInterface/messageEngineToRobot_switch_from_0x01_to_0x27.def"
    default:
      AnkiError( 140, "Head.ProcessMessage.BadTag", 385, "Message to body, unhandled tag 0x%x", 1, msg.tag);
  }
}

bool HAL::RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
{
  // Forward further down the pip
  if (msgID >= 0x28 && msgID <= 0x2F)
  {
    return Bluetooth::transmit((const uint8_t*)buffer, size, msgID);
  }
    
  const int exit = (spine_exit+1) % QUEUE_DEPTH;
  if (spine_enter == exit) {
    return false;
  }
  else if ((size + 1) > SPINE_MAX_CLAD_MSG_SIZE_UP)
  {
    AnkiError( 128, "Spine.Enqueue.MessageTooLong", 386, "Message %x[%d] too long to enqueue to head. MAX_SIZE = %d", 3, msgID, size, SPINE_MAX_CLAD_MSG_SIZE_UP);
    return false;
  }
  else
  {
    u8* dest = spinebuffer[spine_exit].data;
    if (msgID != 0) {
      *dest = msgID;
      ++dest;
      spinebuffer[spine_exit].length = size + 1;
    }
    else
    {
      spinebuffer[spine_exit].length = size;
    }
    memcpy(dest, buffer, size);
    spine_exit = exit;
    return true;
  }
}

void Spine::Dequeue(CladBufferUp* dest) {
  if (spine_enter == spine_exit)
  {
    dest->length = 0;
  }
  else
  {
    memcpy(dest, &spinebuffer[spine_enter], sizeof(CladBufferUp));
    spine_enter = (spine_enter + 1) % QUEUE_DEPTH;
  }
}
