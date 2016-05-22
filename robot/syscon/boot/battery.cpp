#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "hal/hardware.h"
#include "battery.h"

static inline void startADCsample(AnalogInput channel)
{
  NRF_ADC->CONFIG &= ~ADC_CONFIG_PSEL_Msk; // Clear any existing mux
  NRF_ADC->CONFIG |= channel << ADC_CONFIG_PSEL_Pos; // Activate analog input above
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
}

static inline void kickDog() {
  for (int channel = 0; channel < 8; channel++) {
    NRF_WDT->RR[channel] = WDT_RR_RR_Reload;
  }
}

void Battery::init()
{
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

  // Wait for cozmo to sit on a charger
  while (Battery::read() < VEXT_CONTACT_LEVEL) ;
}

void Battery::powerOn() {
  // Disable charger during boot
  nrf_gpio_pin_clear(PIN_CHARGE_EN);
  nrf_gpio_cfg_output(PIN_CHARGE_EN);
 
  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_PWR_EN);
  nrf_gpio_cfg_output(PIN_PWR_EN);

  // Encoder and headboard power
  nrf_gpio_pin_set(PIN_VDDs_EN);
  nrf_gpio_cfg_output(PIN_VDDs_EN);
}

int32_t Battery::read(void) {
  while (!NRF_ADC->EVENTS_END) ;
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_STOP = 1;
  
  uint32_t value = NRF_ADC->RESULT;

  startADCsample(ANALOG_V_EXT_SENSE);

  return value;
}

void Battery::manage(void) {
  static int ground_short = 0;
  
  kickDog();

  int32_t value = read();
    
  if (value < 0x30) {
    if (ground_short++ >= 0x4000) {
      NVIC_SystemReset();
    }
  } else {
    ground_short = 0;
  }
}
