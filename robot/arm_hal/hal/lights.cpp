#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static const uint32_t DROP_LEVEL = 20;

      extern volatile GlobalDataToBody g_dataToBody;
      char const backpackLightLUT[5] = {3, 2, 1, 0, 0};

      GPIO_PIN_SOURCE(IRLED, GPIOE, 0);

      // Initialize LED head/face light hardware
      void LightsInit()
      {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

        // IR LED is controlled by N-FET so positive polarity unlike everything else
        GPIO_RESET(GPIO_IRLED, PIN_IRLED);
        PIN_PULLDOWN(GPIO_IRLED, SOURCE_IRLED);
        PIN_PP(GPIO_IRLED, SOURCE_IRLED);
        PIN_OUT(GPIO_IRLED, SOURCE_IRLED);

      }

      

      // Light up one of the backpack LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u32 color)
      {
        volatile u32* channel = &g_dataToBody.backpackColors[ backpackLightLUT[led_id] ];
        if (led_id == LED_BACKPACK_LEFT) {
          // Use red color channel for intensity
          *channel = (*channel & 0xffff00ff) | (color>>8);
        } else if (led_id == LED_BACKPACK_RIGHT) {
          // Use red color channel for intensity
          *channel = (*channel & 0xff00ffff) | (color);
        } else {
          // RGB -> BGR
          *channel = ((color & 0xff) << 16) |  (color & 0xff00) | ((color & 0xff0000) >> 16) ;
        }
      }

      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state)
      {
        if (state)
          GPIO_SET(GPIO_IRLED, PIN_IRLED);
        else
          GPIO_RESET(GPIO_IRLED, PIN_IRLED);
      }
      
      extern volatile GlobalDataToHead g_dataToHead;      
      bool IsCliffDetected()
      {
        return g_dataToHead.cliffLevel < DROP_LEVEL;
      }
    }
  }
}
