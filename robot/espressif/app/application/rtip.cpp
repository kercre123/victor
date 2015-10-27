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

namespace Anki {
namespace Cozmo {
namespace RTIP {

bool SendMessage(RobotInterface::EngineToRobot& msg)
{
  return i2spiQueueMessage(msg.GetBuffer(), msg.Size());
}

} // RTIP
} // Cozmo
} // Anki
