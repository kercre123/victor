#include "battery.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "random.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"
#include "messages.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "hardware.h"

#include "lights.h"
#include "backpack.h"
#include "bluetooth.h"
#include "cubes.h"
#include "backpack.h"
#include "motors.h"
#include "head.h"
#include "ota.h"
#include "dtm.h"

static const int MaxContactTime     = 30 * 60 * (1000 / 20);  // (30min) 20ms per count
static const int MaxChargedTime     = MaxContactTime * 8; // 4 hours
static const int MinContactTime     = 25;     // .5s
static const int MinChargeTime      = 250;    // Should charge for at least 5 seconds
static const int MaxChargeBounces   = 5;      // If we toggle 5 times too quickly, call charger out of spec
static const int OffOOSChargerReset = 250;    // 5 seconds

// Updated to 3.0
static const u32 V_REFERNCE_MV = 1200; // 1.2V Bandgap reference
static const u32 V_PRESCALE    = 3;
static const u32 V_SCALE       = 0x3ff; // 10 bit ADC

static const Fixed VEXT_SCALE  = TO_FIXED(2.0); // Cozmo 4.1 voltage divider
static const Fixed VBAT_SCALE  = TO_FIXED(4.0); // Cozmo 4.1 voltage divider

static const Fixed VBAT_CHGD_HI_THRESHOLD = TO_FIXED(4.05); // V
static const Fixed VBAT_CHGD_LO_THRESHOLD = TO_FIXED(3.45); // V
static const Fixed VBAT_LOW_THRESHOLD     = TO_FIXED(3.50); // V

static const Fixed VEXT_DETECT_THRESHOLD  = TO_FIXED(4.40); // V
static const Fixed VEXT_CHARGE_THRESHOLD  = TO_FIXED(4.00); // V

static const u32 CLIFF_SENSOR_BLEED = 0;

// Are we currently on charge contacts?
bool Battery::onContacts = false;

Fixed vBat;
Fixed vExt;
int resultLedOn;
int resultLedOff;
#ifdef FACTORY
u32 g_powerOffTime = (4<<23);   // 4 seconds from power-on
bool g_turnPowerOff = false;
Fixed vBatFiltered=0, vExtFiltered=0;
#endif

static bool isCharging = false;
       bool disableCharge = true;
static int ContactTime = 0;

static Anki::Cozmo::BodyRadioMode current_operating_mode = -1;
static Anki::Cozmo::BodyRadioMode active_operating_mode = -1;

static CurrentChargeState charge_state = CHARGE_OFF_CHARGER;

// Which pin is currently being used in the ADC mux
AnalogInput m_pinIndex;

extern GlobalDataToHead g_dataToHead;
extern bool motorOverride;

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
  int sample = NRF_ADC->RESULT;

  // Calibrate the ADC value if hardware revision is 3 or higher.
  int rev = (*((uint32_t volatile *)0xF0000FE8)) & 0x000000F0;
  if (rev > 0x70) {
    const int8_t GAIN_OFFSET = *(const int8_t*) 0x10000024;
    const int8_t GAIN_ERROR  = *(const int8_t*) 0x10000025;

    sample = sample * (1024 + GAIN_ERROR) / 1024 + GAIN_OFFSET;
  }

  return FIXED_MUL(FIXED_DIV(TO_FIXED(sample * V_REFERNCE_MV * V_PRESCALE / V_SCALE), TO_FIXED(1000)), scale);
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

static inline void sendPowerStateUpdate()
{
  using namespace Anki::Cozmo;

  // We don't need to flood the head with power state updates on sync
  if (!Head::synced) {
    return ;
  }

  PowerState msg;
  msg.operatingMode = active_operating_mode;
  msg.VBatFixed = vBat;
  msg.VExtFixed = vExt;
  msg.batteryLevel = Battery::getLevel();
  msg.onCharger  = ContactTime > MinContactTime;
  msg.isCharging = isCharging;
  msg.chargerOOS = charge_state == CHARGE_CHARGER_OUT_OF_SPEC;
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
  Battery::powerOn();
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

  #ifdef FACTORY
  g_turnPowerOff = true;
  #endif
}

void Battery::hookButton(bool button_pressed) {
  static int button_count = 0;

  if (button_pressed) {
    // 4 seconds (thanks to backpack divider)
    if (++button_count >= 200) {
      Battery::powerOff();
      setOperatingMode(BODY_LOW_POWER_OPERATING_MODE);
    } else if (current_operating_mode == BODY_LOW_POWER_OPERATING_MODE) {
      setOperatingMode(BODY_BLUETOOTH_OPERATING_MODE);
    }
  } else {
    button_count = 0;
  }
}

