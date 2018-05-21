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
  | ADC_CHSELR_CHSEL3
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL16
  ;

static const uint16_t LOW_VOLTAGE_POWER_DOWN_POINT = ADC_VOLTS(3.4);
static const int      LOW_VOLTAGE_POWER_DOWN_TIME = 200;  // 1s
static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.3);
static const uint32_t FALLING_EDGE = ADC_WINDOW(ADC_VOLTS(3.5), ~0);

static const uint16_t*  TEMP30_CAL_ADDR = (uint16_t*)0x1FFFF7B8;
static const int32_t    TEMP_VOLT_ADJ   = (int32_t)(0x100000 * (2.8 / 3.3));
static const int32_t    TEMP_SCALE_ADJ  = (int32_t)(0x100000 * (1.000 / 5.336));

static const int POWER_DOWN_TIME = 200 * 4.5;   // Shutdown
static const int POWER_WIPE_TIME = 200 * 12;  // Enter recovery mode
static const int MINIMUM_VEXT_TIME = 20; // 0.1s
static const int CHARGE_ENABLE_DELAY = 50; // 0.25s
static const int MAX_CHARGE_TIME = 200 * 10; // 30 minutes
static const int MAX_CHARGE_TIME_WD = MAX_CHARGE_TIME + 20; // 100ms past MAX_CHARGE_TIME

static const int BOUNCE_LENGTH = 3;
static const int MINIMUM_RELEASE_UNSTUCK = 20;
static const int BUTTON_THRESHOLD = ADC_VOLTS(2.0) * 2;

static bool is_charging;
static bool allow_power;
static bool on_charger = false;

// Assume we started on the charger
static int vext_debounce = MINIMUM_VEXT_TIME;
static int charge_delay = CHARGE_ENABLE_DELAY;
static int32_t temperature;
static bool last_vext = true;
static bool charger_shorted = false;

static bool bouncy_button = false;
static int bouncy_count = 0;
static int hold_count = 0;
static int total_release = 0;

#define CHARGE_CUTOFF (vext_debounce >= MAX_CHARGE_TIME)

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
              | ADC_CFGR1_AWDEN
              | ADC_CFGR1_AWDSGL
              | (ADC_CFGR1_AWDCH_0 * 2) // ADC Channel 2 (VEXT)
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
  DMA1_Channel1->CMAR = (uint32_t)&values[0];
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
  data->battery.battery = values[ADC_VMAIN];
  data->battery.charger = values[ADC_VEXT];
  data->battery.temperature = (int16_t) temperature;
  data->battery.flags = 0
                      | (is_charging ? POWER_IS_CHARGING : 0)
                      | (on_charger ? POWER_ON_CHARGER : 0)
                      | (charger_shorted ? POWER_CHARGER_SHORTED : 0 )
                      | (CHARGE_CUTOFF ? POWER_BATTERY_DISCONNECTED : 0)
                      ;

  data->touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
}

void Analog::delayCharge() {
  charge_delay = 0;
}

void Analog::setPower(bool powered) {
  allow_power = powered;
}

void Analog::tick(void) {
  static bool disable_vmain = false;

  // On-charger delay
  bool vext_now = Analog::values[ADC_VEXT] >= TRANSITION_POINT;
  bool enable_watchdog = vext_debounce >= MAX_CHARGE_TIME_WD;
  bool button_now = (values[ADC_BUTTON] >= BUTTON_THRESHOLD);

  if (vext_now && last_vext) {
    if (!enable_watchdog) vext_debounce++;
  } else {
    vext_debounce = 0;
  }

  last_vext = vext_now;
  on_charger = vext_debounce >= MINIMUM_VEXT_TIME;

  temperature = *TEMP30_CAL_ADDR - ((Analog::values[ADC_TEMP] * TEMP_VOLT_ADJ) >> 20);
  temperature = ((temperature * TEMP_SCALE_ADJ) >> 20) + 30;

  //bool emergency_shutoff = temperature >= 60;    // Will immediately cause a reboot
  //if (temperature >= 45) disable_vmain = true;
  bool emergency_shutoff = false;
  
  #ifdef BOOTLOADER
  static bool has_booted = false;
  
  if (!has_booted && (button_now || on_charger)) {
    Power::setMode(POWER_CALM);
  }
  #endif

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
    charge_delay = 0;
  } else if (!on_charger) {
    NVIC_DisableIRQ(ADC1_IRQn);
    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);

    // Powered, off charger
    nCHG_PWR::set();

    battery_voltage = Analog::values[ADC_VMAIN];
    is_charging = false;
    charge_delay = 0;

    // Charge contact short sense
    static const int CHARGE_DELAY = 32;
    static const uint32_t CHARGE_MINIMUM = ADC_VOLTS(0.30f) * CHARGE_DELAY;
    static const int CHARGE_SHORT_LIMIT = 5;
    static uint32_t charge_shorted = 0;
    static uint32_t charge_count = 0;
    static uint32_t charge_sum = 0;

    charge_sum += Analog::values[ADC_VEXT];
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
    if (battery_voltage < LOW_VOLTAGE_POWER_DOWN_POINT) {
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
   
    // Don't enable power to charge circuit until a second has passed
    if (charge_delay >= CHARGE_ENABLE_DELAY) {
      nCHG_PWR::reset();
    } else {
      nCHG_PWR::set();
      charge_delay++;
    }

    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);

    battery_voltage = Analog::values[ADC_VMAIN];

    is_charging = true;
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
    if (on_charger && hold_count >= POWER_WIPE_TIME) {
      // Reenable power to the head
      Power::setMode(POWER_ACTIVE);

      // We will be signaling a recovery
      BODY_TX::reset();
      BODY_TX::mode(MODE_OUTPUT);
    } else if (++hold_count >= POWER_DOWN_TIME) { // Increment is here to prevent overflow in recovery condition
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
  //nCHG_PWR::set();
}
