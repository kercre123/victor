#include <string.h>

#include "common.h"
#include "hardware.h"

#include "messages.h"

#include "analog.h"
#include "power.h"
#include "vectors.h"
#include "flash.h"

#ifndef BOOTLOADER
#include "contacts.h"
#endif

static const int SELECTED_CHANNELS = 0
  | ADC_CHSELR_CHSEL2
  | ADC_CHSELR_CHSEL4
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL16
  ;

static const uint16_t LOW_VOLTAGE_POWER_DOWN_POINT = ADC_VOLTS(3.4);
static const int      LOW_VOLTAGE_POWER_DOWN_TIME = 200;  // 1s
static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.3);
static const uint32_t FALLING_EDGE = ADC_WINDOW(ADC_VOLTS(3.50), ~0);

static const uint16_t*  TEMP30_CAL_ADDR = (uint16_t*)0x1FFFF7B8;
static const int32_t    TEMP_VOLT_ADJ   = (int32_t)(0x100000 * (2.8 / 3.3));
static const int32_t    TEMP_SCALE_ADJ  = (int32_t)(0x100000 * (1.000 / 5.336));

static const int POWER_DOWN_TIME = 200 * 4.5;               // Shutdown
static const int POWER_WIPE_TIME = 200 * 12;                // Enter recovery mode
static const int MAX_CONTACT_TIME = 200 * 10;               // 10 seconds
static const int MAX_CHARGE_TIME = 200 * 60 * 30;           // 30 minutes
static const int MAX_CHARGE_TIME_WD = MAX_CHARGE_TIME + 5;  // 25ms past MAX_CHARGE_TIME

static const int BOUNCE_LENGTH = 3;
static const int MINIMUM_RELEASE_UNSTUCK = 20;
static const int BUTTON_THRESHOLD = ADC_VOLTS(2.0) * 2;

static bool is_charging = false;
static bool on_charger = false;

// Assume we started on the charger
static bool allow_power;
static int vext_debounce;
static int32_t temperature;
static bool last_vext = true;
static bool charger_shorted = false;
static bool disable_charger = false;

static bool bouncy_button = false;
static int bouncy_count = 0;
static int hold_count = 0;
static int total_release = 0;

#define CHARGE_CUTOFF (vext_debounce >= MAX_CHARGE_TIME)

