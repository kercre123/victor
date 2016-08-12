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
      // Thresholds for factory FW
      // NOTE: Need more testing to figure out what these should be
      static const uint32_t DROP_LEVEL = 400;
      static const uint32_t UNDROP_LEVEL = 600;  // hysteresis
      
      // Previous thresholds
      //static const uint32_t DROP_LEVEL = 20;
      //static const uint32_t UNDROP_LEVEL = 120;  // hysteresis
      
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
        if (!cliffDetected && g_dataToHead.cliffLevel < DROP_LEVEL) {
          cliffDetected = true;
        } else if (cliffDetected && g_dataToHead.cliffLevel > UNDROP_LEVEL) {
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
