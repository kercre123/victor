#include "battery.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace {
        float vBat;
        float vExt;
        u8 chargeStat;
      }
      
      void Battery::HandlePowerStateUpdate(PowerState& msg)
      {
        vBat = static_cast<float>(msg.VBatFixed)/65536.0f;
        vExt = static_cast<float>(msg.VExtFixed)/65536.0f;
        chargeStat = msg.chargeStat;
      }
      
      u8 BatteryGetVoltage10x()
      {
        return static_cast<u8>(vBat * 10.0f); // XXX Returning battery voltage * 10 for now
      }

      bool BatteryIsCharging()
      {
        return false; // XXX On Cozmo 3, head is off if robot is charging
      }

      bool BatteryIsOnCharger()
      {
        // Let's say we consider it to be on charger if 4.0V is detected. 
        return vExt > 4.0f; 
      }
    }
  }
}
