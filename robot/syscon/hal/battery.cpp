#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "uart.h"
#include "timer.h"

namespace
{
  // Updated to 3.0
  // XXX: No distinction 
  const u8 PIN_V_SENSE = 26;
  const u8 PIN_I_SENSE = 6;
  const u8 ANALOG_V_SENSE = ADC_CONFIG_PSEL_AnalogInput0;
  const u8 ANALOG_I_SENSE = ADC_CONFIG_PSEL_AnalogInput7;
  
  const u8 PIN_CHARGE_S1 = 9;
  //const u8 PIN_CHARGE_S2 = 14;
  const u8 PIN_CHARGE_HC = 29;
  const u8 PIN_CHARGE_EN = 5;
  
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
  //nrf_gpio_cfg_input(PIN_CHARGE_S2, NRF_GPIO_PIN_PULLUP);
  
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


void PowerInit()
{
  // Syscon power - this should always be on until battery fail
  const u8 PIN_VDD_EN = 3;             // 3.0
  nrf_gpio_pin_set(PIN_VDD_EN);        // On
  nrf_gpio_cfg_output(PIN_VDD_EN);
  
  // Motor and headboard power
  const u8 PIN_VBATs_EN = 12;
  nrf_gpio_pin_clear(PIN_VBATs_EN);    // Off
  nrf_gpio_cfg_output(PIN_VBATs_EN);
  
  // Encoder and headboard power
  const u8 PIN_VDDs_EN = 8;
  nrf_gpio_pin_set(PIN_VDDs_EN);      // Off
  nrf_gpio_cfg_output(PIN_VDDs_EN);
  
  // Let power drain out - 10ms is plenty long enough
  MicroWait(10000);
  
  // Bring up VBATs first, because camera requires 1V8 (via VBATs) before VDDs  
  nrf_gpio_pin_set(PIN_VBATs_EN);     // On
  MicroWait(1000);                    // Long enough for 1V8 to stabilize (datasheet says ~80uS)
  nrf_gpio_pin_clear(PIN_VDDs_EN);    // On  - XXX: VDDs is a PITA right now due to power sags
}
