#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void FlashBlockIDs()
      {
          // XXX STUB!
      }
      
      Result SetBlockLight(const u8 blockID, const u32* onColor, const u32* offColor,
                           const u32* onPeriod_ms, const u32* offPeriod_ms,
                           const u32* transitionOnPeriod_ms, const u32* transitionOffPeriod_ms)
      {
        return RESULT_OK;
      }
    }
  }
}