static uint16_t volatile adc_values[ADC_CHANNELS];
bool Analog::button_pressed = false;

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
              | ADC_CFGR1_AWDEN
              | ADC_CFGR1_AWDSGL
              | (ADC_CFGR1_AWDCH_0 * 4) // ADC Channel 4 (VMAIN)
              ;
  ADC1->CFGR2 = 0
              | ADC_CFGR2_CKMODE_1
              ;
  ADC1->SMPR  = 0
              | ADC_SMPR_SMP_0  // ~0.4us
              | ADC_SMPR_SMP_2
              ;

  ADC1->IER = ADC_IER_AWDIE;
  ADC1->TR = FALLING_EDGE;

  ADC->CCR = 0
           | ADC_CCR_TSEN
           ;

  // Enable ADC
  ADC1->ISR = ADC_ISR_ADRDY;
  ADC1->CR |= ADC_CR_ADEN;
  while (~ADC1->ISR & ADC_ISR_ADRDY) ;

  // Start sampling
  ADC1->CR |= ADC_CR_ADSTART;

  // DMA in continuous mode
  DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
  DMA1_Channel1->CMAR = (uint32_t)&adc_values[0];
  DMA1_Channel1->CNDTR = ADC_CHANNELS;
  DMA1_Channel1->CCR |= 0
                     | DMA_CCR_MINC
                     | DMA_CCR_MSIZE_0
                     | DMA_CCR_PSIZE_0
                     | DMA_CCR_CIRC;
  DMA1_Channel1->CCR |= DMA_CCR_EN;

  #ifdef BOOTLOADER
  allow_power = false;
  #else
  allow_power = true;
  #endif

  NVIC_DisableIRQ(ADC1_IRQn);
  NVIC_SetPriority(ADC1_IRQn, PRIORITY_ADC);

  // Configure all our GPIO to what it needs to be
  #ifdef BOOTLOADER
  nCHG_PWR::type(TYPE_PUSHPULL);
  nCHG_PWR::mode(MODE_OUTPUT);
  nCHG_PWR::reset();

  BODY_TX::alternate(0);  // USART1_TX
  BODY_TX::speed(SPEED_HIGH);
  BODY_TX::set();
  BODY_TX::mode(MODE_OUTPUT);

  BODY_RX::alternate(0);  // USART1_RX
  BODY_RX::speed(SPEED_HIGH);
  BODY_RX::mode(MODE_ALTERNATE);

  VEXT_TX::alternate(1);  // USART2_TX
  VEXT_TX::speed(SPEED_HIGH);
  VEXT_TX::pull(PULL_NONE);
  VEXT_TX::mode(MODE_ALTERNATE);

  // Motor should be primed for reset
  LN1::reset(); LN1::mode(MODE_OUTPUT);
  LN2::reset(); LN2::mode(MODE_OUTPUT);
  HN1::reset(); HN1::mode(MODE_OUTPUT);
  HN2::reset(); HN2::mode(MODE_OUTPUT);
  RTN1::reset(); RTN1::mode(MODE_OUTPUT);
  RTN2::reset(); RTN2::mode(MODE_OUTPUT);
  LTN1::reset(); LTN1::mode(MODE_OUTPUT);
  LTN2::reset(); LTN2::mode(MODE_OUTPUT);
  
  // Configure P pins 
  LP1::reset();
  LP1::type(TYPE_OPENDRAIN); 
  LP1::mode(MODE_OUTPUT);
  HP1::type(TYPE_OPENDRAIN); 
  HP1::reset();   
  HP1::mode(MODE_OUTPUT);
  RTP1::type(TYPE_OPENDRAIN); 
  RTP1::reset();  
  RTP1::mode(MODE_OUTPUT);
  LTP1::type(TYPE_OPENDRAIN); 
  LTP1::reset();  
  LTP1::mode(MODE_OUTPUT);

  // Encoders
  LENCA::mode(MODE_INPUT);
  LENCB::mode(MODE_INPUT);
  HENCA::mode(MODE_INPUT);
  HENCB::mode(MODE_INPUT);
  RTENC::mode(MODE_INPUT);
  LTENC::mode(MODE_INPUT);

  nVENC_EN::set();
  nVENC_EN::mode(MODE_OUTPUT);

  #ifndef DEBUG
  // Cap-sense
  CAPO::reset();
  CAPO::mode(MODE_OUTPUT);
  CAPI::alternate(2);
  CAPI::mode(MODE_ALTERNATE);
  
  // LEDs
  LED_CLK::type(TYPE_OPENDRAIN);
  LED_DAT::reset();
  LED_CLK::reset();
  LED_DAT::mode(MODE_OUTPUT);
  LED_CLK::mode(MODE_OUTPUT);

  leds_off();
  #endif
  #endif

  POWER_EN::mode(MODE_INPUT);
  POWER_EN::pull(PULL_UP);
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
  data->battery.main_voltage = adc_values[ADC_VMAIN];
  data->battery.charger = adc_values[ADC_VEXT];
  data->battery.temperature = (int16_t) temperature;
  data->battery.flags = 0
                      | (is_charging ? POWER_IS_CHARGING : 0)
                      | (on_charger ? POWER_ON_CHARGER : 0)
                      | (charger_shorted ? POWER_CHARGER_SHORTED : 0 )
                      | (CHARGE_CUTOFF ? POWER_BATTERY_DISCONNECTED : 0)
                      ;

  data->touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
}

#ifndef BOOTLOADER
void Analog::receive(HeadToBody* data) {
  if (data->powerFlags & POWER_DISCONNECT_CHARGER) {
    disable_charger = true;
  }
  if (data->powerFlags & POWER_CONNECT_CHARGER) {
    disable_charger = false;
  }
}

void Analog::inhibitCharge(bool force) {
  // UART may not take over if 
  if (on_charger && !force) return ;

  vext_debounce = 0;
  disable_charger = true;
}
#endif

void Analog::setPower(bool powered) {
  allow_power = powered;
}

