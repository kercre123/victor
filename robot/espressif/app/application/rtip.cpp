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

#define MIN(a,b) (a < b) ? a : b

bool Init()
{
  Version = 0;
  Date = 0;
  VersionDescription[0] = 0;
  return true;
}

bool SendMessage(RobotInterface::EngineToRobot& msg)
{
  static uint32_t lastCallTime = 0;
  static uint16_t sentThisTick = 0;
  const uint32_t now = system_get_time();
  const uint32_t timeSinceLastCall = now - lastCallTime;
  const uint32_t flushed = BYTES_PER_TICK * timeSinceLastCall / TICK_TIME;
  sentThisTick = flushed > sentThisTick ? 0 : sentThisTick - flushed;
  AnkiConditionalErrorAndReturnValue((msg.Size() + sentThisTick) <= BYTES_PER_TICK,  false, 30, "RTIP", 197, "SendMessage: Not enough buffer on RTIP", 0);
  AnkiConditionalErrorAndReturnValue(msg.Size() <= DROP_TO_RTIP_MAX_VAR_PAYLOAD,     false, 30, "RTIP", 198, "SendMessage: Message too large for RTIP, %d > %d", 2, msg.Size(), DROP_TO_RTIP_MAX_VAR_PAYLOAD);
  AnkiConditionalErrorAndReturnValue(i2spiQueueMessage(msg.GetBuffer(), msg.Size()), false, 30, "RTIP", 199, "SendMessage: Couldn't forward message to RTIP", 0);
  return true;
}

void UpdateVersionInfo(RobotInterface::RTIPVersionInfo& info)
{
  int i = 0;
  info.description_length = MIN(info.description_length, VERSION_DESCRIPTION_SIZE-1);
  while (i < info.description_length)
  {
    VersionDescription[i] = info.description[i];
    i++;
  }
  while (i < VERSION_DESCRIPTION_SIZE)
  {
    VersionDescription[i] = 0;
    i++;
  }
  Version = info.version;
  Date    = info.date;
}

#define RELAY_BUFFER_SIZE (384)
ct_assert(RELAY_BUFFER_SIZE > RTIP_MAX_CLAD_MSG_SIZE + DROP_TO_WIFI_MAX_PAYLOAD);

extern "C" bool AcceptRTIPMessage(uint8_t* payload, uint8_t length)
{
  static uint8 relayBuffer[RELAY_BUFFER_SIZE];
  static uint8 relayQueued = 0;
  if (clientConnected())
  {
    if (length > (RELAY_BUFFER_SIZE - relayQueued))
    {
      AnkiError( 50, "RTIP.AcceptRTIPMessage", 288, "overflow! %d > %d - %d", 3, length, RELAY_BUFFER_SIZE, relayQueued);
      AnkiConditionalError(relayQueued < 2, 50, "RTIP.AcceptRTIPMessage", 290, "Waiting for message of size %d", 1, (relayBuffer[0] | ((relayBuffer[1] & RTIP_CLAD_SIZE_HIGH_MASK) << 8)));
      relayQueued = 0;
      return false;
    }
    else
    {
      os_memcpy(relayBuffer + relayQueued, payload, length);
      relayQueued += length;
      while (relayQueued > 2) // Have a header +
      {
        const uint16 size = relayBuffer[0] | ((relayBuffer[1] & RTIP_CLAD_SIZE_HIGH_MASK) << 8);
        const uint16 sizeWHeader = size + 2;
        if (relayQueued >= sizeWHeader)
        {
          const bool reliable = relayBuffer[1] & RTIP_CLAD_MSG_RELIABLE_FLAG;
          const bool hot      = relayBuffer[1] & RTIP_CLAD_MSG_HOT_FLAG;
          if (relayBuffer[2] == RobotInterface::RobotToEngine::Tag_rtipVersion)
          {
            //RobotInterface::RTIPVersionInfo info;
            //os_memcpy(info.GetBuffer(), relayBuffer + 3, size - 1);
            //UpdateVersionInfo(info);
          }
          else
          {
            AnkiConditionalError(
              clientSendMessage(relayBuffer + 2, size, 0 /* GLOBAL_INVALID_TAG, fighting include nightmare here */, reliable, hot),
              50, "RTIP.AcceptRTIPMessage", 289, "Couldn't relay message (%x[%d]) from RTIP over wifi", 2, relayBuffer[2], size);
          }
          relayQueued -= size + 2;
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
  else return true;
}



} // RTIP
} // Cozmo
} // Anki
