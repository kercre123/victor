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
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL16
  ;

static const uint16_t LOW_VOLTAGE_POWER_DOWN_POINT = ADC_VOLTS(3.4);
static const int      LOW_VOLTAGE_POWER_DOWN_TIME = 200;  // 1s
static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.3);
static const uint16_t DISCHARGED_BATTERY = ADC_VOLTS(3.7);
static const uint32_t FALLING_EDGE = ADC_WINDOW(ADC_VOLTS(3.50), ~0);
static const int      MINIMUM_ON_CHARGER = 5;

static const uint16_t*  TEMP30_CAL_ADDR = (uint16_t*)0x1FFFF7B8;
static const int32_t    TEMP_VOLT_ADJ   = (int32_t)(0x100000 * (2.8 / 3.3));
static const int32_t    TEMP_SCALE_ADJ  = (int32_t)(0x100000 * (1.000 / 5.336));

// We allow for 4 hours of over-heat time (variable)
static const int MAX_HEAT_COUNTDOWN = 200 * 60 * 60 * 4 * 5;
static const int OVERHEAT_SHUTDOWN = 200 * 30;

static const int POWER_DOWN_TIME = 200 * 5.5;               // Shutdown
static const int POWER_WIPE_TIME = 200 * 12;                // Enter recovery mode
static const int MAX_CHARGE_TIME = 200 * 60 * 30;           // 30 minutes
static const int START_DISCHARGE = 200 * 60 * 60 * 24 * 3;  // 3 Days
static const int ON_CHARGER_RESET = 200 * 60;               // 1 Minute
static const int TOP_OFF_TIME    = 200 * 60 * 60 * 24 * 90; // 90 Days

static const int BOUNCE_LENGTH = 3;
static const int MINIMUM_RELEASE_UNSTUCK = 20;
static const int BUTTON_THRESHOLD = ADC_VOLTS(2.0) * 2;

static bool is_charging = false;
bool Analog::on_charger = false;
static bool charge_cutoff = false;
static bool too_hot = false;
static int heat_counter = 0;
static TemperatureAlarm temp_alarm = TEMP_ALARM_SAFE;

// Assume we started on the charger
static bool allow_power;

static int16_t temperature;
static bool disable_charger = false;
static int overheated = 0;

static uint16_t volatile adc_values[ADC_CHANNELS];
static bool button_pressed = false;

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
  data->battery.main_voltage = adc_values[ADC_VMAIN];
  data->battery.charger = adc_values[ADC_VEXT];
  data->battery.temperature = (int16_t) temperature;
  data->battery.flags = 0
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

  bool button_now = (adc_values[ADC_BUTTON] >= BUTTON_THRESHOLD);

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

static bool handleTemperature() {
  // Temperature logic
  int32_t temp_now = *TEMP30_CAL_ADDR - ((adc_values[ADC_TEMP] * TEMP_VOLT_ADJ) >> 20);
  temp_now = ((temp_now * TEMP_SCALE_ADJ) >> 20) + 30;

  // We are running way too hot, have a bowl of boot loops.
  if (temp_now >= 70) {
    Power::setMode(POWER_STOP);
    return true;
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
  if (temperature < 45) {
    temp_alarm = TEMP_ALARM_SAFE;
    if (heat_counter > 0) heat_counter--;
  } else {
    // Start processing our overheat alarms
    bool disable_vmain = false
      || alarmTimer<TEMP_ALARM_HOT, MAX_HEAT_COUNTDOWN>(temperature, 60) // Fire immediately
      || alarmTimer<TEMP_ALARM_MID,                 10>(temperature, 50) // Fire in ~2H
      || alarmTimer<TEMP_ALARM_LOW,                  5>(temperature, 45) // Fire in ~4H
      ;

    if (overheated == 0 && disable_vmain) {
      overheated = OVERHEAT_SHUTDOWN;
    }
  } 

  // NOTE: This counter cannot be reset until it fires
  if (overheated > 0 && --overheated == 0) {
    Power::setMode(POWER_STOP);
  }

  if (temperature >= 45) {
    too_hot = true;
  } else if (temperature <= 42) {
    too_hot = false;
  }

  return too_hot;
}

static void handleLowBattery() {
  if (Analog::on_charger) return ;

  // Low voltage shutdown
  static int power_down_timer = LOW_VOLTAGE_POWER_DOWN_TIME;
  if (adc_values[ADC_VMAIN] < LOW_VOLTAGE_POWER_DOWN_POINT) {
    if (--power_down_timer <= 0) {
      Power::setMode(POWER_STOP);
    }
  } else {
    power_down_timer = LOW_VOLTAGE_POWER_DOWN_TIME;
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
  bool vext_now = adc_values[ADC_VEXT] >= TRANSITION_POINT;

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
  static bool discharge_battery = false;

  debounceVEXT();
  handleButton();
  handleLowBattery();

  // Handle our temperature / contact time thresholds
  bool prevent_charge = handleTemperature() 
    || disable_charger;

  if (on_charger) {
    if (!prevent_charge) {
      if (++on_charger_time == START_DISCHARGE) {
        discharge_battery = true;
      } else if (on_charger_time >= TOP_OFF_TIME) {
        on_charger_time = 0;
      }
    }
  } else if (++off_charger_time >= ON_CHARGER_RESET) {
    on_charger_time = 0;
  }

  // 30 minute charge cut-off
  bool max_charge_time_expired = on_charger_time >= MAX_CHARGE_TIME;
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

    overheated = 0;
    heat_counter = 0;
    is_charging = false;
  } else if (!on_charger || discharge_battery) {
    // Powered on, off charger
    POWER_EN::pull(PULL_UP);
    POWER_EN::mode(MODE_INPUT);

    nCHG_PWR::set();

    NVIC_DisableIRQ(ADC1_IRQn);

    if (adc_values[ADC_VMAIN] <= DISCHARGED_BATTERY) {
      discharge_battery = false;
    }

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

    // As long as the 30 min timer hasn't expired (and we're not manually disabling charging)
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
