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
        static const uint16_t LEFT_MASK  = PACK_COLORS(0xFF, 0, 0xFF, 0xFF);
        static const uint16_t RIGHT_MASK = PACK_COLORS(0xFF, 0xFF, 0, 0xFF);

        switch (led_id) {
          case LED_BACKPACK_FRONT:
          case LED_BACKPACK_MIDDLE:
          case LED_BACKPACK_BACK:
            colorState[led_id] = color;
            break ;
          case LED_BACKPACK_LEFT:
            colorState[0] = (colorState[0] & LEFT_MASK) | PACK_COLORS(0, UNPACK_RED(color), 0, 0);
            break ;
          case LED_BACKPACK_RIGHT:
            colorState[0] = (colorState[0] & RIGHT_MASK) | PACK_COLORS(0, 0, UNPACK_RED(color), 0);
            break ;
        }
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
