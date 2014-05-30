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
      GPIO_PIN_SOURCE(RED, GPIOH, 11);
      GPIO_PIN_SOURCE(GREEN, GPIOH, 12);
      GPIO_PIN_SOURCE(BLUE, GPIOH, 8);
      GPIO_PIN_SOURCE(EYECLK, GPIOA, 7);
      GPIO_PIN_SOURCE(EYERST, GPIOB, 1);        

      // Initialize LED head/face light hardware
      void LightsInit()
      {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);

        // Leave the high side pins off until first LED is set
        GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
        PIN_OUT(GPIO_EYECLK, SOURCE_EYECLK);        
        GPIO_RESET(GPIO_EYERST, PIN_EYERST);
        PIN_OUT(GPIO_EYERST, SOURCE_EYERST);

        // Initialize all LED colors to OFF
        // Low side drivers must be open drain to allow voltages > VDD
        GPIO_SET(GPIO_RED, PIN_RED);        
        GPIO_SET(GPIO_GREEN, PIN_GREEN);        
        GPIO_SET(GPIO_BLUE, PIN_BLUE);
        PIN_NOPULL(GPIO_RED, SOURCE_RED);
        PIN_NOPULL(GPIO_GREEN, SOURCE_GREEN);        
        PIN_NOPULL(GPIO_BLUE, SOURCE_BLUE);
        PIN_OD(GPIO_RED, SOURCE_RED);
        PIN_OD(GPIO_GREEN, SOURCE_GREEN);        
        PIN_OD(GPIO_BLUE, SOURCE_BLUE);
        PIN_OUT(GPIO_RED, SOURCE_RED);
        PIN_OUT(GPIO_GREEN, SOURCE_GREEN);        
        PIN_OUT(GPIO_BLUE, SOURCE_BLUE);        
      }
      
      // Light up one of the eye LEDs as white as possible
      // number is 0 thru 7 to select which LED to light
      void SetLED(LEDId led_id, LEDColor color)
      {
        if (led_id >= NUM_LEDS) {
          return;
        }
        
        // Turn off all LEDs
        GPIO_SET(GPIO_RED, PIN_RED);        
        GPIO_SET(GPIO_GREEN, PIN_GREEN);        
        GPIO_SET(GPIO_BLUE, PIN_BLUE);

        // Reset LED number to first LED
        GPIO_SET(GPIO_EYERST, PIN_EYERST);
        MicroWait(1);
        GPIO_RESET(GPIO_EYERST, PIN_EYERST);
        
        // Count up to specified LED number
        for (int eye = 0; eye < led_id; eye++) {
          GPIO_SET(GPIO_EYECLK, PIN_EYECLK);
          MicroWait(1);
          GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
          MicroWait(1);
        }
                        
        // Turn on the specified LED color
        if (color & LED_RED)
          GPIO_RESET(GPIO_RED, PIN_RED);
        if (color & LED_GREEN)
          GPIO_RESET(GPIO_GREEN, PIN_GREEN);
        if (color & LED_BLUE)
          GPIO_RESET(GPIO_BLUE, PIN_BLUE);
      }
    }
  }
}
