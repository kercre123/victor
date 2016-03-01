/** Impelementation for RTIP interface on Espressif
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "driver/i2spi.h"
#include "anki/cozmo/robot/drop.h"
#include "client.h"
}
#include "anki/cozmo/robot/logging.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"

namespace Anki {
namespace Cozmo {
namespace RTIP {

#define TICK_TIME (5000)
#define BYTES_PER_TICK (256)

u32 Version;
u32 Date;
char VersionDescription[VERSION_DESCRIPTION_SIZE]; 

bool Init()
{
  return true;
}

bool SendMessage(u8* buffer, u16 bufferSize)
{
  static uint32_t lastCallTime = 0;
  static uint16_t sentThisTick = 0;
  const uint32_t now = system_get_time();
  const uint32_t timeSinceLastCall = now - lastCallTime;
  const uint32_t flushed = BYTES_PER_TICK * timeSinceLastCall / TICK_TIME;
  sentThisTick = flushed > sentThisTick ? 0 : sentThisTick - flushed;
  AnkiConditionalErrorAndReturnValue((bufferSize + sentThisTick) <= BYTES_PER_TICK,  false, 30, "RTIP", 197, "SendMessage: Not enough buffer on RTIP", 0);
  AnkiConditionalErrorAndReturnValue(bufferSize <= DROP_TO_RTIP_MAX_VAR_PAYLOAD,     false, 30, "RTIP", 198, "SendMessage: Message too large for RTIP, %d > %d", 2, bufferSize, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
  AnkiConditionalErrorAndReturnValue(i2spiQueueMessage(buffer, bufferSize), false, 30, "RTIP", 199, "SendMessage: Couldn't forward message %x[%d] to RTIP", 2, buffer[0], bufferSize);
  return true;
}

bool SendMessage(RobotInterface::EngineToRobot& msg)
{
  return SendMessage(msg.GetBuffer(), msg.Size());
}

#define RELAY_BUFFER_SIZE (384)
ct_assert(RELAY_BUFFER_SIZE > RTIP_MAX_CLAD_MSG_SIZE + DROP_TO_WIFI_MAX_PAYLOAD);

extern "C" bool AcceptRTIPMessage(uint8_t* payload, uint8_t length)
{
  static uint8 relayBuffer[RELAY_BUFFER_SIZE];
  static uint8 relayQueued = 0;
  AnkiConditionalErrorAndReturnValue(length <= RTIP_MAX_CLAD_MSG_SIZE, false, 50, "RTIP.AcceptRTIPMessage", 396, "Impossible message length %d > RTIP_MAX_CLAD_MSG_SIZE (%d)", 2, length, RTIP_MAX_CLAD_MSG_SIZE);
  if (length > (RELAY_BUFFER_SIZE - relayQueued))
  {
    AnkiError( 50, "RTIP.AcceptRTIPMessage", 288, "overflow! %d > %d - %d", 3, length, RELAY_BUFFER_SIZE, relayQueued);
    AnkiConditionalError(relayQueued < 2, 50, "RTIP.AcceptRTIPMessage", 290, "Waiting for message of size %d", 1, relayBuffer[0]);
    relayQueued = 0;
    return false;
  }
  else
  {
    os_memcpy(relayBuffer + relayQueued, payload, length);
    relayQueued += length;
    while (relayQueued > 1) // Have a header +
    {
      const u8 size = relayBuffer[0];
      const u8 sizeWHeader = size + 1;
      if (relayQueued >= sizeWHeader)
      {
        const u8 tag = relayBuffer[1];
        if (tag < RobotInterface::TO_WIFI_START)
        {
          AnkiError( 50, "RTIP.AcceptRTIPMessage", 376, "WiFi received message from RTIP, %x[%d] that seems bound below (< 0x%x)", 3, tag, size, (int)RobotInterface::TO_WIFI_START);
        }
        else if (tag <= RobotInterface::TO_WIFI_END) // This message is for us
        {
          Messages::ProcessMessage(relayBuffer + 1, size);
        }
        else // This message is above us
        {
          if (clientConnected())
          {
            AnkiConditionalError(clientSendMessage(relayBuffer + 1, size, RobotInterface::GLOBAL_INVALID_TAG, relayBuffer[0] < RobotInterface::TO_ENG_UNREL, false), 50, "RTIP.AcceptRTIPMessage", 289, "Couldn't relay message (%x[%d]) from RTIP over wifi", 2, tag, size);
          }
        }
        relayQueued -= sizeWHeader;
        os_memcpy(relayBuffer, relayBuffer + sizeWHeader, relayQueued);
      }
      else
      {
        break;
      }
    }
    return true;
  }
}



} // RTIP
} // Cozmo
} // Anki
