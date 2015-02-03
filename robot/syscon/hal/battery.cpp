#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "uart.h"
#include "timer.h"
#include "spiData.h"

namespace
{
  // Updated to 3.0
  const u32 V_REFERNCE_MV = 1200; // 1.2V Bandgap reference
  const u32 V_PRESCALE    = 3;
  const u32 V_SCALE       = 0x3ff; // 10 bit ADC
  const Fixed IBAT_SCALE  = TO_FIXED(0.91); // Cozmo 3 transducer scaling
  const Fixed VUSB_SCALE  = TO_FIXED(3.128); // Cozmo 3 voltage divider
  const Fixed VBAT_SCALE  = TO_FIXED(3.128); // Cozmo 3 voltage divider
  
  const Fixed VBAT_CHARGED_THRESHOLD = TO_FIXED(4.0); // V
  const Fixed VBAT_EMPTY_THRESHOLD   = TO_FIXED(3.4); // V
  const Fixed VUSB_DETECT_THRESHOLD  = TO_FIXED(4.4); // V
  
  const u8 PIN_I_SENSE     = 6;
  const u8 PIN_V_BAT_SENSE = 26;
  const u8 PIN_V_USB_SENSE = 27;
  const u8 ANALOG_I_SENSE     = ADC_CONFIG_PSEL_AnalogInput7;
  const u8 ANALOG_V_BAT_SENSE = ADC_CONFIG_PSEL_AnalogInput0;
  const u8 ANALOG_V_USB_SENSE = ADC_CONFIG_PSEL_AnalogInput1;
  
  const u8 PIN_CHARGE_S1 = 9;
  //const u8 PIN_CHARGE_S2 = 14;
  const u8 PIN_CHARGE_HC = 29;
  const u8 PIN_CHARGE_EN = 5;
  
  const u8 PIN_VDD_EN = 3;             // 3.0
  const u8 PIN_VBATs_EN = 12;
  const u8 PIN_VDDs_EN = 8;

  
  // Which pin is currently being used in the ADC mux
  u8 m_pinIndex;
}

extern GlobalDataToHead g_dataToHead;

static inline void startADCsample(u8 channel)
{
  static const u32 MUX_NONE = ~(0xff << ADC_CONFIG_PSEL_Pos);
  NRF_ADC->CONFIG &= MUX_NONE; // Clear any existing mux
  NRF_ADC->CONFIG |= ((u32)channel) << ADC_CONFIG_PSEL_Pos; // Activate analog input above
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
}

void BatteryInit()
{
  // Configure the charger status pins as inputs
  nrf_gpio_cfg_input(PIN_CHARGE_S1, NRF_GPIO_PIN_PULLUP);
  //nrf_gpio_cfg_input(PIN_CHARGE_S2, NRF_GPIO_PIN_PULLUP);
  
  // Initially set pins to disable high current
  nrf_gpio_pin_set(PIN_CHARGE_HC);
  nrf_gpio_cfg_output(PIN_CHARGE_HC);
  // Initially clear charge en to enable charging by default
  nrf_gpio_pin_clear(PIN_CHARGE_EN);
  nrf_gpio_cfg_output(PIN_CHARGE_EN);

  // Configure the analog sense pins
  nrf_gpio_cfg_input(PIN_I_SENSE, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_V_BAT_SENSE, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_V_USB_SENSE, NRF_GPIO_PIN_NOPULL);
    
  // Just in case we need to power on the peripheral ourselves
  NRF_ADC->POWER = 1;
  
  NRF_ADC->CONFIG =
    (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) | // 10 bit resolution
    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) | // External inputs with 1/3rd analog prescaling
    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) | // 1.2V Bandgap reference
    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_None); // Disable external analog reference pins
  
  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;
  
  // Start initial conversions for the volatge and current
  // Battery current
  startADCsample(ANALOG_I_SENSE);
  while (!NRF_ADC->EVENTS_END)  // Wait for the conversion to finish
  { /* spin */ }
  NRF_ADC->TASKS_STOP = 1;
  g_dataToHead.IBat = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), IBAT_SCALE);
  // Battery voltage
  startADCsample(ANALOG_V_BAT_SENSE);
  while (!NRF_ADC->EVENTS_END)  // Wait for the conversion to finish
  { /* spin */ }
  NRF_ADC->TASKS_STOP = 1;
  g_dataToHead.VBat = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VBAT_SCALE);
  // USB voltage
  startADCsample(ANALOG_V_USB_SENSE);
  while (!NRF_ADC->EVENTS_END)  // Wait for the conversion to finish
  { /* spin */ }
  NRF_ADC->TASKS_STOP = 1;
  g_dataToHead.Vusb = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VUSB_SCALE);
  
  // Sample charger pin
  g_dataToHead.chargeStat = nrf_gpio_pin_read(PIN_CHARGE_S1);
  
  m_pinIndex = 0; // Set ADC phase index to 0
  startADCsample(ANALOG_I_SENSE); // Start conversion
}