void Battery::setOperatingMode(Anki::Cozmo::BodyRadioMode mode) {
  current_operating_mode = mode;
}

void Battery::updateOperatingMode() {
  using namespace Anki::Cozmo;

  #ifdef FACTORY
  if (g_turnPowerOff && (int)(g_powerOffTime - GetCounter()) < 0 ) {
    Battery::powerOff();
    g_turnPowerOff = false;
  }
  #endif

  if (active_operating_mode == current_operating_mode) {
    return ;
  }

  // Tear down existing mode
  switch (active_operating_mode) {
    case BODY_LOW_POWER_OPERATING_MODE:
      nrf_gpio_pin_clear(PIN_VDDs_EN);
      Bluetooth::shutdown();

      //Head::leaveLowPowerMode();
      current_operating_mode = BODY_OTA_MODE;
      break ;

    case BODY_BLUETOOTH_OPERATING_MODE:
      Backpack::detachTimer();
      Bluetooth::shutdown();
      break ;

    case BODY_ACCESSORY_OPERATING_MODE:
      Backpack::setLayer(BPL_IMPULSE);
      Backpack::detachTimer();
      Radio::shutdown();
      break ;

    case BODY_STARTUP:
      motorOverride = false;
      break ;

    case BODY_DTM_OPERATING_MODE:
      #ifdef FACTORY
      Head::enableFixtureComms(true);
      #endif
      DTM::stop();
      break ;

    default:
      Bluetooth::shutdown();
      break ;
  }

  // Setup new mode
  switch(current_operating_mode) {
    case BODY_FORCE_RECOVERY:
      // This will destroy the first sector in the application layer
      // ... it is also a creative hack to work around a bug in cboot

      __disable_irq();

      NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
      while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
      NRF_NVMC->ERASEPAGE = 0x18000;
      while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
      NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
      while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;

      NVIC_SystemReset();
      break ;

    case BODY_BATTERY_CHARGE_TEST_MODE:
      nrf_gpio_pin_set(PIN_VDDs_EN);
      Motors::disable(true);
      Head::enterLowPowerMode();

      // Spinout for ~15 seconds
      MicroWait(15000000);

      nrf_gpio_pin_clear(PIN_PWR_EN);

      // Spinout for ~20 minutes
      for (int i = 0; i < 20 * 60 * 1000; i++) {
        MicroWait(1000);
      }

      Battery::powerOff();
      break ;

    #ifdef FACTORY
    case BODY_DTM_OPERATING_MODE:
      Backpack::clearLights(BPL_USER);
      Backpack::setLayer(BPL_USER);

      Motors::disable(true);
      Backpack::useTimer();
      DTM::start();

      g_turnPowerOff = false;

      #ifdef FACTORY
      Head::enableFixtureComms(false);
      #endif
      break ;
    #endif

    case BODY_OTA_MODE:
      // NOTE: DMA Devices (RADIO, SPI) and SD MUST be disabled
      // for this to work properly.  Do not reenable IRQs from
      // this point forward or you could murder all the cozmos

      __disable_irq();

      NVIC_DisableIRQ(TIMER1_IRQn);
      NVIC_DisableIRQ(UART0_IRQn);
      NVIC_DisableIRQ(GPIOTE_IRQn);
      NVIC_DisableIRQ(RTC1_IRQn);
      NVIC_DisableIRQ(SWI2_IRQn);

      Motors::disable(true);

      NRF_RTC0->TASKS_STOP = 1;
      NRF_TIMER0->TASKS_STOP = 1;
      NRF_TIMER1->TASKS_STOP = 1;
      NRF_TIMER2->TASKS_STOP = 1;

      // Disable charger during OTA to prevent battery damage
      nrf_gpio_pin_clear(PIN_CHARGE_EN);

      // Disconnect charge contact comms
      NRF_GPIO->PIN_CNF[PIN_TX_VEXT] = GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos;

      EnterOTA();

      break ;

    case BODY_LOW_POWER_OPERATING_MODE:
      #ifdef FACTORY
      g_turnPowerOff = true;
      #endif

      // Turn off encoders
      nrf_gpio_pin_set(PIN_VDDs_EN);
      Lights::setHeadlight(false);
      Head::enterLowPowerMode();
      Motors::disable(true);

      NVIC_DisableIRQ(UART0_IRQn);
      NVIC_DisableIRQ(GPIOTE_IRQn);
      NVIC_DisableIRQ(RTC1_IRQn);

      Bluetooth::advertise();

      NVIC_EnableIRQ(UART0_IRQn);
      NVIC_EnableIRQ(GPIOTE_IRQn);
      NVIC_EnableIRQ(RTC1_IRQn);
      break ;

    case BODY_BLUETOOTH_OPERATING_MODE:
      Motors::disable(true);

      NVIC_DisableIRQ(UART0_IRQn);
      NVIC_DisableIRQ(GPIOTE_IRQn);
      NVIC_DisableIRQ(RTC1_IRQn);

      Bluetooth::advertise();

      NVIC_EnableIRQ(UART0_IRQn);
      NVIC_EnableIRQ(GPIOTE_IRQn);
      NVIC_EnableIRQ(RTC1_IRQn);

      Backpack::useTimer();

      break ;

    case BODY_STARTUP:
      motorOverride = true;

      Motors::disable(false);

      for (int i = 0; i < MOTOR_COUNT; i++) {
        Motors::setPower(i, 0);
      }

      Motors::setPower(MOTOR_HEAD, 0x2800);
      break ;

    case BODY_ACCESSORY_OPERATING_MODE:
      Motors::disable(false);

      Backpack::clearLights(BPL_USER);
      Backpack::setLayer(BPL_USER);

      Radio::advertise();

      Backpack::useTimer();
      break ;

    default:
      Battery::powerOff();
      NVIC_SystemReset();
      break ;
  }

  active_operating_mode = current_operating_mode;
}

