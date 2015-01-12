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
        // No thing to do here on the Cozmo 3 
        return;
      }
    }
  }
}
