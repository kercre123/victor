#include <string.h>

#include "common.h"
#include "hardware.h"

#include "messages.h"

#include "analog.h"
#include "power.h"
#include "vectors.h"
#include "flash.h"
#include "motors.h"

static const int SELECTED_CHANNELS = 0
  | ADC_CHSELR_CHSEL2
  | ADC_CHSELR_CHSEL3
  | ADC_CHSELR_CHSEL4
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL16
  | ADC_CHSELR_CHSEL17
  ;

static const uint16_t BATTERY_FULL_VOLTAGE = ADC_VOLTS(4.2);
static const int      CHARGE_FULL_TIME = 200 * 60 * 5;           // 5 minutes

static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.3);
static const uint32_t FALLING_EDGE = ADC_WINDOW(ADC_VOLTS(3.50), ~0);
static const int      MINIMUM_ON_CHARGER = 5;

static const uint16_t*  TEMP30_CAL_ADDR = (uint16_t*)0x1FFFF7B8;
static const uint16_t* VREFINT_CAL_ADDR = (uint16_t*)0x1FFFF7BA;
static const int32_t    TEMP_VOLT_ADJ   = (int32_t)(0x10000 * (2.8 / 3.3));
static const int32_t    TEMP_SCALE_ADJ  = (int32_t)(0x10000 * (1.000 / 5.336));

// We allow for 4 hours of over-heat time (variable)
static const int MAX_HEAT_COUNTDOWN = 200 * 60 * 60 * 4 * 5;
static const int OVERHEAT_SHUTDOWN = 200 * 30;

static const int POWER_DOWN_TIME = 200 * 5.5;               // Shutdown
static const int POWER_WIPE_TIME = 200 * 12;                // Enter recovery mode
static const int ON_CHARGER_RESET = 200 * 60;               // 1 Minute
static const int TOP_OFF_TIME    = 200 * 60 * 60 * 24 * 90; // 90 Days

static const int BOUNCE_LENGTH = 3;
static const int MINIMUM_RELEASE_UNSTUCK = 20;
static const int BUTTON_THRESHOLD = ADC_VOLTS(2.0) * 2;

static bool is_charging = false;
bool Analog::on_charger = false;
static bool charge_cutoff = false;
static bool too_hot = false;
static bool power_low = false;
static bool power_battery_shutdown = false;
static int heat_counter = 0;
static int low_power_count_up = 0;
static TemperatureAlarm temp_alarm = TEMP_ALARM_SAFE;

// Assume we started on the charger
static bool allow_power;

static int16_t temperature;
static bool disable_charger = false;
static int overheated = 0;

static uint16_t volatile adc_values[ADC_CHANNELS];
static bool button_pressed = false;

static const int ADC_SCALE_BITS = 15;
static uint32_t adc_compensate = 1 << ADC_SCALE_BITS;

static inline uint32_t EXACT_ADC(ADC_CHANNEL ch) {
  return (adc_values[ch] * adc_compensate) >> ADC_SCALE_BITS;
}

static void updateADCCompensate()
{
  const int OVERSAMPLE = 32;
  static uint8_t count = 0;
  static uint32_t vref_avg = 0;
  vref_avg += adc_values[ADC_VREF];

  if (++count == OVERSAMPLE)
  {
    // adc_compensate converts 3.3 calibration to 2.8 - making ADC values as if we had a perfect 2.8 VDDA

    adc_compensate = *VREFINT_CAL_ADDR * ((OVERSAMPLE << ADC_SCALE_BITS) * 33 / 28) / vref_avg;
    count = vref_avg = 0;
  }
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
              //| ADC_SMPR_SMP_1
              | ADC_SMPR_SMP_2
              ;

  ADC1->IER = ADC_IER_AWDIE;
  ADC1->TR = FALLING_EDGE;

  ADC->CCR = 0
           | ADC_CCR_VREFEN
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

  allow_power = true;

  NVIC_DisableIRQ(ADC1_IRQn);
  NVIC_SetPriority(ADC1_IRQn, PRIORITY_ADC);

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
  data->battery.main_voltage = EXACT_ADC(ADC_VMAIN);
  data->battery.charger = EXACT_ADC(ADC_VEXT);
  data->battery.temperature = (int16_t) temperature;
  data->battery.flags = 0
                      | (power_battery_shutdown ? POWER_BATTERY_SHUTDOWN : 0)
                      | (power_low ? POWER_IS_TOO_LOW : 0)
                      | (is_charging ? POWER_IS_CHARGING : 0)
                      | (on_charger ? POWER_ON_CHARGER : 0)
                      | (too_hot ? POWER_CHARGER_OVERHEAT : 0)
                      | ((overheated > 0) ? POWER_IS_OVERHEATED : 0)
                      | ((charge_cutoff && on_charger) ? POWER_BATTERY_DISCONNECTED : 0)
                      ;

  data->tempAlarm = temp_alarm;
  data->touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
}

void Analog::receive(HeadToBody* data) {
  if (data->powerFlags & POWER_DISCONNECT_CHARGER) {
    disable_charger = true;
  }
  if (data->powerFlags & POWER_CONNECT_CHARGER) {
    disable_charger = false;
  }
}