void Battery::powerOn()
{
  disableCharge = false;
  nrf_gpio_pin_set(PIN_PWR_EN);
}

void Battery::powerOff()
{
  disableCharge = true;
  // Shutdown the extra things
  nrf_gpio_pin_clear(PIN_PWR_EN);
  MicroWait(10000);
}

static void setChargeState(CurrentChargeState state) {
  if (state == charge_state) {
    return ;
  }

  charge_state = state;

  Backpack::setChargeState(state);

  switch(state) {
    case CHARGE_OFF_CHARGER:
      isCharging = false;
      nrf_gpio_pin_clear(PIN_CHARGE_EN);
      break ;
    case CHARGE_CHARGING:
      isCharging = true;
      nrf_gpio_pin_set(PIN_CHARGE_EN);
      break ;
    case CHARGE_CHARGED:
      isCharging = false;
      nrf_gpio_pin_clear(PIN_CHARGE_EN);
      break ;
    case CHARGE_CHARGER_OUT_OF_SPEC:
      isCharging = false;
      nrf_gpio_pin_clear(PIN_CHARGE_EN);
      break ;
  }
}

static void updateChargeState(Fixed vext) {
  using namespace Battery;

  static int chargeBounces = 0;

  // Check that we are at safe charging thresholds
  onContacts = vExt > VEXT_DETECT_THRESHOLD || (isCharging && vExt > VEXT_CHARGE_THRESHOLD);

  // Measure the amount of time we are on the charger
  if (!onContacts) {
    ContactTime = 0;
  } else {
    if (++ContactTime == MinContactTime) {
      // Wake if we hopped on the charger
      if (current_operating_mode == BODY_LOW_POWER_OPERATING_MODE) {
        setOperatingMode(BODY_BLUETOOTH_OPERATING_MODE);
      }
    }
  }

  switch (charge_state) {
    case CHARGE_OFF_CHARGER:
      if (!disableCharge && ContactTime >= MinContactTime) {
        setChargeState(CHARGE_CHARGING);
      }
      break ;

    case CHARGE_CHARGING:
      if (disableCharge) {
        setChargeState(CHARGE_OFF_CHARGER);
        break ;
      }

      // Charger says we are done, or we've been charging for 30 minutes
      if ((Motors::getChargeOkay() && ContactTime >= MinChargeTime) || ContactTime >= MaxContactTime) {
        setChargeState(CHARGE_CHARGED);
        break ;
      }

      // We are no longer on contacts
      if (!onContacts) {
        // Lock up charger if we are bouncing
        if (++chargeBounces > MaxChargeBounces) {
          setChargeState(CHARGE_CHARGER_OUT_OF_SPEC);
        } else {
          setChargeState(CHARGE_OFF_CHARGER);
        }

        break ;
      }

      // We appear to have a stable charge time (clear bounce count)
      if (ContactTime > MinChargeTime) {
        chargeBounces = 0;
      }

      break ;
    case CHARGE_CHARGED:
      // Every 4 hours, reenable the charger
      if (ContactTime > MaxChargedTime) {
        ContactTime = MinContactTime;
        setChargeState(CHARGE_CHARGING);
      }

      // We have left the charger
      if (!onContacts) {
        setChargeState(CHARGE_OFF_CHARGER);
        return ;
      }

      break ;

    case CHARGE_CHARGER_OUT_OF_SPEC:
      static int off_charger_time = 0;

      if (!onContacts) {
        if (++off_charger_time > OffOOSChargerReset) {
          setChargeState(CHARGE_OFF_CHARGER);
          chargeBounces = 0;
        }
      } else {
        off_charger_time = 0;
      }

      break ;
  }
}

