// Check hardware for board 2.1
#include "lib/stm32f4xx.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/hal.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void Startup()
      {
        // If you get stuck here, you are trying to run a 4.0 build on 3.0 hardware!
        while (0x431 == (DBGMCU->IDCODE & DBGMCU_IDCODE_DEV_ID))  // STM32F411
        {
        }       
        
        // Initialize clocks for each power rail
        GPIO_PIN_SOURCE(V33, GPIOF, 7);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
        
        GPIO_PIN_SOURCE(V12, GPIOD, 4);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        
        GPIO_PIN_SOURCE(V28, GPIOC, 3);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        
        GPIO_PIN_SOURCE(V18b, GPIOD, 12);
        
        GPIO_PIN_SOURCE(V18a, GPIOG, 3);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
        
        GPIO_PIN_SOURCE(CAM_RESET, GPIOA, 8);  // Hacked onto TP1 in 4.0
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        GPIO_PIN_SOURCE(CAM_PWDN, GPIOE, 2);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
        
        // Now power on, in order
        
        // 1.2 and 3.3 together
        GPIO_SET(GPIO_V12, PIN_V12);    // Active high
        PIN_OUT(GPIO_V12, SOURCE_V12);
        GPIO_SET(GPIO_V33, PIN_V33);    // Active high
        PIN_OUT(GPIO_V33, SOURCE_V33);
        
        MicroWait(50);    // Let 1.2 + 3.3 stabilize
        
        // Put camera in powerdown/reset
        GPIO_RESET(GPIO_CAM_RESET, PIN_CAM_RESET);  // Reset low (on)
        PIN_OUT(GPIO_CAM_RESET, SOURCE_CAM_RESET);
        GPIO_SET(GPIO_CAM_PWDN, PIN_CAM_PWDN);      // Powerdown high (on)
        PIN_OUT(GPIO_CAM_PWDN, SOURCE_CAM_PWDN);
        
        // 1.8a
        GPIO_RESET(GPIO_V18a, PIN_V18a);// Active low
        PIN_OUT(GPIO_V18a, SOURCE_V18a);
        
        MicroWait(50);    // Must wait 50uS minimum
        
        // 1.8b and 2.8 together
        GPIO_RESET(GPIO_V18b, PIN_V18b);// Active low
        PIN_OUT(GPIO_V18b, SOURCE_V18b);
        GPIO_SET(GPIO_V28, PIN_V28);    // Active high
        PIN_OUT(GPIO_V28, SOURCE_V28);                
      }
    }
  }
}
