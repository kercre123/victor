/** Impelementation for RTIP interface on Espressif
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "driver/i2spi.h"
#include "client.h"
}
#include "anki/cozmo/robot/logging.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

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
    AnkiConditionalErrorAndReturnValue(bufferSize <= DROP_TO_RTIP_MAX_VAR_PAYLOAD,     false, 30, "RTIP.SendMessage.TooBig", 198, "SendMessage: Message too large for RTIP, %d > %d", 2, bufferSize, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
    if (i2spiQueueMessage(buffer, bufferSize)) return true;
    else
    {
      AnkiDebug( 216, "RTIP.SendMessage.Failed", 199, "SendMessage: Couldn't forward message %x[%d] to RTIP", 2, buffer[0], bufferSize);
      return false;
    }
  }
  else
  {
    u8 msgBuffer[DROP_TO_RTIP_MAX_VAR_PAYLOAD];
    AnkiConditionalErrorAndReturnValue(bufferSize + 1 <= DROP_TO_RTIP_MAX_VAR_PAYLOAD, false, 30, "RTIP.SendMessage.TooBig", 415, "SendMessage with %x[%d] > %d", 3, tag, bufferSize, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
    msgBuffer[0] = tag;
    os_memcpy(msgBuffer + 1, buffer, bufferSize);
    if (i2spiQueueMessage(msgBuffer, bufferSize+1)) return true;
    else
    {
      AnkiDebug( 216, "RTIP.SendMessage.Failed", 199, "SendMessage: Couldn't forward message %x[%d] to RTIP", 2, tag, bufferSize+1);
      return false;
    }
  }
}

bool SendMessage(RobotInterface::EngineToRobot& msg)
{
  return SendMessage(msg.GetBuffer(), msg.Size());
}

void Update()
{
  RobotInterface::EngineToRobot msg;
  i2spiUpdateRtipQueueEstimate();
  u8* buffer = msg.GetBuffer();
  int size = i2spiGetCladMessage(buffer);
  while (size > 0)
  {
    if (msg.tag < RobotInterface::TO_WIFI_START)
    {
      os_printf("Wifi rcvd msg bound below %d\r\n", msg.tag);
      AnkiError( 50, "RTIP.AcceptRTIPMessage", 376, "WiFi received message from RTIP, %x[%d] that seems bound below (< 0x%x)", 3, msg.tag, size, (int)RobotInterface::TO_WIFI_START);
      {  // This most likely means we are completely out of sync on the I2SPI bus, and it is probably impossible to recover.
        RobotInterface::RobotErrorReport rer;
        rer.error = RobotInterface::REC_I2SPI_Sync;
        rer.fatal = true;
        Anki::Cozmo::RobotInterface::SendMessage(rer);
        i2spiLogDesync(buffer, msg.Size());
      }
    }
    else if (msg.tag <= RobotInterface::TO_WIFI_END) // This message is for us
    {
      Messages::ProcessMessage(msg);
    }
    else if (clientConnected())
    {
      if ((msg.tag == RobotInterface::RobotToEngine::Tag_trace) && (clientQueueAvailable() < LOW_PRIORITY_BUFFER_ROOM))
      {
        AnkiWarn( 50, "RTIP.AcceptRTIPMessage", 442, "dropping RTIP trace", 0);
      }
      else
      {
        const bool reliable = msg.tag < RobotInterface::TO_ENG_UNREL;
        if (reliable)
        {
          AnkiConditionalError(clientSendMessage(buffer, size, RobotInterface::GLOBAL_INVALID_TAG, true, false), 50, "RTIP.AcceptRTIPMessage", 289, "Couldn't relay message (%x[%d]) from RTIP over wifi", 2, msg.tag, msg.Size());
        }
        else 
        {
          clientSendMessage(buffer, size, RobotInterface::GLOBAL_INVALID_TAG, false, false);
        }
      }
    }
    size = i2spiGetCladMessage(buffer);
  }
}

} // RTIP
} // Cozmo
} // Anki
