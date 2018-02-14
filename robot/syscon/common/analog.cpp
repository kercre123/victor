#include <string.h>

#include "common.h"
#include "hardware.h"

#include "messages.h"

#include "analog.h"
#include "power.h"
#include "vectors.h"
#include "flash.h"
#include "lights.h"

static const int SELECTED_CHANNELS = 0
  | ADC_CHSELR_CHSEL2
  | ADC_CHSELR_CHSEL4
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL16
  | ADC_CHSELR_CHSEL17
  ;

static const uint16_t POWER_DOWN_POINT = ADC_VOLTS(3.4);
static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.5);
static const uint32_t FALLING_EDGE = ADC_WINDOW(TRANSITION_POINT, ~0);

static const int POWER_DOWN_TIME = 200 * 2;   // Shutdown
static const int POWER_WIPE_TIME = 200 * 10;  // Enter recovery mode
static const int MINIMUM_VEXT_TIME = 20; // 0.1s

static const int BUTTON_THRESHOLD = ADC_VOLTS(5.45); // Must be halved from actual
static const int BOUNCE_LENGTH = 3;
static const int MINIMUM_RELEASE_UNSTUCK = 20;

static volatile bool onBatPower;
static bool chargeAllowed;
static bool is_charging;
static int vext_debounce;
static bool last_vext = false;
static bool bouncy_button = false;
static int bouncy_count = 0;
static int hold_count = 0;
static int total_release = 0;
static bool on_charger = false;

uint16_t volatile Analog::values[ADC_CHANNELS];
bool Analog::button_pressed = false;
uint16_t Analog::battery_voltage = 0;

void Analog::init(void) {
  // Calibrate ADC1
  if ((ADC1->CR & ADC_CR_ADEN) != 0) {
    ADC1->CR |= ADC_CR_ADDIS;
  }
  while ((ADC1->CR & ADC_CR_ADEN) != 0) ;
  ADC1->CFGR1 &= ~ADC_CFGR1_DMAEN;
  ADC1->CR |= ADC_CR_ADCAL;
  while ((ADC1->CR & ADC_CR_ADCAL) != 0) ;

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
              ;
  ADC1->IER = ADC_IER_AWDIE;
  ADC1->TR = FALLING_EDGE;

  ADC->CCR = 0
           | ADC_CCR_TSEN
           | ADC_CCR_VREFEN;

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

  #ifdef BOOTLOADER
  POWER_EN::mode(MODE_INPUT);
  POWER_EN::pull(PULL_UP);

  Power::enableHead();

  CHG_EN::type(TYPE_OPENDRAIN);
  CHG_PWR::type(TYPE_OPENDRAIN);

  CHG_EN::mode(MODE_OUTPUT);
  CHG_PWR::mode(MODE_OUTPUT);

  Analog::allowCharge(true);
  #endif
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
  data->battery.battery = values[ADC_VMAIN];
  data->battery.charger = values[ADC_VEXT];
  data->battery.temperature = values[ADC_TEMP];
  data->battery.flags = 0
                      | (is_charging ? POWER_IS_CHARGING : 0)
                      | (on_charger ? POWER_ON_CHARGER : 0)
                      ;

  data->touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
}

void Analog::allowCharge(bool enable) {
  chargeAllowed = enable;

  if (enable) {
    CHG_EN::set();
  } else {
    CHG_EN::reset();
  }

  vext_debounce = 0;
}

bool Analog::delayCharge() {
  vext_debounce = 0;
  return is_charging;
}

void Analog::tick(void) {
  // On-charger delay
  bool vext_now = Analog::values[ADC_VEXT] >= TRANSITION_POINT;

  // Emergency trap (V3.3)
  if (!is_charging) {
    battery_voltage = Analog::values[ADC_VMAIN];
    if (Analog::values[ADC_VMAIN] < POWER_DOWN_POINT) {
      Power::setMode(POWER_STOP);
    }
  }

  // Debounced VEXT line
  if (vext_now && last_vext) {
    vext_debounce++;
  } else {
    vext_debounce = 0;
  }

  last_vext = vext_now;

  // Charge logic
  on_charger = vext_debounce >= MINIMUM_VEXT_TIME;
  is_charging = chargeAllowed && on_charger;
  if (is_charging) {
    CHG_PWR::set();
  } else {
    CHG_PWR::reset();
  }

  // Button logic
  bool new_button = (values[ADC_BUTTON] >= BUTTON_THRESHOLD);

  // Trap stuck down button
  if (total_release < MINIMUM_RELEASE_UNSTUCK) {
    if (!new_button) {
      total_release++;
    } else {
      total_release = 0;
    }

    return ;
  }

  // Debounce the button
  if (bouncy_button != new_button) {
    bouncy_button = new_button;
    bouncy_count = 0;
  } else if (++bouncy_count > BOUNCE_LENGTH) {
    button_pressed = bouncy_button;
  }

  if (button_pressed) {
    hold_count++;
    if (hold_count >= POWER_WIPE_TIME) {
      // We will be signaling a recovery
      BODY_TX::reset();
      BODY_TX::mode(MODE_OUTPUT);

      // Reenable power to the head
      Power::enableHead();
    } else if (hold_count >= POWER_DOWN_TIME) {
      Power::disableHead();
      Lights::disable();
    } else {
      Power::setMode(POWER_ACTIVE);
      Power::enableHead();
    }
  } else {
    if (hold_count >= POWER_DOWN_TIME && hold_count < POWER_WIPE_TIME) {
      Power::setMode(POWER_STOP);
    }

    hold_count = 0;
  }
}