void Battery::manage()
{
  using namespace Battery;
  static const int LOW_BAT_TIME = CYCLES_MS(60*1000); // 1 minute
  static const int LOW_POWER_TIME = 5760000; // 8 hours of low power
  static int low_power_ticks;

  // Startup heads up check
  #ifdef FACTORY
  if (active_operating_mode == BODY_STARTUP && GetCounter() > CYCLES_MS(500)) {
    Battery::setOperatingMode(BODY_BLUETOOTH_OPERATING_MODE);
  }
  #endif

  // Do a full power down if the robot is in low power mode for too long
  if (active_operating_mode == BODY_LOW_POWER_OPERATING_MODE) {
    if (++low_power_ticks >= LOW_POWER_TIME) {
      powerOff();
    }
  } else {
    low_power_ticks = 0;
  }

  if (!NRF_ADC->EVENTS_END) {
    return ;
  }

  switch (m_pinIndex)
  {
    case ANALOG_V_BAT_SENSE:
      {
        vBat = calcResult(VBAT_SCALE);

        #ifdef FACTORY
        //u16 vBat100x = (vBat * 100) / 65536; //convert to centiVolts
        if( vBat < (430*65536)/100 ) //< 4.3V
          vBatFiltered = vBat; //filter out higher voltages when connected to vExt
        #endif

        Backpack::setLowBattery(vBat < VBAT_LOW_THRESHOLD);

        // after 1 minute of low battery, turn off
        static int lowBatTimer = GetCounter() + LOW_BAT_TIME;

        if (vBat < VBAT_CHGD_LO_THRESHOLD) {
          int ticks_left = lowBatTimer - GetCounter();

          if (ticks_left < 0) {
            powerOff();
          }
        } else {
          lowBatTimer = GetCounter() + LOW_BAT_TIME;
        }

        startADCsample(ANALOG_CLIFF_SENSE);
        sendPowerStateUpdate();

        break ;
      }
    case ANALOG_V_EXT_SENSE:
      {
        // Are our power pins shorted?
        static int ground_short = 0;

        #ifdef FACTORY
        if (*FIXTURE_HOOK != 0xDEADFACE)
        #endif
        if (NRF_ADC->RESULT < 0x30) {
          if (++ground_short > 30) {
            Battery::powerOff();
            NVIC_SystemReset();
          }
          else if (ground_short == 10)
          {
            AnkiEvent( 415, "charge_contact_shorted", 347, "%d", 1, NRF_ADC->RESULT);
          }
        } else {
          ground_short = 0;
        }

        vExt = calcResult(VEXT_SCALE);
        updateChargeState(vExt);

        #ifdef FACTORY
        if( vExt < (025*65536)/100 || vExt > (420*65536)/100 ) // vExt > 4.2V, or near 0 (can register ~0.5V from charge contact communications)
          vExtFiltered = vExt; //filter out higher voltages when connected to vExt
        #endif

        if (active_operating_mode == BODY_LOW_POWER_OPERATING_MODE) {
          nrf_gpio_pin_clear(PIN_IR_DROP);
          startADCsample(ANALOG_V_EXT_SENSE);
        } else {
          startADCsample(ANALOG_CLIFF_SENSE);
        }
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
          startADCsample(ANALOG_V_EXT_SENSE);

        }

        int result = resultLedOn - resultLedOff - CLIFF_SENSOR_BLEED;
        g_dataToHead.cliffLevel = (result < 0) ? 0 : result;
        g_dataToHead.cliffOffLevel = resultLedOff;

        nrf_gpio_pin_toggle(PIN_IR_DROP);

        break ;
      }
  }
}
