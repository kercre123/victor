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
      static const uint32_t DROP_LEVEL = 20;
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
        switch(HAL::GetID()) {
          // Cliff sensors not working on these robots
          case 0x3AA7:
          case 0x3A94:
            return false;
          default:
            return g_dataToHead.cliffLevel < DROP_LEVEL;
        }
      }
    }
  }
}
