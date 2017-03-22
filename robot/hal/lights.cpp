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
      static uint16_t colorState[4];

      // Light up one of the backpack LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u16 color)
      {
      }
      
      void GetBackpack(uint16_t* colors) {
        memcpy(colors, colorState, sizeof(colorState));
      }
      
      u16 GetRawCliffData() 
      {
        return g_dataToHead.cliffLevel;
      }

      u16 GetCliffOffLevel()
      {
        return g_dataToHead.cliffOffLevel;
      }
    }
  }
}
