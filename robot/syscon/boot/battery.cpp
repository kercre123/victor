#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "../../hal/hardware.h"
#include "battery.h"

static inline void startADCsample(AnalogInput channel)
{
  NRF_ADC->CONFIG &= ~ADC_CONFIG_PSEL_Msk; // Clear any existing mux
  NRF_ADC->CONFIG |= channel << ADC_CONFIG_PSEL_Pos; // Activate analog input above
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
}

void Battery::init()
{
  // Enable charger
  nrf_gpio_pin_set(PIN_CHARGE_EN);
  nrf_gpio_cfg_output(PIN_CHARGE_EN);
 
  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_PWR_EN);
  nrf_gpio_cfg_output(PIN_PWR_EN);

  // Encoder and headboard power
  nrf_gpio_pin_set(PIN_VDDs_EN);
  nrf_gpio_cfg_output(PIN_VDDs_EN);

  // Configure the analog sense pins
  nrf_gpio_cfg_input(PIN_V_EXT_SENSE, NRF_GPIO_PIN_PULLUP);

  // Just in case we need to power on the peripheral ourselves
  NRF_ADC->POWER = 1;

  NRF_ADC->CONFIG =
    (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) | // 10 bit resolution
    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) | // External inputs with 1/3rd analog prescaling
    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) | // 1.2V Bandgap reference
    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_None); // Disable external analog reference pins

  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

  startADCsample(ANALOG_V_EXT_SENSE);
}

void Battery::manage(void) {
  if (!NRF_ADC->EVENTS_END) return ;

  static int ground_short = 0;
  uint32_t raw = NRF_ADC->RESULT;

  if (raw < 0x30) {
    // Wait a full 5 seconds in recovery mode before rebooting
    if (ground_short++ >= 5) {
      NVIC_SystemReset();
    }
  } else {
    ground_short = 0;
  }

  startADCsample(ANALOG_V_EXT_SENSE);
}

