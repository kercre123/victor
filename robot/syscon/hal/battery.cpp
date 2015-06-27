#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "uart.h"
#include "timer.h"
#include "anki/cozmo/robot/spineData.h"

#include "hardware.h"

namespace
{
  // Updated to 3.0
  const u32 V_REFERNCE_MV = 1200; // 1.2V Bandgap reference
  const u32 V_PRESCALE    = 3;
  const u32 V_SCALE       = 0x3ff; // 10 bit ADC
  const Fixed IBAT_SCALE  = TO_FIXED(0.91); // Cozmo 3 transducer scaling
  const Fixed VUSB_SCALE  = TO_FIXED(4.0); // Cozmo 3 voltage divider
  const Fixed VBAT_SCALE  = TO_FIXED(4.0); // Cozmo 3 voltage divider
  
  const Fixed VBAT_CHGD_HI_THRESHOLD = TO_FIXED(4.05); // V
  const Fixed VBAT_CHGD_LO_THRESHOLD = TO_FIXED(3.70); // V
  const Fixed VBAT_EMPTY_THRESHOLD   = TO_FIXED(2.90); // V
  const Fixed VUSB_DETECT_THRESHOLD  = TO_FIXED(4.40); // V
  
  const u8 ANALOG_I_SENSE     = ADC_CONFIG_PSEL_AnalogInput7;
  const u8 ANALOG_V_BAT_SENSE = ADC_CONFIG_PSEL_AnalogInput0;
  const u8 ANALOG_V_USB_SENSE = ADC_CONFIG_PSEL_AnalogInput1;
  
  // Read battery dead state N times before w e believe it is dead
  const u8 BATTERY_DEAD_CYCLES = 60;
  // Read charger contact state N times before we believe it changed
  const u8 CONTACT_DEBOUNCE_CYCLES = 30;
  
  // Are we currently on charge contacts?
  u8 m_onContacts = 0;

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

int IsOnContacts()
{
  return m_onContacts;
}

void BatteryInit()
{
  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_VDD_EN);        // On
  nrf_gpio_cfg_output(PIN_VDD_EN);
  
  // Motor and headboard power
  nrf_gpio_pin_clear(PIN_VBATs_EN);    // Off
  nrf_gpio_cfg_output(PIN_VBATs_EN);
  
  // Encoder and headboard power
  nrf_gpio_pin_set(PIN_VDDs_EN);       // Off
  nrf_gpio_cfg_output(PIN_VDDs_EN);
  
  // Initially set charge en to disable charging by default
  nrf_gpio_pin_clear(PIN_VUSBs_EN);
  nrf_gpio_cfg_output(PIN_VUSBs_EN);

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
  //g_dataToHead.chargeStat = XXX (we no longer have charge status as of v3.2)
  
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
        static u8 debounceBattery = 0;
        
        // Is battery dead AND we are not on the contacts
        if (!m_onContacts && g_dataToHead.VBat < VBAT_EMPTY_THRESHOLD)
        {
          if (debounceBattery < BATTERY_DEAD_CYCLES)
          {
            debounceBattery++;
          } else {
            // Shut everything down
            nrf_gpio_pin_clear(PIN_VBATs_EN);
            nrf_gpio_pin_clear(PIN_VDDs_EN);
            nrf_gpio_pin_clear(PIN_VDD_EN);
          }
        } else {
          debounceBattery = 0;
        }
        startADCsample(ANALOG_V_USB_SENSE);
        m_pinIndex = 2;
        break;
      }
      case 2:   // Charge contacts
      {
        g_dataToHead.Vusb = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VUSB_SCALE);

        // Are we on external power?
        u8 onContacts = (g_dataToHead.Vusb > VUSB_DETECT_THRESHOLD);
        static u8 debounceContacts = 0;

        // If contact state hasn't changed, do nothing
        if (onContacts == m_onContacts)
        {
          debounceContacts = 0;   // Reset debounce time
          
        // If contact state has changed, debounce it first
        } else if (debounceContacts < CONTACT_DEBOUNCE_CYCLES) {
            debounceContacts++;
          
        // If contact state has changed, and we are debounced, commit the change
        } else {
          debounceContacts = 0;   // Reset debounce time
         
          m_onContacts = onContacts;
          
          // If we are now on contacts, start charging
          if (m_onContacts)
          {
            // MUST disable VBATs (and thus head power) while charging, to protect battery
            nrf_gpio_pin_clear(PIN_VBATs_EN);   // Off
            nrf_gpio_pin_set(PIN_VDDs_EN);      // Off            
            nrf_gpio_pin_set(PIN_VUSBs_EN);  // Enable charging
            
          // If we are now off contacts, stop charging and reboot
          } else {
            nrf_gpio_pin_clear(PIN_VUSBs_EN);    // Disable charging
            PowerOn();
          }
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
  
  //g_dataToHead.chargeStat = XXX (we no longer have charge status as of v3.2)
  // Act on charge S1 if need be
}


void PowerOn()
{
  // Let power drain out - 10ms is plenty long enough
  MicroWait(10000);
  
  // Bring up VDDs first, because IMU requires VDDs before 1V8 (via VBATs)
  // WARNING:  This might affect camera and/or leakage into M4/Torpedo prior to 1V8
  nrf_gpio_pin_clear(PIN_VDDs_EN);    // On  - XXX: VDDs is a PITA right now due to power sags
  nrf_gpio_pin_set(PIN_VBATs_EN);     // On  - takes about 80uS to get to 1V8
}