void Analog::setPower(bool powered) {
  allow_power = powered;
}

static void handleButton() {
  static bool bouncy_button = false;
  static int bouncy_count = 0;
  static int hold_count = 0;
  static int total_release = 0;

  bool button_now = (EXACT_ADC(ADC_BUTTON) >= BUTTON_THRESHOLD);

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

  // Button is not depressed
  if (!button_pressed) {
    hold_count = 0;
    return ;
  }

  // Button is held, handle appropriate cases
  if (++hold_count >= POWER_WIPE_TIME) {
    // Restart head in recovery mode
    if (Analog::on_charger) {
      // Reset encoder hysteresis to old values so that
      // factory menu cursor moves as expected.
      Motors::resetEncoderHysteresis();

      Power::signalRecovery();
      Power::setMode(POWER_ACTIVE);
    }
  } else if (hold_count >= POWER_DOWN_TIME) {
    Power::setMode(POWER_STOP);
  } else {
    Power::setMode(POWER_ACTIVE);
  }
}

template <const TemperatureAlarm alert, const int increment>
static inline bool alarmTimer(uint16_t temp, const int target) {
  if (temp < target) {
    return false ;
  }

  temp_alarm = alert;
  heat_counter += increment;

  return heat_counter > MAX_HEAT_COUNTDOWN;
}

static void handleTemperature() {
  int32_t temp_now;
  // Temperature logic
  if (IS_WHISKEY) {
    temp_now = (adc_values[ADC_THERMISTOR] * 4) / 112 - 53;
  } else {
    temp_now = *TEMP30_CAL_ADDR - ((EXACT_ADC(ADC_TEMP) * TEMP_VOLT_ADJ) >> 16);
    temp_now = ((temp_now * TEMP_SCALE_ADJ) >> 16) + 30;

    // We are running way too hot, have a bowl of boot loops.
    if (temp_now >= 70) {
      Power::setMode(POWER_STOP);
      return ;
    }
  }

  static int samples = 0;
  static int filt_temp = 0;
  filt_temp += temp_now;

  if (++samples >= 32) {
    temperature = filt_temp / 32;
    samples = 0;
    filt_temp = 0;
  }

  // Our filtered temp is cool enough to reset the counter
  if (temperature < 47) {
    temp_alarm = TEMP_ALARM_SAFE;
    if (heat_counter > 0) heat_counter--;
  } else {
    // Start processing our overheat alarms
    bool disable_vmain = false
      || alarmTimer<TEMP_ALARM_HOT, MAX_HEAT_COUNTDOWN>(temperature, 60) // Fire immediately
      || alarmTimer<TEMP_ALARM_MID,                 10>(temperature, 50) // Fire in ~2H
      || alarmTimer<TEMP_ALARM_LOW,                  5>(temperature, 47) // Fire in ~4H
      ;

    if (overheated == 0 && disable_vmain) {
      overheated = OVERHEAT_SHUTDOWN;
    }
  }

  // NOTE: This counter cannot be reset until it fires
  if (overheated > 0 && --overheated == 0) {
    Power::setMode(POWER_STOP);
  }
}

static void handleLowBattery() {
  // Levels
  static const uint32_t EMERGENCY_POWER_DOWN_POINT = ADC_VOLTS(3.4);
  static const uint32_t LOW_VOLTAGE_POWER_DOWN_POINT = ADC_VOLTS(3.62);
  static const int      LOW_VOLTAGE_POWER_DOWN_TIME = 45*200; // 45 seconds
  static const int      EARLY_POWER_COUNT_TIME = 200; // 1 second

  static const int      POWER_DOWN_BATTERY_TIME = 10*200; // 10 seconds
  static const int      POWER_DOWN_WARNING_TIME = 4*60*200 + POWER_DOWN_BATTERY_TIME; // 4 minutes

  // Low voltage shutdown
  static int power_down_timer = LOW_VOLTAGE_POWER_DOWN_TIME;
  static int power_down_limit = POWER_DOWN_WARNING_TIME;
  static int emergency_filter = EARLY_POWER_COUNT_TIME;

  uint32_t vmain = EXACT_ADC(ADC_VMAIN);

  // Emergency power down
  if (vmain < EMERGENCY_POWER_DOWN_POINT) {
    if (emergency_filter > 0 && --emergency_filter == 0) {
      Power::setMode(POWER_STOP);
    }
    return ;
  } else {
    emergency_filter = EARLY_POWER_COUNT_TIME;
  }

  // Crazy low power logic
  if (Analog::on_charger) {
    power_battery_shutdown = false;
    low_power_count_up = EARLY_POWER_COUNT_TIME;

    // Top-up power down timers while on charger
    if (power_down_limit < POWER_DOWN_WARNING_TIME) {
      ++power_down_limit;
    } else if (power_down_timer < LOW_VOLTAGE_POWER_DOWN_TIME) {
      ++power_down_timer;
    } else {
      power_low = false;
    }
  }
  // On boot, if voltage is low immediately off charger, warn and shut down fast.
  // Relying on rampost to show low battery icon and 
  // relying on vic-robot to detect and shutdown right away.
  else if (low_power_count_up < EARLY_POWER_COUNT_TIME) {
    if (vmain < LOW_VOLTAGE_POWER_DOWN_POINT) {
      power_low = true;
    }
    low_power_count_up++;
  } else if (power_low) {
    if (power_down_limit <= 0) {
      Power::setMode(POWER_STOP);
    } else if (--power_down_limit < POWER_DOWN_BATTERY_TIME) {
      power_battery_shutdown = true;
    }
  } else if (vmain < LOW_VOLTAGE_POWER_DOWN_POINT) {
    if (--power_down_timer <= 0) {
      power_low = true;
    }
  } else {
    power_down_timer = LOW_VOLTAGE_POWER_DOWN_TIME;
    power_down_limit = POWER_DOWN_WARNING_TIME;
  }
}

