#include <stdint.h>
#include "MK02F12810.h"

#define CORE_CLOCK (100000000)

// 0.32 reciprocal conversion from CORE_CLOCK to microseconds
// Multiply this by SYSTICK to yield microseconds << 32
const uint32_t SYSTICK_RECIP = (1000000LL << 32LL) / CORE_CLOCK;
const uint32_t MAX_COUNT = (((65536LL << 32LL) / SYSTICK_RECIP)) - 1;

uint32_t GetMicroCounter(void);

// Initialize system timers, excluding watchdog
// This must run first in main()
void TimerInit(void)
{
  SysTick->LOAD = MAX_COUNT;
  SysTick->VAL = MAX_COUNT;
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
}

// Wait the specified number of microseconds
void MicroWait(uint32_t us)
{
  uint32_t start = GetMicroCounter();
  us++;   // Wait 1 tick longer, in case we are midway through start tick

  // Note: The below handles wrapping with unsigned subtraction
  while (GetMicroCounter()-start < us)
    ;
}

// Get the number of microseconds since boot
uint32_t GetMicroCounter(void)
{
  // The code below turns the 24-bit core clock into a 32-bit microsecond timer
  static uint16_t high = 0;	// Supply the missing high bits
  static uint16_t last = 0;	// Last read of SysTick/US_DIVISOR
  
  // NOTE:  This must be interrupt-safe, so take care below
  __disable_irq();
  uint32_t now = ((uint64_t)(MAX_COUNT - SysTick->VAL) * SYSTICK_RECIP) >> 32;
  
  if (now < last)				// Each time we wrap the low part, increase the high part by 1
    high++;	
  
  last = now;
  now |= (high << 16);
  __enable_irq();
  
  return now;
}
