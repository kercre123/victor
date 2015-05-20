#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
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

      // Light up one of the eye LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u32 color)
      {
        // KEVIN: Disabled for now since there are no eye LEDs anymore
        //if (led_id < NUM_LEDS)  // Unsigned, so always >= 0
        //  m_channels[HW_CHANNELS[led_id]].asColor = color;
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
