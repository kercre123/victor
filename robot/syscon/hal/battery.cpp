#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "debug.h"
#include "timer.h"
#include "anki/cozmo/robot/spineData.h"

#include "hardware.h"

// Updated to 3.0
const u32 V_REFERNCE_MV = 1200; // 1.2V Bandgap reference
const u32 V_PRESCALE    = 3;
const u32 V_SCALE       = 0x3ff; // 10 bit ADC

const Fixed VEXT_SCALE  = TO_FIXED(2.0); // Cozmo 4.1 voltage divider
const Fixed VBAT_SCALE  = TO_FIXED(4.0); // Cozmo 4.1 voltage divider

/*
const Fixed VBAT_CHGD_HI_THRESHOLD = TO_FIXED(4.05); // V
const Fixed VBAT_CHGD_LO_THRESHOLD = TO_FIXED(3.70); // V
const Fixed VBAT_EMPTY_THRESHOLD   = TO_FIXED(2.90); // V
const Fixed VUSB_DETECT_THRESHOLD  = TO_FIXED(4.40); // V

// Read battery dead state N times before w e believe it is dead
const u8 BATTERY_DEAD_CYCLES = 60;
// Read charger contact state N times before we believe it changed
const u8 CONTACT_DEBOUNCE_CYCLES = 30;
*/

// Are we currently on charge contacts?
bool Battery::onContacts = false;

// Which pin is currently being used in the ADC mux
AnalogInput m_pinIndex;

extern GlobalDataToHead g_dataToHead;

static inline void startADCsample(AnalogInput channel)
{
  m_pinIndex = channel; // This is super cheap
  
  NRF_ADC->CONFIG &= ~ADC_CONFIG_PSEL_Msk; // Clear any existing mux
  NRF_ADC->CONFIG |= channel << ADC_CONFIG_PSEL_Pos; // Activate analog input above
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
}

static inline Fixed calcResult(const Fixed scale)
{
  return FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), scale);
}

static inline Fixed getADCsample(AnalogInput channel, const Fixed scale)
{
  startADCsample(channel);
  while (!NRF_ADC->EVENTS_END) ; // Wait for the conversion to finish
  NRF_ADC->TASKS_STOP = 1;
  return calcResult(scale);
}

void Battery::init()
{
  // Configure charge pins
  nrf_gpio_pin_set(PIN_CHARGE_EN);
  nrf_gpio_cfg_output(PIN_CHARGE_EN);
 
  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_PWR_EN);
  nrf_gpio_cfg_output(PIN_PWR_EN);
  
  // Encoder and headboard power
  nrf_gpio_pin_set(PIN_nVDDs_EN);
  nrf_gpio_cfg_output(PIN_nVDDs_EN);
  
  // Configure the analog sense pins
  nrf_gpio_cfg_input(PIN_V_BAT_SENSE, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_V_EXT_SENSE, NRF_GPIO_PIN_NOPULL);

  // Just in case we need to power on the peripheral ourselves
  NRF_ADC->POWER = 1;

  NRF_ADC->CONFIG =
    (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) | // 10 bit resolution
    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) | // External inputs with 1/3rd analog prescaling
    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) | // 1.2V Bandgap reference
    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_None); // Disable external analog reference pins

  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

  g_dataToHead.VBat = getADCsample(ANALOG_V_BAT_SENSE, VBAT_SCALE); // Battery voltage
  g_dataToHead.VExt = getADCsample(ANALOG_V_EXT_SENSE, VEXT_SCALE); // External voltage

  startADCsample(ANALOG_V_BAT_SENSE);
}

void Battery::powerOn()
{
  // Let power drain out - 10ms is plenty long enough
  MicroWait(10000);
  
  nrf_gpio_pin_clear(PIN_nVDDs_EN);
}

void Battery::update()
{
  if (!NRF_ADC->EVENTS_END) {
    return ;
  }

  switch (m_pinIndex)
  {
    case ANALOG_V_BAT_SENSE:
      g_dataToHead.VBat = calcResult(VBAT_SCALE);
      startADCsample(ANALOG_V_EXT_SENSE);

      // todo: LOGIC HERE
      break ;
    case ANALOG_V_EXT_SENSE:
      g_dataToHead.VExt = calcResult(VEXT_SCALE);
      startADCsample(ANALOG_V_BAT_SENSE);
    
      // todo: LOGIC HERE
      break ;
  }

  /*
  // Have a new result
  switch (m_pinIndex)
  {
    case ANALOG_I_SENSE: // Battery current
    {
      g_dataToHead.IBat = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), IBAT_SCALE);
      startADCsample(ANALOG_V_BAT_SENSE);
      break;
    }
    case ANALOG_V_BAT_SENSE: // Battery voltage
    {
      g_dataToHead.VBat = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VBAT_SCALE);
      static u8 debounceBattery = 0;
      
      // Is battery dead AND we are not on the contacts
      if (!onContacts && g_dataToHead.VBat < VBAT_EMPTY_THRESHOLD)
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
      break;
    }
    case ANALOG_V_USB_SENSE:   // Charge contacts
    {
      g_dataToHead.Vusb = FIXED_MUL(FIXED_DIV(TO_FIXED(NRF_ADC->RESULT * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), VUSB_SCALE);

      // Are we on external power?
      bool onContacts = (g_dataToHead.Vusb > VUSB_DETECT_THRESHOLD);
      static u8 debounceContacts = 0;

      // If contact state hasn't changed, do nothing
      if (onContacts == onContacts)
      {
        debounceContacts = 0;   // Reset debounce time
        
      // If contact state has changed, debounce it first
      } else if (debounceContacts < CONTACT_DEBOUNCE_CYCLES) {
          debounceContacts++;
        
      // If contact state has changed, and we are debounced, commit the change
      } else {
        debounceContacts = 0;   // Reset debounce time
       
        Battery::onContacts = onContacts;
        
        // If we are now on contacts, start charging
        if (onContacts)
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
      break;
    }
  }
  */
}
