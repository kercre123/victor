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

      extern volatile GlobalDataToBody g_dataToBody;
      char const backpackLightLUT[5] = {3, 2, 1, 0, 0};

      GPIO_PIN_SOURCE(IRLED, GPIOE, 0);

      // Initialize LED head/face light hardware
      void LightsInit()
      {
        int i;

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
          *channel = (*channel & 0x000000ff) | (color > 0 ? 0xff00 : 0);
        } else if (led_id == LED_BACKPACK_RIGHT) {
          *channel = (*channel & 0x0000ff00) | (color > 0 ? 0xff : 0);
        } else {
          *channel = color;
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
    }
  }
}
