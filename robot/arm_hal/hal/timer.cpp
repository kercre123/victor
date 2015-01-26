#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"

#define ABP2_CLOCK_MHZ (90)

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Initialize system timers, excluding watchdog
      // This must run first in main()
      void TimerInit(void)
      {
        TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

        // Initialize the 1 microsecond timer
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM10, ENABLE);
        TIM_TimeBaseStructure.TIM_Prescaler = (ABP2_CLOCK_MHZ) - 1;  // APB1 clock is not system clock
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_Period = 0xffff;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
        TIM_TimeBaseInit(TIM10, &TIM_TimeBaseStructure);
        TIM_Cmd(TIM10, ENABLE);
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
        // The code below turns the 16-bit TIM10 into a 32-bit timer
        volatile static u16 high = 0;	// Supply the missing high bits of TIM10
        volatile static u16 last = 0;	// Last read of TIM10
        
        // NOTE:  This must be interrupt-safe for encoder code, so take care below
        __disable_irq();
        u32 now = TIM10->CNT;
        
        if (now < last)				// Each time we wrap TIM10, increase the high part by 1
          high++;	
        
        last = now;
        now |= (high << 16);
        __enable_irq();
        
        return now;
      }
    }
  }
}