void Analog::tick(void) {
  static bool disable_vmain = false;

  // On-charger delay
  bool vext_now = adc_values[ADC_VEXT] >= TRANSITION_POINT;
  bool enable_watchdog = (vext_debounce >= MAX_CHARGE_TIME_WD);
  bool button_now = (adc_values[ADC_BUTTON] >= BUTTON_THRESHOLD);

  if (vext_now == last_vext) {
    if (!vext_now) {
      on_charger = false;
      vext_debounce = 0;
    } else if (!enable_watchdog) {
      on_charger = true;
      vext_debounce++;
    }
  } else {
    last_vext = vext_now;
  }

  #ifdef BOOTLOADER
  static bool has_booted = false;
  
  if (!has_booted && (button_now || on_charger)) {
    has_booted = true;
    Power::setMode(POWER_CALM);
  }
  #endif

  temperature = *TEMP30_CAL_ADDR - ((adc_values[ADC_TEMP] * TEMP_VOLT_ADJ) >> 20);
  temperature = ((temperature * TEMP_SCALE_ADJ) >> 20) + 30;

  bool emergency_shutoff = temperature >= 70;    // Will immediately cause a reboot
  if (temperature >= 60) disable_vmain = true;

  if (emergency_shutoff) {
    nCHG_PWR::set();
    POWER_EN::reset();
    POWER_EN::mode(MODE_OUTPUT);
    POWER_EN::pull(PULL_NONE);
  } else if (!allow_power) {
    NVIC_DisableIRQ(ADC1_IRQn);

    if (on_charger) {
      // Powered off charger to stop reboot loop
      nCHG_PWR::reset();
      POWER_EN::pull(PULL_NONE);
      POWER_EN::mode(MODE_INPUT);
    } else {
      // Explicitly powered down
      nCHG_PWR::set();
      POWER_EN::reset();
      POWER_EN::mode(MODE_OUTPUT);
      POWER_EN::pull(PULL_NONE);
    }

    is_charging = false;
  } else if (!on_charger) {
    NVIC_DisableIRQ(ADC1_IRQn);
    // Powered, off charger
    nCHG_PWR::set();

    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);

    is_charging = false;
    #ifndef BOOTLOADER
    if (!Contacts::dataQueued) {
      disable_charger = false;
    }
    #endif

    // Charge contact short sense
    static const int CHARGE_DELAY = 32;
    static const uint32_t CHARGE_MINIMUM = ADC_VOLTS(0.30f) * CHARGE_DELAY;
    static const int CHARGE_SHORT_LIMIT = 5;
    static uint32_t charge_shorted = 0;
    static uint32_t charge_count = 0;
    static uint32_t charge_sum = 0;

    charge_sum += adc_values[ADC_VEXT];
    if (++charge_count >= CHARGE_DELAY) {
      if (charge_sum > CHARGE_MINIMUM) {
        charger_shorted = false;
        charge_shorted = 0;
      } else {
        if (++charge_shorted > CHARGE_SHORT_LIMIT) charger_shorted = true;
      }
      charge_sum = 0;
      charge_count = 0;
    }

    // Low voltage shutdown
    static int power_down_timer = LOW_VOLTAGE_POWER_DOWN_TIME;
    if (adc_values[ADC_VMAIN] < LOW_VOLTAGE_POWER_DOWN_POINT) {
      if (--power_down_timer <= 0) {
        Power::setMode(POWER_STOP);
      }
    } else {
      power_down_timer = LOW_VOLTAGE_POWER_DOWN_TIME;
    }
  } else if (CHARGE_CUTOFF) {
    // Unpowered, on charger (timeout)
    nCHG_PWR::reset();
    POWER_EN::pull(PULL_NONE);
    POWER_EN::mode(MODE_INPUT);

    is_charging = false;

    if (enable_watchdog) {
      ADC1->ISR = ADC_ISR_AWD;
      NVIC_EnableIRQ(ADC1_IRQn);
    }
  } else {
    // On charger, powered (charging)
    NVIC_DisableIRQ(ADC1_IRQn);
   
    // Charge circuit disable
    #ifndef BOOTLOADER
    // Prevent charge contacts from blocking charging entirely (you have 5 minutes, craig)
    if (vext_debounce >= MAX_CONTACT_TIME && disable_charger) {
      disable_charger = false;
      vext_debounce = 0;
    }

    if (disable_charger) {
      nCHG_PWR::set();
      is_charging = false;
    } else {
      nCHG_PWR::reset();
      is_charging = true;
    }
    #else
    nCHG_PWR::reset();
    is_charging = true;
    #endif

    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);
  }

  // VMain / VBat trap
  if (disable_vmain) {
    Power::setMode(POWER_STOP);
    return ;
  }

  // Trap stuck down button
  if (total_release < MINIMUM_RELEASE_UNSTUCK) {
    if (!button_now) {
      total_release++;
    } else {
      total_release = 0;
    }

    return ;
  }

  // Debounce the button
  if (bouncy_button != button_now) {
    bouncy_button = button_now;
    bouncy_count = 0;
  } else if (++bouncy_count > BOUNCE_LENGTH) {
    button_pressed = bouncy_button;
  }

  if (button_pressed) {
    if (++hold_count >= POWER_WIPE_TIME) {
      // We will be signaling a recovery
      BODY_TX::reset();

      if (on_charger) {
        // Re-enable power to the head
        Power::setMode(POWER_ACTIVE);
      }
    } else if (hold_count >= POWER_DOWN_TIME) {
      // Do not signal recovery
      BODY_TX::set();
      BODY_TX::mode(MODE_INPUT);

      Analog::setPower(false);
      Power::setMode(POWER_STOP);
    } else {
      Power::setMode(POWER_ACTIVE);
    }
  } else {
    hold_count = 0;
  }
}

extern "C" void ADC1_IRQHandler(void) {
  ADC1->ISR = ADC_ISR_AWD;
  NVIC_DisableIRQ(ADC1_IRQn);

  POWER_EN::set();
  POWER_EN::mode(MODE_OUTPUT);
  POWER_EN::pull(PULL_UP);
  POWER_EN::mode(MODE_INPUT);
}
