#include <string.h>

#include "common.h"
#include "hardware.h"

#include "touch.h"

static const int OVERSAMPLE = 8;
static const int MAX_SAMPLE_TIME = 0xFFFF / OVERSAMPLE;
static int samples;
static uint32_t touch;

void Touch::init(void) {
  TIM16->PSC = 0;
  TIM16->ARR = MAX_SAMPLE_TIME;

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

static void kickoff() {
  TIM16->CR1 = 0
    | TIM_CR1_OPM               // One pulse mode
    | TIM_CR1_CEN               // Enabled
    ;
  CAPO::set();
}

extern "C" void TIM16_IRQHandler(void) {
  touch += (TIM16->SR & TIM_SR_CC1IF) ? TIM16->CCR1 : MAX_SAMPLE_TIME;

  CAPO::reset();
  TIM16->SR = 0;

  if (--samples) kickoff();
}

void Touch::transmit(uint16_t* data) {
  *data = (uint16_t) touch;
}

void Touch::tick(void) {
  samples = OVERSAMPLE;

  // Start Counting
  __disable_irq();
  kickoff();
  __enable_irq();
}
