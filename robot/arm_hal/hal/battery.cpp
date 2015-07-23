#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include <limits.h>

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      extern volatile GlobalDataToHead g_dataToHead;

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
        return false; // XXX On Cozmo 3, head is off if robot is charging
      }
    }
  }
}
