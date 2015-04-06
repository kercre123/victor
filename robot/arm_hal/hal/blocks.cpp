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
      
      Result SetBlockLight(const u8 blockID, const u32* color) {
        return RESULT_OK;
      }
    }
  }
}
