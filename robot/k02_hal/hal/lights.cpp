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

      // Light up one of the backpack LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u16 color)
      {
        u8 led_channel = led_id & 0x3; // so that LED_BACKPACK_RIGHT maps to 0
        volatile u32* channel = &g_dataToBody.backpackColors[ led_channel ];
        if (led_id == LED_BACKPACK_LEFT) {
          // Use red color channel for intensity
          *channel = (*channel & 0xffff00ff) | (((color & LED_ENC_RED) >> LED_ENC_RED_SHIFT) << ( 8+3));
        } else if (led_id == LED_BACKPACK_RIGHT) {
          // Use red color channel for intensity
          *channel = (*channel & 0xff00ffff) | (((color & LED_ENC_RED) >> LED_ENC_RED_SHIFT) << (16+3));
        } else {
          // RGB encoded -> BGR expanded
          *channel = (((color & LED_ENC_BLU) >> LED_ENC_BLU_SHIFT) << (16+3)) | 
                     (((color & LED_ENC_GRN) >> LED_ENC_GRN_SHIFT) << ( 8+3)) |
                     (((color & LED_ENC_RED) >> LED_ENC_RED_SHIFT) << ( 0+3));
        }
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