void BatteryUpdate()
{
  if (NRF_ADC->EVENTS_END)
  { // Have a new result
    switch (m_pinIndex)
    {
      case 0: // Battery current
      {
        g_dataToHead.IBat = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), IBAT_SCALE);
        startADCsample(ANALOG_V_BAT_SENSE);
        m_pinIndex = 1;
        break;
      }
      case 1: // Battery voltage
      {
        g_dataToHead.VBat = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VBAT_SCALE);
        if (g_dataToHead.VBat < VBAT_EMPTY_THRESHOLD) // Battery dead
        {
          // Shut everything down
          nrf_gpio_pin_clear(PIN_VBATs_EN);
          nrf_gpio_pin_clear(PIN_VDDs_EN);
          nrf_gpio_pin_clear(PIN_VDD_EN);
        }
        else if (g_dataToHead.VBat > VBAT_CHARGED_THRESHOLD) // Battery charged
        {
          nrf_gpio_pin_set(PIN_CHARGE_EN); // Disable charging
          // Enable power if it was shut down
          nrf_gpio_pin_clear(PIN_VBATs_EN);
          nrf_gpio_pin_clear(PIN_VDDs_EN);
          nrf_gpio_pin_clear(PIN_VDD_EN);
        }
        startADCsample(ANALOG_V_USB_SENSE);
        m_pinIndex = 2;
        break;
      }
      case 2:
      {
        g_dataToHead.Vusb = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VUSB_SCALE);
        if ((g_dataToHead.Vusb > VUSB_DETECT_THRESHOLD) && (g_dataToHead.VBat < VBAT_CHARGED_THRESHOLD))
        {
          nrf_gpio_pin_set(PIN_CHARGE_EN); // Enable charging
        }
        startADCsample(ANALOG_I_SENSE);
        m_pinIndex = 0;
        break;
      }
      default:
      {
        m_pinIndex = 0;
      }
    }
  }
  
  g_dataToHead.chargeStat = nrf_gpio_pin_read(PIN_CHARGE_S1);
  // Act on charge S1 if need be
}


void PowerInit()
{
  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_VDD_EN);        // On
  nrf_gpio_cfg_output(PIN_VDD_EN);
  
  // Motor and headboard power
  nrf_gpio_pin_clear(PIN_VBATs_EN);    // Off
  nrf_gpio_cfg_output(PIN_VBATs_EN);
  
  // Encoder and headboard power
  nrf_gpio_pin_set(PIN_VDDs_EN);      // Off
  nrf_gpio_cfg_output(PIN_VDDs_EN);
  
  // Let power drain out - 10ms is plenty long enough
  MicroWait(10000);
  
  // Bring up VBATs first, because camera requires 1V8 (via VBATs) before VDDs  
  nrf_gpio_pin_set(PIN_VBATs_EN);     // On
  MicroWait(1000);                    // Long enough for 1V8 to stabilize (datasheet says ~80uS)
  nrf_gpio_pin_clear(PIN_VDDs_EN);    // On  - XXX: VDDs is a PITA right now due to power sags
}
