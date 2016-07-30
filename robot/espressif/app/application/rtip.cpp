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

bool Init()
{
  return true;
}

bool SendMessage(const u8* buffer, const u16 bufferSize, const u8 tag)
{
  if (tag == RobotInterface::GLOBAL_INVALID_TAG)
  {
    AnkiConditionalErrorAndReturnValue(bufferSize <= DROP_TO_RTIP_MAX_VAR_PAYLOAD,     false, 30, "RTIP", 198, "SendMessage: Message too large for RTIP, %d > %d", 2, bufferSize, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
    AnkiConditionalErrorAndReturnValue(i2spiQueueMessage(buffer, bufferSize), false, 30, "RTIP", 199, "SendMessage: Couldn't forward message %x[%d] to RTIP", 2, buffer[0], bufferSize);
    return true;
  }
  else
  {
    u8 msgBuffer[DROP_TO_RTIP_MAX_VAR_PAYLOAD];
    AnkiConditionalErrorAndReturnValue(bufferSize + 1 <= DROP_TO_RTIP_MAX_VAR_PAYLOAD, false, 30, "RTIP", 415, "SendMessage with %x[%d] > %d", 3, tag, bufferSize, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
    msgBuffer[0] = tag;
    os_memcpy(msgBuffer + 1, buffer, bufferSize);
    AnkiConditionalErrorAndReturnValue(i2spiQueueMessage(msgBuffer, bufferSize+1), false, 30, "RTIP", 199, "SendMessage: Couldn't forward message %x[%d] to RTIP", 2, tag, bufferSize+1);
  return true;
  }
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
            if ((tag == RobotInterface::RobotToEngine::Tag_trace) && (clientQueueAvailable() < 200))
            {
              AnkiWarn( 50, "RTIP.AcceptRTIPMessage", 442, "dropping RTIP trace", 0);
            }
            else
            {
              const bool reliable = tag < RobotInterface::TO_ENG_UNREL;
              if (reliable)
              {
                AnkiConditionalError(clientSendMessage(relayBuffer + 1, size, RobotInterface::GLOBAL_INVALID_TAG, true, false), 50, "RTIP.AcceptRTIPMessage", 289, "Couldn't relay message (%x[%d]) from RTIP over wifi", 2, tag, size);
              }
              else 
              {
                clientSendMessage(relayBuffer + 1, size, RobotInterface::GLOBAL_INVALID_TAG, false, false);
              }
            }
          }
        }
        relayQueued -= sizeWHeader;
        os_memmove(relayBuffer, relayBuffer + sizeWHeader, relayQueued);
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
