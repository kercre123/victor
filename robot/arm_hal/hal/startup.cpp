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
        // Set up 4 bits for priority, 0 bits for subpriority
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); 
        
        GPIO_PIN_SOURCE(BOARDID, GPIOC, 7);
        
        // Check that we are on the correct board revision
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        PIN_PULLUP(GPIO_BOARDID, SOURCE_BOARDID);
        PIN_IN(GPIO_BOARDID, SOURCE_BOARDID);
        MicroWait(5);   // Wait for pin settling time
        
        // If you get stuck here, you are trying to run a 2.1 build on non-2.1 hardware!
        while (0 != (GPIO_READ(GPIO_BOARDID) & PIN_BOARDID))
        {
        }          
      }
    }
  }
}
