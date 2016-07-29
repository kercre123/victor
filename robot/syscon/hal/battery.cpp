#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "anki/cozmo/robot/spineData.h"
#include "messages.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

#include "hardware.h"
#include "rtos.h"

static const int MaxContactTime = 90000; // (30min) 20ms per count
static const int MinContactTime = 20;    // .10s

// Updated to 3.0
static const u32 V_REFERNCE_MV = 1200; // 1.2V Bandgap reference
static const u32 V_PRESCALE    = 3;
static const u32 V_SCALE       = 0x3ff; // 10 bit ADC

static const Fixed VEXT_SCALE  = TO_FIXED(2.0); // Cozmo 4.1 voltage divider
static const Fixed VBAT_SCALE  = TO_FIXED(4.0); // Cozmo 4.1 voltage divider

static const Fixed VBAT_CHGD_HI_THRESHOLD = TO_FIXED(4.05); // V
static const Fixed VBAT_CHGD_LO_THRESHOLD = TO_FIXED(3.45); // V

static const Fixed VEXT_DETECT_THRESHOLD  = TO_FIXED(4.40); // V
static const Fixed VEXT_CHARGE_THRESHOLD  = TO_FIXED(4.00); // V

static const u32 CLIFF_SENSOR_BLEED = 0;

static int ContactTime = 0;

void manage_adc(void*);

// Are we currently on charge contacts?
bool Battery::onContacts = false;

Fixed vBat, vExt;
static bool isCharging = false;
static bool disableCharge = true;

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

uint8_t Battery::getLevel(void) {
  return (vBat - VBAT_CHGD_LO_THRESHOLD) * 100 / (VBAT_CHGD_HI_THRESHOLD - VBAT_CHGD_LO_THRESHOLD);
}

static void SendPowerStateUpdate(void *userdata)
{
  using namespace Anki::Cozmo;

  PowerState msg;
  msg.VBatFixed = vBat;
  msg.VExtFixed = vExt;
  msg.batteryLevel = Battery::getLevel();
  msg.onCharger  = ContactTime > MinContactTime;
  msg.isCharging = isCharging;
  RobotInterface::SendMessage(msg);
}

void Battery::init()
{
  // Configure charge pins
  nrf_gpio_pin_clear(PIN_CHARGE_EN);
  nrf_gpio_cfg_output(PIN_CHARGE_EN);

  // Configure cliff sensor pins
  nrf_gpio_cfg_output(PIN_IR_DROP);

  // Syscon power - this should always be on until battery fail
  nrf_gpio_pin_set(PIN_PWR_EN);
  nrf_gpio_cfg_output(PIN_PWR_EN);

  // Encoder and LED power (enabled)
  nrf_gpio_pin_clear(PIN_VDDs_EN);
  nrf_gpio_cfg_output(PIN_VDDs_EN);

  // turn off headlight
  nrf_gpio_pin_clear(PIN_IR_FORWARD);
  nrf_gpio_cfg_output(PIN_IR_FORWARD);

  // Configure the analog sense pins
  nrf_gpio_cfg_input(PIN_V_BAT_SENSE, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(PIN_V_EXT_SENSE, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(PIN_CLIFF_SENSE, NRF_GPIO_PIN_NOPULL);

  // Just in case we need to power on the peripheral ourselves
  NRF_ADC->POWER = 1;

  NRF_ADC->CONFIG =
    (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) | // 10 bit resolution
    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) | // External inputs with 1/3rd analog prescaling
    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) | // 1.2V Bandgap reference
    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_None); // Disable external analog reference pins

  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

  vBat = getADCsample(ANALOG_V_BAT_SENSE, VBAT_SCALE); // Battery voltage
  vExt = getADCsample(ANALOG_V_EXT_SENSE, VEXT_SCALE); // External voltage
  int temp = getADCsample(ANALOG_CLIFF_SENSE, VEXT_SCALE);

  startADCsample(ANALOG_CLIFF_SENSE);

  /*
  NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;
  NVIC_EnableIRQ(ADC_IRQn);
  NVIC_SetPriority(ADC_IRQn, 3);
  */

  RTOS::schedule(manage_adc);
  RTOS::schedule(SendPowerStateUpdate, CYCLES_MS(60.0f));
}

void Battery::setHeadlight(bool status) {
  if (status) {
    nrf_gpio_pin_set(PIN_IR_FORWARD);
  } else {
    nrf_gpio_pin_clear(PIN_IR_FORWARD);
  }
}

void Battery::powerOn()
{
  disableCharge = false;
  nrf_gpio_pin_set(PIN_PWR_EN);
}

void Battery::powerOff()
{
  disableCharge = true;
  nrf_gpio_pin_clear(PIN_CHARGE_EN);
  nrf_gpio_pin_clear(PIN_PWR_EN);
  MicroWait(10000);
}

int resultLedOn;
int resultLedOff;
void manage_adc(void*)
{
  using namespace Battery;
  static const int LOW_BAT_TIME = CYCLES_MS(60*1000); // 1 minute

  if (!NRF_ADC->EVENTS_END) {
    return ;
  }

  switch (m_pinIndex)
  {
    case ANALOG_V_BAT_SENSE:
      {
        vBat = calcResult(VBAT_SCALE);

        // after 1 minute of low battery, turn off
        static int lowBatTimer = GetCounter() + LOW_BAT_TIME;

        if (vBat < VBAT_CHGD_LO_THRESHOLD) {
          int ticks_left = lowBatTimer - GetCounter();

          if (ticks_left < 0) {
            powerOff();
            NVIC_SystemReset();
          }
        } else {
          lowBatTimer = GetCounter() + LOW_BAT_TIME;
        }

        startADCsample(ANALOG_CLIFF_SENSE);

        break ;
      }
    case ANALOG_V_EXT_SENSE:
      {
        // Are our power pins shorted?
        static int ground_short = 0;

        if (NRF_ADC->RESULT < 0x30) {
          if (++ground_short > 30) {
            Battery::powerOff();
            NVIC_SystemReset();
          }
        } else {
          ground_short = 0;
        }

        vExt = calcResult(VEXT_SCALE);
        onContacts = vExt > VEXT_DETECT_THRESHOLD || (isCharging && vExt > VEXT_CHARGE_THRESHOLD);

        if (!onContacts) {
          ContactTime = 0;
        } else {
          ContactTime++;
        }

        if (!disableCharge && ContactTime < MaxContactTime && ContactTime > MinContactTime) {
          nrf_gpio_pin_set(PIN_CHARGE_EN);
          isCharging = true;
        } else {
          nrf_gpio_pin_clear(PIN_CHARGE_EN);
          isCharging = false;
        }

        startADCsample(ANALOG_CLIFF_SENSE);
        break ;
      }
    case ANALOG_CLIFF_SENSE:
      {
        static const uint32_t PIN_IR_DROP_MASK = 1 << PIN_IR_DROP;

        if (NRF_GPIO->OUT & PIN_IR_DROP_MASK) {
          resultLedOn = NRF_ADC->RESULT;
          startADCsample(ANALOG_V_BAT_SENSE);
        } else {
          resultLedOff = NRF_ADC->RESULT;
          if (*FIXTURE_HOOK == 0xDEADFACE)
            startADCsample(ANALOG_V_BAT_SENSE);
          else
            startADCsample(ANALOG_V_EXT_SENSE);

        }

        int result = resultLedOn - resultLedOff - CLIFF_SENSOR_BLEED;
        g_dataToHead.cliffLevel = (result < 0) ? 0 : result;

        nrf_gpio_pin_toggle(PIN_IR_DROP);

        break ;
      }
  }
}
