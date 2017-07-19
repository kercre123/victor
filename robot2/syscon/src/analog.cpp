#include <string.h>

#include "common.h"
#include "hardware.h"

#include "analog.h"
#include "power.h"

static const int channels[ADC_CHANNELS] = {
  ADC_CHSELR_CHSEL2,
  ADC_CHSELR_CHSEL5,
  ADC_CHSELR_CHSEL6
};

static const int POWER_DOWN_TIME = 333; // Hold down for 5s
static const int BUTTON_THRESHOLD = 0xC00;
static const int BOUNCE_LENGTH = 3;

// Since the analog portion is shared with the bootloader, this has to be in the shared ram space
uint16_t Analog::values[ADC_CHANNELS] __attribute__((section("SHARED_RAM")));
bool Analog::button_pressed __attribute__((section("SHARED_RAM")));
static bool bouncy_button __attribute__((section("SHARED_RAM")));
static int bouncy_count __attribute__((section("SHARED_RAM")));
static int current_channel __attribute__((section("SHARED_RAM")));
static uint16_t calibration_value __attribute__((section("SHARED_RAM")));
static int hold_count __attribute__((section("SHARED_RAM")));

static void start_sample(void) {
  using namespace Analog;
  
  if (++current_channel >= ADC_CHANNELS) {
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
      }
    } else {
      if (hold_count >= POWER_DOWN_TIME) {
        Power::stop();
      } else {
        hold_count = 0;
      }
    }

    current_channel = 0;
  }

  ADC1->CHSELR = channels[current_channel];
  ADC1->CR |= ADC_CR_ADSTART;
}

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

  // Enable ADC1
  if (ADC1->ISR & ADC_ISR_ADRDY) {
    ADC1->ISR |= ADC_ISR_ADRDY;
  }
  ADC1->CR |= ADC_CR_ADEN;
  while (~ADC1->ISR & ADC_ISR_ADRDY) ;

  // Configure ADC
  ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2;
  ADC->CCR |= ADC_CCR_VREFEN;

  // Kick off the ADC
  current_channel = 0;
  hold_count = 0;
  start_sample();
  
}

void Analog::stop(void) {
  ADC1->CR |= ADC_CR_ADSTP;
  while ((ADC1->CR & ADC_CR_ADSTP) != 0) ;

  ADC1->CR |= ADC_CR_ADDIS;
  while ((ADC1->CR & ADC_CR_ADEN) != 0) ;
}

void Analog::tick(void) {
  // While we have samples waiting
  if (ADC1->ISR & ADC_ISR_EOC) {
    values[current_channel] = ADC1->DR;

    start_sample();
  }
}
