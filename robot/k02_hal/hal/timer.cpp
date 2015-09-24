#include "board.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "core_cm4.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // 0.32 reciprocal conversion from CORE_CLOCK to microseconds
      // Multiply this by SYSTICK to yield microseconds << 32
      const u32 SYSTICK_RECIP = (1000000LL << 32LL) / CORE_CLOCK;
      const u32 MAX_COUNT = (((65536LL << 32LL) / SYSTICK_RECIP)) - 1;

      // Initialize system timers, excluding watchdog
      // This must run first in main()
      void TimerInit(void)
      {
        SysTick->LOAD = MAX_COUNT;
        SysTick->VAL = MAX_COUNT;
        SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
      }

      // Wait the specified number of microseconds
      void MicroWait(u32 us)
      {
        u32 start = GetMicroCounter();
        us++;   // Wait 1 tick longer, in case we are midway through start tick

        // Note: The below handles wrapping with unsigned subtraction
        while (GetMicroCounter()-start < us)
          ;
      }

      // Get the number of microseconds since boot
      u32 GetMicroCounter(void)
      {
        // The code below turns the 24-bit core clock into a 32-bit microsecond timer
        static u16 high = 0;	// Supply the missing high bits
        static u16 last = 0;	// Last read of SysTick/US_DIVISOR
        
        // NOTE:  This must be interrupt-safe, so take care below
        __disable_irq();
        u32 now = ((u64)(MAX_COUNT - SysTick->VAL) * SYSTICK_RECIP) >> 32;
        
        if (now < last)				// Each time we wrap the low part, increase the high part by 1
          high++;	
        
        last = now;
        now |= (high << 16);
        __enable_irq();
        
        return now;
      }
    }
  }
}
