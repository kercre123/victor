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

#ifdef ROBOT41
const Fixed VEXT_SCALE  = TO_FIXED(2.0); // Cozmo 4.1 voltage divider
const Fixed VBAT_SCALE  = TO_FIXED(4.0); // Cozmo 4.1 voltage divider
#else
const Fixed IBAT_SCALE  = TO_FIXED(0.91); // Cozmo 3 transducer scaling
const Fixed VUSB_SCALE  = TO_FIXED(4.0); // Cozmo 3 voltage divider
const Fixed VBAT_SCALE  = TO_FIXED(4.0); // Cozmo 3 voltage divider
#endif

const Fixed VBAT_CHGD_HI_THRESHOLD = TO_FIXED(4.05); // V
const Fixed VBAT_CHGD_LO_THRESHOLD = TO_FIXED(3.70); // V
const Fixed VBAT_EMPTY_THRESHOLD   = TO_FIXED(2.90); // V

#ifdef ROBOT41
const Fixed VEXT_DETECT_THRESHOLD  = TO_FIXED(4.40); // V
#else
const Fixed VUSB_DETECT_THRESHOLD  = TO_FIXED(4.40); // V
#endif

// Read battery dead state N times before we believe it is dead
const u8 BATTERY_DEAD_CYCLES = 60;
// Read charger contact state N times before we believe it changed
const u8 CONTACT_DEBOUNCE_CYCLES = 30;

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
#ifdef ROBOT41
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
#else
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
#endif

  // Just in case we need to power on the peripheral ourselves
  NRF_ADC->POWER = 1;

  NRF_ADC->CONFIG =
    (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) | // 10 bit resolution
    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) | // External inputs with 1/3rd analog prescaling
    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) | // 1.2V Bandgap reference
    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_None); // Disable external analog reference pins

  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

#ifdef ROBOT41
  g_dataToHead.VBat = getADCsample(ANALOG_V_BAT_SENSE, VBAT_SCALE); // Battery voltage
  g_dataToHead.VExt = getADCsample(ANALOG_V_EXT_SENSE, VEXT_SCALE); // External voltage

  startADCsample(ANALOG_V_BAT_SENSE);
#else
  g_dataToHead.IBat = getADCsample(ANALOG_I_BAT_SENSE, IBAT_SCALE); // Battery current
  g_dataToHead.VBat = getADCsample(ANALOG_V_BAT_SENSE, VBAT_SCALE); // Battery voltage
  g_dataToHead.Vusb = getADCsample(ANALOG_V_USB_SENSE, VUSB_SCALE); // External voltage

  startADCsample(ANALOG_I_BAT_SENSE);
#endif
}

void Battery::powerOn()
{
  // Let power drain out - 10ms is plenty long enough
  MicroWait(10000);

#ifdef ROBOT41
  nrf_gpio_pin_clear(PIN_nVDDs_EN);
#else 
  // Bring up VDDs first, because IMU requires VDDs before 1V8 (via VBATs)
  // WARNING:  This might affect camera and/or leakage into M4/Torpedo prior to 1V8
  nrf_gpio_pin_clear(PIN_VDDs_EN);    // On  - XXX: VDDs is a PITA right now due to power sags
  nrf_gpio_pin_set(PIN_VBATs_EN);     // On  - takes about 80uS to get to 1V8
#endif
}

void Battery::powerOff()
{
#ifdef ROBOT41
  // Shutdown the extra things
  MicroWait(10000);
  nrf_gpio_pin_set(PIN_nVDDs_EN);

  MicroWait(10000);
  nrf_gpio_pin_clear(PIN_PWR_EN);
#endif
}

void Battery::update()
{
  if (!NRF_ADC->EVENTS_END) {
    return ;
  }

#ifndef ROBOT41
  static u8 debounceBattery = 0;
#endif

  switch (m_pinIndex)
  {
#ifdef ROBOT41
    case ANALOG_V_BAT_SENSE:
      g_dataToHead.VBat = calcResult(VBAT_SCALE);
      startADCsample(ANALOG_V_EXT_SENSE);
      break ;

    case ANALOG_V_EXT_SENSE:
      g_dataToHead.VExt = calcResult(VEXT_SCALE);  
      startADCsample(ANALOG_V_BAT_SENSE);
      break ;
#else
    case ANALOG_I_BAT_SENSE: // Battery current
      g_dataToHead.IBat = calcResult(IBAT_SCALE);
      startADCsample(ANALOG_V_BAT_SENSE);
      break;

    case ANALOG_V_BAT_SENSE: // Battery voltage
      g_dataToHead.VBat = calcResult(VBAT_SCALE);
      
      // Is battery dead AND we are not on the contacts
      if (!Battery::onContacts && g_dataToHead.VBat < VBAT_EMPTY_THRESHOLD)
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

    case ANALOG_V_USB_SENSE:   // Charge contacts
      g_dataToHead.Vusb = calcResult(VUSB_SCALE);

      // Are we on external power?
      u8 onContacts = (g_dataToHead.Vusb > VUSB_DETECT_THRESHOLD);
      static u8 debounceContacts = 0;

      // If contact state hasn't changed, do nothing
      if (onContacts == Battery::onContacts)
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
        if (Battery::onContacts)
        {
          // MUST disable VBATs (and thus head power) while charging, to protect battery
          nrf_gpio_pin_clear(PIN_VBATs_EN);   // Off
          nrf_gpio_pin_set(PIN_VDDs_EN);      // Off            
          nrf_gpio_pin_set(PIN_VUSBs_EN);  // Enable charging
          
        // If we are now off contacts, stop charging and reboot
        } else {
          nrf_gpio_pin_clear(PIN_VUSBs_EN);    // Disable charging
          powerOn();
        }
      }
      startADCsample(ANALOG_I_BAT_SENSE);
      break;
#endif
  }
}
