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
        // If you get stuck here, you are trying to run a 3.0 build on non-3.0 hardware!
        while (0x431 != (DBGMCU->IDCODE & DBGMCU_IDCODE_DEV_ID))  // STM32F411
        {
        }          
      }
    }
  }
}
