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
}
#include "anki/cozmo/robot/logging.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"

namespace Anki {
namespace Cozmo {
namespace RTIP {

bool SendMessage(RobotInterface::EngineToRobot& msg)
{
  AnkiConditionalErrorAndReturnValue(msg.Size() <= DROP_TO_RTIP_MAX_VAR_PAYLOAD,     false, "RTIP", "SendMessage: Message too large for RTIP, %d > %d", msg.Size(), DROP_TO_RTIP_MAX_VAR_PAYLOAD);
  AnkiConditionalErrorAndReturnValue(i2spiQueueMessage(msg.GetBuffer(), msg.Size()), false, "RTIP", "SendMessage: Couldn't forward message to RTIP");
  return true;
}

} // RTIP
} // Cozmo
} // Anki
