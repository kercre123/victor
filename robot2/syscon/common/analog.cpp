#include <string.h>

#include "common.h"
#include "hardware.h"

#include "messages.h"

#include "analog.h"
#include "power.h"
#include "vectors.h"
#include "flash.h"

static const int SELECTED_CHANNELS = 0
  | ADC_CHSELR_CHSEL2
  | ADC_CHSELR_CHSEL4
  | ADC_CHSELR_CHSEL6;

// Since the analog portion is shared with the bootloader, this has to be in the shared ram space
uint16_t volatile Analog::values[ADC_CHANNELS];
static uint16_t calibration_value;

void Analog::init(void) {
  // Calibrate ADC1
  if ((ADC1->CR & ADC_CR_ADEN) != 0) {
    ADC1->CR |= ADC_CR_ADDIS;
  }
  while ((ADC1->CR & ADC_CR_ADEN) != 0) ;
  ADC1->CFGR1 &= ~ADC_CFGR1_DMAEN;
  ADC1->CR |= ADC_CR_ADCAL;
  while ((ADC1->CR & ADC_CR_ADCAL) != 0) ;
  calibration_value = ADC1->DR;

  // Setup the ADC for continuous mode
  ADC1->CHSELR = SELECTED_CHANNELS;
  ADC1->CFGR1 = 0
              | ADC_CFGR1_CONT
              | ADC_CFGR1_DMAEN
              | ADC_CFGR1_DMACFG
              ;
  ADC1->CFGR2 = 0
              | ADC_CFGR2_CKMODE_1
              ;
  ADC1->SMPR  = 0
              | ADC_SMPR_SMP  // Long polling time
              ;

  // Enable VRef
  ADC->CCR = ADC_CCR_VREFEN;

  // Enable ADC
  ADC1->ISR = ADC_ISR_ADRDY;
  ADC1->CR |= ADC_CR_ADEN;
  while (~ADC1->ISR & ADC_ISR_ADRDY) ;

  // Start sampling
  ADC1->CR |= ADC_CR_ADSTART;

  // DMA in continuous mode
  DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
  DMA1_Channel1->CMAR = (uint32_t)&values[0];
  DMA1_Channel1->CNDTR = ADC_CHANNELS;
  DMA1_Channel1->CCR |= 0
                     | DMA_CCR_MINC
                     | DMA_CCR_MSIZE_0
                     | DMA_CCR_PSIZE_0
                     | DMA_CCR_CIRC;
  DMA1_Channel1->CCR |= DMA_CCR_EN;
}

void Analog::stop(void) {
  // Disable the DMA
  DMA1_Channel1->CCR = 0;

  // Stop reading values
  ADC1->CR |= ADC_CR_ADSTP;
  while ((ADC1->CR & ADC_CR_ADSTP) != 0) ;

  ADC1->CR |= ADC_CR_ADDIS;
  while ((~ADC1->CR & ADC_CR_ADEN) != 0) ;
}

void Analog::transmit(BodyToHead* data) {
  data->battery.battery = values[ADC_VBAT];
  data->battery.charger = values[ADC_VEXT];
}

#ifndef BOOTLOADER
#include "lights.h"

static const int POWER_DOWN_TIME = 200 * 2;   // Shutdown
static const int POWER_WIPE_TIME = 200 * 10;  // Erase flash
static const int BUTTON_THRESHOLD = 0xD00;
static const int BOUNCE_LENGTH = 3;

bool Analog::button_pressed;

void Analog::tick(void) {
  static bool bouncy_button;
  static int bouncy_count;
  static int hold_count;

  // Debounce buttons
  bool new_button = (values[ADC_BUTTON] >= BUTTON_THRESHOLD);

  if (bouncy_button != new_button) {
    bouncy_button = new_button;
    bouncy_count = 0;
  } else if (++bouncy_count > BOUNCE_LENGTH) {
    button_pressed = bouncy_button;
  }

  if (button_pressed) {
    if (hold_count < POWER_DOWN_TIME) {
      hold_count++;
    } else if (hold_count < POWER_WIPE_TIME) {
      Lights::disable();
    } else {
      Power::softReset(true);
    }
  } else {
    if (hold_count >= POWER_DOWN_TIME) {
      Power::stop();
    } else {
      hold_count = 0;
    }
  }
}
#endif
