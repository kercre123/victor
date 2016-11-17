#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "uart.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static bool cliffDetected = false;
      static uint16_t colorState[4];

      // Light up one of the backpack LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u16 color)
      {
      }
      
      void GetBackpack(uint16_t* colors) {
        memcpy(colors, colorState, sizeof(colorState));
      }

      bool IsCliffDetected()
      {
        if (!cliffDetected && g_dataToHead.cliffLevel < CLIFF_SENSOR_DROP_LEVEL) {
          cliffDetected = true;
        } else if (cliffDetected && g_dataToHead.cliffLevel > CLIFF_SENSOR_UNDROP_LEVEL) {
          cliffDetected = false;
        }
        return cliffDetected;
      }
      
      u16 GetRawCliffData() 
      {
        return static_cast<u16>(g_dataToHead.cliffLevel);
      }
    }
  }
}
