#include <string.h>

#include "common.h"
#include "hardware.h"

#include "touch.h"

static uint16_t touch[2];

void Touch::init(void) {
  TIM16->PSC = TIM17->PSC = 0;
  TIM16->ARR = TIM17->ARR = 0xFFFF;

  TIM16->CCMR1 = TIM17->CCMR1 = 0
    | (TIM_CCMR1_IC1F_0 * 4)    // Four sample debounce
    | (TIM_CCMR1_IC1PSC_0 * 0)  // No-prescalar
    | (TIM_CCMR1_CC1S_0 * 1)    // TI1 input mode  
    ;
  
  TIM16->CCER = TIM17->CCER = 0
    | (TIM_CCER_CC1P * 0)       // Rising edge
    | TIM_CCER_CC1E             // Enable Capture 1
    ;

  TIM16->DIER = TIM17->DIER = 0
    | TIM_DIER_UIE
    ;

  TIM16->CR2 = TIM17->CR2 = 0
    ;

  // Configure the pins
  CAPO::reset();
  CAPO::mode(MODE_OUTPUT);
  
  CAP1I::alternate(2);
  CAP1I::mode(MODE_ALTERNATE);

  CAP2I::alternate(2);
  CAP2I::mode(MODE_ALTERNATE);

  NVIC_EnableIRQ(TIM16_IRQn);
  NVIC_SetPriority(TIM16_IRQn, 3);
}

extern "C" void TIM16_IRQHandler(void) {
  touch[0] = (TIM16->SR & TIM_SR_CC1IF) ? TIM16->CCR1 : 0xFFFF;
  touch[1] = (TIM17->SR & TIM_SR_CC1IF) ? TIM17->CCR1 : 0xFFFF;

  CAPO::reset();
  TIM16->SR = TIM17->SR = 0;
}

void Touch::transmit(uint16_t* data) {
  memcpy(data, touch, sizeof(touch));
}

void Touch::tick(void) {
  // Start Counting
  __disable_irq();
  TIM16->CR1 = TIM17->CR1 = 0
    | TIM_CR1_OPM               // One pulse mode
    | TIM_CR1_CEN               // Enabled
    ;
  CAPO::set();
  __enable_irq();
}
