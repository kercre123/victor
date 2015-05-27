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
      char const backpackLightLUT[5] = {0, 1, 2, 2, 3};

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
        g_dataToBody.backpackColors[ backpackLightLUT[led_id] ] = color;    
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
