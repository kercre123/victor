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

#define RELAY_BUFFER_SIZE (256)
extern "C" bool AcceptRTIPMessage(uint8_t* payload, uint8_t length)
{
  static uint8 relayBuffer[RELAY_BUFFER_SIZE];
  static uint8 relayQueued = 0;
  if (clientConnected())
  {
    if (length > (RELAY_BUFFER_SIZE - relayQueued))
    {
      AnkiError( 50, "RTIP.AcceptRTIPMessage", 288, "overflow! %d > %d - %d", 3, length, RELAY_BUFFER_SIZE, relayQueued);
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
        if (relayQueued >= size)
        {
          const bool reliable = relayBuffer[1] & RTIP_CLAD_MSG_RELIABLE_FLAG;
          const bool hot      = relayBuffer[1] & RTIP_CLAD_MSG_HOT_FLAG;
          //GLOBAL_INVALID_TAG = 0, fighting include nightmare
          AnkiConditionalError(clientSendMessage(relayBuffer + 2, size, 0, reliable, hot), 50, "RTIP.AcceptRTIPMessage", 289, "Couldn't relay message (%x[%d]) from RTIP over wifi", 2, relayBuffer[2], size);
          relayQueued -= size + 2;
          os_memcpy(relayBuffer, relayBuffer + size + 2, relayQueued);
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
