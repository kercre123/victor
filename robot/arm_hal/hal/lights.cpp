#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

// Uncomment this if you want the eye LED to flash constantly to show head/body sync
#define BLINK_ON_SYNC

// Uncomment this if your 2.1 robot has the headlight fix mod (it should by now)
#define FIXED_HEADLIGHTS

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
      
      static LEDId m_led;
      static LEDColor m_color;
      
      // Map the natural LED order (as shown in hal.h) to the hardware swizzled order
      // See schematic if you care about why the LEDs are swizzled
      static u8 HW_CHANNELS[8] = {5, 1, 2, 0, 6, 7, 3, 4};
      
      // Initialize LED head/face light hardware
      void LightsInit()
      {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);

        // Leave the high side pins off until first LED is set
        GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
        PIN_OD(GPIO_EYECLK, SOURCE_EYECLK);   // For 2.1 pullup mod
        PIN_OUT(GPIO_EYECLK, SOURCE_EYECLK);        
        GPIO_RESET(GPIO_EYERST, PIN_EYERST);
        PIN_OD(GPIO_EYERST, SOURCE_EYERST);
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
      
      // Private helper to actually set the hardware color
      static void SetColor(LEDId led_id, LEDColor color)
      {              
        static u8 s_lastLED = 0xff;
        
        // Turn off all LEDs
        GPIO_SET(GPIO_RED, PIN_RED);        
        GPIO_SET(GPIO_GREEN, PIN_GREEN);        
        GPIO_SET(GPIO_BLUE, PIN_BLUE);

#ifdef FIXED_HEADLIGHTS
        // If the requested LED does not match the current LED, select a new LED
        if (led_id != s_lastLED)
        {
          // Reset LED number to first LED
          GPIO_SET(GPIO_EYERST, PIN_EYERST);
          MicroWait(1);
          GPIO_RESET(GPIO_EYERST, PIN_EYERST);
          
          // Count up to specified LED number
          u8 hwChannel = HW_CHANNELS[led_id];
          for (int eye = 0; eye < hwChannel; eye++) {
            GPIO_SET(GPIO_EYECLK, PIN_EYECLK);
            MicroWait(1);
            GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
            MicroWait(1);
          }
          
          s_lastLED = led_id;
        }
#endif        
                        
        // Turn on the specified LED color
        if (color & LED_RED)
          GPIO_RESET(GPIO_RED, PIN_RED);
        if (color & LED_GREEN)
          GPIO_RESET(GPIO_GREEN, PIN_GREEN);
        if (color & LED_BLUE)
          GPIO_RESET(GPIO_BLUE, PIN_BLUE);
      }
      
      static u8 m_blanked = 0;
      
      // Light up one of the eye LEDs as white as possible
      // number is 0 thru 7 to select which LED to light
      void SetLED(LEDId led_id, LEDColor color)
      {
        if (led_id >= NUM_LEDS) {
          return;
        }
        
        m_led = led_id;
        m_color = color;
        
        if (!m_blanked)
          SetColor(m_led, m_color);
      }
      
      // If enabled, this allows head sync to blink the LED
      void BlinkOnSync(bool blank)
      {
#ifdef BLINK_ON_SYNC
        if (m_blanked == blank)
          return;
        
        if (blank)
          SetColor(m_led, LED_OFF);
        else
          SetColor(m_led, m_color);
        
        m_blanked = blank;
#endif        
      }
    }
  }
}
