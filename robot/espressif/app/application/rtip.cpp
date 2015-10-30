/** Impelementation for RTIP interface on Espressif
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "driver/i2spi.h"
}
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"

namespace Anki {
namespace Cozmo {
namespace RTIP {

bool SendMessage(RobotInterface::EngineToRobot& msg)
{
  if (i2spiQueueMessage(msg.GetBuffer(), msg.Size()))
  {
    return true;
  }
  else
  {
    PRINT("Couldn't forward message to RTIP\r\n");
    return false;
  }
}

} // RTIP
} // Cozmo
} // Anki
