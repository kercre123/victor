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
								while(1)
				{
				SetLED(LED_RIGHT_EYE_RIGHT, 0x000001FF);
				MicroWait(500000);
				SetLED(LED_RIGHT_EYE_RIGHT, 0x00000000);
				MicroWait(500000);
				}
      }
      
      Result SetBlockLight(const u8 blockID, const u32* color) {
				for(int Id = 0; Id<NUM_LEDS; Id++)
				{
						SetLED(LEDId(Id), color[Id]);
				}
				/*
				while(1)
				{
				SetLED(LED_RIGHT_EYE_RIGHT, 0x000001FF);
				MicroWait(500000);
				SetLED(LED_RIGHT_EYE_RIGHT, 0x00000000);
				MicroWait(500000);
				}*/
        return RESULT_OK;
      }
    }
  }
}