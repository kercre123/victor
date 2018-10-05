#include <string.h>

#include "common.h"
#include "hardware.h"

#include "touch.h"

static volatile uint32_t sumandcount;   // Eternally accumulating SUM and COUNT
const int SAMPLE_INTERVAL = 18481;      // ~13x oversample, prime to 245600 cyc/tick

const int BOXCAR_WIDTH = 39;  // 50&60Hz integer divisor: 15625/80/39 = 5.008Hz (good enough)
const int COUNT_BITS = 10;    // 2^x max number of readings in boxcar (in bits)
const int GAIN_BITS = 3;      // 2^x gain (in bits)

void Touch::init(void) {
  CAPI::reset();
  
  TIM16->PSC = 0;
  TIM16->ARR = SAMPLE_INTERVAL-1; 
  
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
  
  TIM16->CR1 = TIM_CR1_CEN;     // Start running
}

extern "C" void TIM16_IRQHandler(void) {
  if (TIM16->SR & TIM_SR_CC1IF) sumandcount += ((TIM16->CCR1<<COUNT_BITS) | 1);
  TIM16->SR = 0;

  __disable_irq();
  CAPI::modenoirq(MODE_OUTPUT);
  CAPI::modenoirq(MODE_OUTPUT); // ~10 cycles (100:1 discharge:charge, 1000 worst case TIM16 discharge)
  TIM16->CR1 = 0
    | TIM_CR1_OPM               // One pulse mode
    | TIM_CR1_CEN               // Enabled
    ;
  CAPI::modenoirq(MODE_ALTERNATE);
  __enable_irq();
}

void Touch::transmit(uint16_t* data) {
  static uint32_t boxcar[BOXCAR_WIDTH] = {0};
  static uint8_t boxi = 0;
  if (++boxi >= BOXCAR_WIDTH)
    boxi = 0;
  
  // Pull latest sum and count, boxcar filtered
  uint32_t latest = sumandcount;
  uint32_t sum =   (latest - boxcar[boxi]) >> (COUNT_BITS-GAIN_BITS);
  uint32_t count = (latest - boxcar[boxi]) & ((1<<COUNT_BITS)-1);
  boxcar[boxi] = latest;
  
  if (count) sum /= count;    // Average
  data[0] = sum >> GAIN_BITS; // Low-res/compatibility slot
  data[4] = sum;              // Hi-res slot
}
