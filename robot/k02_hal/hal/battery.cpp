#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "uart.h"
#include <limits.h>

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      u8 BatteryGetVoltage10x()
      {
        return (g_dataToHead.VBat * 10)/65535; // XXX Returning battery voltage * 10 for now
      }

      bool BatteryIsCharging()
      {
        return false; // XXX On Cozmo 3, head is off if robot is charging
      }

      bool BatteryIsOnCharger()
      {
        // Let's say we consider it to be on charger if 4.0V is detected. 
        return (g_dataToHead.VExt / 65536.0f) > 4.0f; 
      }
    }
  }
}
