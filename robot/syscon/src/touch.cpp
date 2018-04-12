#include <string.h>

#include "common.h"
#include "hardware.h"

#include "touch.h"

static uint16_t touch;

void Touch::init(void) {
  TIM16->PSC = 0;
  TIM16->ARR = 0xFFFF;

  TIM16->CCMR1 = 0
    | (TIM_CCMR1_IC1F_0 * 4)    // Four sample debounce
    | (TIM_CCMR1_IC1PSC_0 * 0)  // No-prescalar
    | (TIM_CCMR1_CC1S_0 * 1)    // TI1 input mode
    ;

  TIM16->CCER = 0
    | (TIM_CCER_CC1P * 0)       // Rising edge
    | TIM_CCER_CC1E             // Enable Capture 1
    ;

  TIM16->DIER = 0
    | TIM_DIER_UIE
    ;

  TIM16->CR2 = 0
    ;

  NVIC_EnableIRQ(TIM16_IRQn);
  NVIC_SetPriority(TIM16_IRQn, PRIORITY_TOUCH_SENSE);
}

extern "C" void TIM16_IRQHandler(void) {
  touch = (TIM16->SR & TIM_SR_CC1IF) ? TIM16->CCR1 : 0xFFFF;

  CAPO::reset();
  TIM16->SR = 0;
}

void Touch::transmit(uint16_t* data) {
  *data = touch;
}

void Touch::tick(void) {
  // Start Counting
  __disable_irq();
  TIM16->CR1 = 0
    | TIM_CR1_OPM               // One pulse mode
    | TIM_CR1_CEN               // Enabled
    ;
  CAPO::set();
  __enable_irq();
}