static bool shouldPowerRobot() {
  static int disable_countdown = 0;

  if (allow_power) {
    disable_countdown = 100; // 0.5 seconds before the robot will actually turn off
  } else if (disable_countdown > 0) {
    disable_countdown--;
  }

  return disable_countdown > 0;
}

static void debounceVEXT() {
  static bool last_vext = true;
  static int vext_debounce = MINIMUM_ON_CHARGER;
  bool vext_now = EXACT_ADC(ADC_VEXT) >= TRANSITION_POINT;

  if (vext_now == last_vext) {
    if (vext_debounce < MINIMUM_ON_CHARGER) {
      vext_debounce++;
    } else {
      Analog::on_charger = vext_now;
    }
  } else {
    vext_debounce = 0;
  }

  last_vext = vext_now;
}

void Analog::tick(void) {
  static bool delay_disable = true;
  static int on_charger_time = 0;
  static int off_charger_time = 0;
  static int charging_time = 0;

  updateADCCompensate();
  debounceVEXT();
  handleButton();
  handleLowBattery();
  handleTemperature();

  if (temperature > 41 && on_charger_time < 200) {
    too_hot = true;
  } else {
    too_hot = false;
  }

  bool prevent_charge = too_hot
    || disable_charger;

  if (on_charger) {
    // This holds the on_charger_time at zero if charging is disabled
    if (!prevent_charge && on_charger_time++ >= TOP_OFF_TIME) {
      on_charger_time = 0;
      charging_time = 0;
    }

    uint16_t vmain_adc = EXACT_ADC(ADC_VMAIN);

    if (vmain_adc > BATTERY_FULL_VOLTAGE) {
      if (on_charger_time < 200) {
        charging_time = CHARGE_FULL_TIME;
      } else {
        charging_time++;
      }

      // Unlikely that power_low == true,
      // but maybe possible with strategic charger hopping.
      power_low = false;
    }
  } else if (++off_charger_time >= ON_CHARGER_RESET) {
    on_charger_time = 0;
    charging_time = 0;
  }

  // Charge saturation time after desired voltage
  const bool max_charge_time_expired = charging_time >= CHARGE_FULL_TIME;
  charge_cutoff = prevent_charge || max_charge_time_expired;

  // Charger / Battery logic
  if (!shouldPowerRobot()) {
    NVIC_DisableIRQ(ADC1_IRQn);

    // Powered off, on charger (let charger power body)
    nCHG_PWR::reset();

    if (!delay_disable) {
      POWER_EN::pull(PULL_NONE);
      POWER_EN::mode(MODE_INPUT);
    } else {
      delay_disable = false;
    }

    low_power_count_up = 0;
    overheated = 0;
    heat_counter = 0;
    is_charging = false;
  } else if (!on_charger) {
    // Powered on, off charger
    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);

    nCHG_PWR::set();

    NVIC_DisableIRQ(ADC1_IRQn);

    delay_disable = true;
    is_charging = false;
  } else if (charge_cutoff) {
    // Battery disconnected, on charger (timeout)
    nCHG_PWR::reset();

    if (!delay_disable) {
      ADC1->ISR = ADC_ISR_AWD;
      NVIC_EnableIRQ(ADC1_IRQn);

      POWER_EN::pull(PULL_NONE);
      POWER_EN::mode(MODE_INPUT);
    } else {
      delay_disable = false;
    }

    // As long as the timer hasn't expired (and we're not manually disabling charging)
    // continue to report that we are charging.
    is_charging = !max_charge_time_expired && !disable_charger;
  } else {
    // Battery connected, on charger (charging)
    nCHG_PWR::reset();

    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);

    NVIC_DisableIRQ(ADC1_IRQn);

    delay_disable = true;
    is_charging = true;
    off_charger_time = 0;
  }
}

extern "C" void ADC1_IRQHandler(void) {
  POWER_EN::set();
  POWER_EN::mode(MODE_OUTPUT);
  POWER_EN::pull(PULL_UP);
  POWER_EN::mode(MODE_INPUT);

  ADC1->ISR = ADC_ISR_AWD;
}
