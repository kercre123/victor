#include "board.h"
#include "timer.h"

//select peripheral
#if defined(SYSCON) && defined(STM32F030xC)
  #include "stm32f0xx.h"
  #define USE_TIM6      0
  #define USE_SYSTICK   1
#elif defined(FIXTURE) && defined(STM32F2XX)
  #define USE_TIM6      1
  #define USE_SYSTICK   0
#else
  #error No compatible target for Timer selection
#endif

#if USE_SYSTICK > 0
static const uint32_t FsysMHz = SYSTEM_CLOCK / 1000000;
static const uint32_t SysTickReloadVal = 0xFFFF * FsysMHz;
#endif

void Timer::init(void)
{
  #if USE_TIM6 > 0
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // Initialize the 1 microsecond timer
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
    TIM_TimeBaseStructure.TIM_Prescaler = 59;  // should be 29 for 30 MHz (assuming peripheral clock 1)... WTF
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 0xffff;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM6, ENABLE);

  #elif USE_SYSTICK > 0
    SysTick->LOAD = SysTickReloadVal; //set reload such that 1 counter cycle is exactly 65,536us (16-bit us counter)
    SysTick->VAL  = SysTickReloadVal;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  //1 = processor clock
                    //SysTick_CTRL_TICKINT_Msk |    //1 = counting down to zero asserts the SysTick exception request
                    SysTick_CTRL_ENABLE_Msk;      //1 = counter enabled
  
  #endif
}

uint32_t Timer::get(void)
{
  // The code below turns a 16-bit into a 32-bit timer (with continuous polling)
  volatile static uint16_t high = 0;	// Supply the missing high bits of TIM6
  volatile static uint16_t last = 0;	// Last read of TIM6
  
  // NOTE:  This must be interrupt-safe for encoder code, so take care below
  __disable_irq();
  #if USE_TIM6
    uint32_t now = TIM6->CNT;
  #elif USE_SYSTICK
    //reload value (e.g. max count) is calculated such that resulting range is 0..65k (16-bit)
    uint32_t count = SysTickReloadVal - SysTick->VAL; //make count-down into count-up
    uint32_t now = count / FsysMHz; //#us = count*Tsys[us] = count/Fsys[MHz]
  #endif
  
  if (now < last)	// Each time we wrap TIM6, increase the high part by 1
    high++;
  
  last = now;
  now |= (high << 16);
  __enable_irq();
  
  return now;
}

void Timer::wait(uint32_t us)
{
  uint32_t start = Timer::get();
  us++; // Wait 1 tick longer, in case we are midway through start tick

  // Note: The below handles wrapping with unsigned subtraction
  while (Timer::get()-start < us) {}
}

void Timer::delayMs(uint32_t ms)
{
  for( ; ms>0; ms--)
		Timer::wait(1000);
}

uint32_t Timer::elapsedUs(uint32_t base)
{
  uint32_t tick_diff = Timer::get() - base;
  return tick_diff; //1us period, full counter range
}

#if USE_SYSTICK > 0
extern "C" void SysTick_Handler(void)
{
}
#endif
