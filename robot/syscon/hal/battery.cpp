#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "uart.h"

namespace
{
  const u8 PIN_V_SENSE = 5;
  const u8 PIN_I_SENSE = 6;
  const u8 ANALOG_V_SENSE = ADC_CONFIG_PSEL_AnalogInput6;
  const u8 ANALOG_I_SENSE = ADC_CONFIG_PSEL_AnalogInput7;
  
  const u8 PIN_CHARGE_S1 = 13;
  const u8 PIN_CHARGE_S2 = 14;
  const u8 PIN_CHARGE_HC = 15;
  const u8 PIN_CHARGE_EN = 16;
  
  const u8 ANALOG_PINS[2] =
  {
    ANALOG_V_SENSE,
    ANALOG_I_SENSE
  };
  
  // Which pin is currently being used in the ADC mux
  u8 m_pinIndex;
  
  bool m_isConverting;
}

void BatteryInit()
{
  // Configure the charger status pins as inputs
  nrf_gpio_cfg_input(PIN_CHARGE_S1, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(PIN_CHARGE_S2, NRF_GPIO_PIN_PULLUP);
  
  // Let the charge enable lines float high from external pullups
  nrf_gpio_cfg_input(PIN_CHARGE_HC, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_CHARGE_EN, NRF_GPIO_PIN_NOPULL);
  
  // Configure the analog sense pins
  nrf_gpio_cfg_input(PIN_V_SENSE, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_I_SENSE, NRF_GPIO_PIN_NOPULL);
  
  m_pinIndex = 0;
  
  // Just in case we need to power on the peripheral ourselves
  NRF_ADC->POWER = 1;
  
  NRF_ADC->CONFIG =
    (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) |
    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) |
    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) |
    (ANALOG_PINS[1] << ADC_CONFIG_PSEL_Pos) |
    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_None);
  
  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;
  
  // Start initial conversions for the volatge and current
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
  while (!NRF_ADC->EVENTS_END)  // Wait for the conversion to finish
    ;
  NRF_ADC->EVENTS_END = 0;
  
  u32 result = NRF_ADC->RESULT; //((NRF_ADC->RESULT * 7) / 0x3FF) * 3;
  
  NRF_ADC->TASKS_STOP = 1;
}

void BatteryUpdate()
{
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
  while (!NRF_ADC->EVENTS_END)  // Wait for the conversion to finish
    ;
  NRF_ADC->EVENTS_END = 0;
  
  // TODO: This only works for V-SENSE
  // 1200mV = VBG ref, *3 = OneThirdPrescale, *2 = Voltage Divider is 50/50 (One Half)
  u32 result = ((NRF_ADC->RESULT * 1200 * 3 * 2) / 0x3FF);
  
  NRF_ADC->TASKS_STOP = 1;
}
