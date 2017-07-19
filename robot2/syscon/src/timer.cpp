#include "common.h"
#include "hardware.h"

#include "timer.h"
#include "vectors.h"
#include "flash.h"

static const uint16_t MAIN_EXEC_PRESCALE = SYSTEM_CLOCK / MAIN_EXEC_TICK; // Timer prescale
static const uint16_t MAIN_EXEC_PERIOD = MAIN_EXEC_TICK / 200;        // 200hz (5ms period)

extern void Main_Execution(void);

uint32_t Timer::clock;

void Timer::init(void) {
  // Set SysTick to free run
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk;

  TIM14->PSC = MAIN_EXEC_PRESCALE - 1;
  TIM14->ARR = MAIN_EXEC_PERIOD - 1;
  TIM14->DIER = TIM_DIER_UIE;
  TIM14->CR1 = TIM_CR1_CEN;

  NVIC_SetPriority(TIM14_IRQn, 3);  // Extremely low priority
  NVIC_EnableIRQ(TIM14_IRQn);
}

extern "C" void WWDG_IRQHandler(void) {
  Flash::writeFaultReason(FAULT_WATCHDOG);
}

extern "C" void TIM14_IRQHandler(void) {
  // Kick watch dog when we enter our service routine
  IWDG->KR = 0xAAAA;
  WWDG->CR = 0xFF;  // Enabled with 64 ticks

  TIM14->SR = 0;
  Timer::clock++;
  Timer::getTime();

  Main_Execution();
}
