#include "common.h"
#include "hardware.h"

#include "timer.h"
#include "vectors.h"
#include "flash.h"
#include "touch.h"

extern void Main_Execution(void);

uint32_t Timer::clock;
void_funct Timer::LightHandler;

void dummy(void) {}

void Timer::init(void) {
  // Set SysTick to free run
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk;

  TIM14->CR1 = 0;
  for (int i = 0; i < 4; i++) __asm("nop");
  TIM14->PSC = MAIN_EXEC_PRESCALE - 1;
  TIM14->ARR = MAIN_EXEC_OVERFLOW - 1;
  TIM14->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE;
  TIM14->CR1 = TIM_CR1_CEN;
  TIM14->CCR1 = 15000;

  Timer::LightHandler = dummy;

  NVIC_SetPriority(TIM14_IRQn, PRIORITY_MAIN_EXEC);  // Extremely low priority
  NVIC_EnableIRQ(TIM14_IRQn);
}

extern "C" void TIM14_IRQHandler(void) {
  if (TIM14->SR & TIM_SR_UIF) {
    Timer::clock++;
    Timer::getTime();

    Main_Execution();
  } else {
    // Capture mode
    Touch::Pause();
    Timer::LightHandler();
    Touch::Resume();
  }

  TIM14->SR = 0;
}
