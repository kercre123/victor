#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
//#include "anki/common/robot/config.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
			
      // Light up one of the eye LEDs as white as possible
      // number is 0 thru 7 to select which LED to light
      // color is 1 (GREEN) | 2 (ORANGE) | 4 (BLUE) - or some combination
      void SetLED(LEDId led_id, LEDColor color)
      {
        if (led_id >= NUM_LEDS) {
          return;
        }
				
        //const int EYENUM = 0;
        GPIO_PIN_SOURCE(RED, GPIOA, 2);
        GPIO_PIN_SOURCE(GREEN, GPIOA, 1);
        GPIO_PIN_SOURCE(BLUE, GPIOB, 1);
        GPIO_PIN_SOURCE(EYECLK, GPIOA, 11);
        GPIO_PIN_SOURCE(EYERST, GPIOA, 12);
        
        if (color & LED_RED)
          GPIO_RESET(GPIO_RED, PIN_RED);
        else
          GPIO_SET(GPIO_RED, PIN_RED);        
        PIN_OUT(GPIO_RED, SOURCE_RED);
        if (color & LED_GREEN)
          GPIO_RESET(GPIO_GREEN, PIN_GREEN);
        else
          GPIO_SET(GPIO_GREEN, PIN_GREEN);        
        PIN_OUT(GPIO_GREEN, SOURCE_GREEN);        
        if (color & LED_BLUE)
          GPIO_RESET(GPIO_BLUE, PIN_BLUE);
        else
          GPIO_SET(GPIO_BLUE, PIN_BLUE);
        PIN_OUT(GPIO_BLUE, SOURCE_BLUE);

        GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
        PIN_OUT(GPIO_EYECLK, SOURCE_EYECLK);        
        GPIO_SET(GPIO_EYERST, PIN_EYERST);
        PIN_OUT(GPIO_EYERST, SOURCE_EYERST);
        MicroWait(1);
        GPIO_RESET(GPIO_EYERST, PIN_EYERST);
        
        for (int eye = 0; eye < led_id; eye++) {
          GPIO_SET(GPIO_EYECLK, PIN_EYECLK);
          MicroWait(1);
          GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
          MicroWait(1);
        }
      }
    }
  }
}
