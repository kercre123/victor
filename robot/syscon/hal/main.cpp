#include <string.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"
}

#include "hardware.h"

#include "storage.h"
#include "rtos.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "timer.h"
#include "backpack.h"
#include "lights.h"
#include "tests.h"
#include "radio.h"
#include "random.h"
#include "crypto.h"
#include "bluetooth.h"
#include "dtm.h"
#include "bootloader.h"

#include "sha1.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#include "clad/robotInterface/messageEngineToRobot.h"


GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern "C" void HardFault_Handler(void) {
  __disable_irq();

  uint8_t pins[] = {
    PIN_LED1,
    PIN_LED2,
    PIN_LED3,
    PIN_LED4
  };

  for (int i = 0; i < sizeof(pins) * sizeof(pins) * 2; i++) {
    for (int k = 0; k < sizeof(pins); k++) {
      nrf_gpio_cfg_input(pins[k], NRF_GPIO_PIN_NOPULL);
    }

    uint8_t ann = pins[i % sizeof(pins)];
    uint8_t cath = pins[i / sizeof(pins) % sizeof(pins)];

    nrf_gpio_pin_set(ann);
    nrf_gpio_cfg_output(ann);
    nrf_gpio_pin_clear(cath);
    nrf_gpio_cfg_output(cath);
    
    MicroWait(10000);
  }

  NVIC_SystemReset();
}

using namespace Anki::Cozmo::RobotInterface;

// Not in a known operating mode until later in main()
static BodyRadioMode current_operating_mode = -1;

void enterOperatingMode(BodyRadioMode mode) {
  current_operating_mode = mode;
}

int g_powerOffTime = (4<<23);   // 4 seconds from power-on
bool g_turnPowerOff = false;
static void setupOperatingMode() {
  static BodyRadioMode active_operating_mode = -1;
  BodyRadioMode new_mode = current_operating_mode;

  // XXX: Factory only - to support test modes:  Dirty hack to run battery for 4 seconds after power-up regardless of mode
  if (g_turnPowerOff && GetCounter() > g_powerOffTime)
  {
    Battery::powerOff();
    g_turnPowerOff = false;
  }
  
  if (active_operating_mode == new_mode) {
    return ;
  }

  g_turnPowerOff = false;

  // Tear down existing mode
  switch (active_operating_mode) {
    case BODY_BLUETOOTH_OPERATING_MODE:
      Bluetooth::shutdown();
      break ;
    
    case BODY_ACCESSORY_OPERATING_MODE:
      Radio::shutdown();
      break ;

    case BODY_DTM_OPERATING_MODE:
      DTM::stop();
      break ;

    case BODY_IDLE_OPERATING_MODE:
      break ;
  }

  // Setup new mode
  switch(new_mode) {
    case BODY_LOW_POWER_OPERATING_MODE:
      // Shut-down non-essentials
      __disable_irq();  
      Motors::disable(true);
      Motors::manage(0);        // Otherwise disable doesn't take effect
      Battery::powerOff();      // Make sure we reboot when removed from charger
      
      // Power-down encoders and front facing LED
      nrf_gpio_pin_set(PIN_VDDs_EN);
      nrf_gpio_pin_clear(PIN_IR_FORWARD);

      // Loop forever
      for (;;) {
        for (int i = 0; i < 8; i++) {
          NRF_WDT->RR[i] = WDT_RR_RR_Reload;
        }
      }
      break ;

    case BODY_IDLE_OPERATING_MODE:
      Motors::disable(true);  
      g_turnPowerOff = true;
      Timer::lowPowerMode(true);
      Backpack::lightMode(RTC_LEDS);
      break ;
    
    case BODY_BLUETOOTH_OPERATING_MODE:
      Motors::disable(true);
      Battery::powerOn();
      Timer::lowPowerMode(true);
      Backpack::lightMode(RTC_LEDS);

      Bluetooth::advertise();
      break ;
    
    case BODY_ACCESSORY_OPERATING_MODE:
      Motors::disable(false);
      Battery::powerOn();
      Backpack::lightMode(TIMER_LEDS);

      Radio::advertise();

      Timer::lowPowerMode(false);
      break ;

    case BODY_DTM_OPERATING_MODE:
      Motors::disable(false);
      Timer::lowPowerMode(true);
      Backpack::lightMode(RTC_LEDS);

      DTM::start();
      break ;
  }
  
  active_operating_mode = new_mode;
}

int main(void)
{
  using namespace Anki::Cozmo::RobotInterface;

  Storage::init();

  // Initialize our scheduler
  RTOS::init();
  // XXX: Never for factory!  Bootloader::init();

  // Initialize the SoftDevice handler module.
  Bluetooth::init();
  Crypto::init();
  Lights::init();

  // Setup all tasks
  Motors::init();
  Radio::init();
  Head::init();
  Battery::init();
  Timer::init();
  Backpack::init();

  // Startup the system
  Battery::powerOn();

  // Let the test fixtures run, if nessessary
  #ifdef RUN_TESTS
  Head::enabled(false);
  TestFixtures::run();
  #endif

  enterOperatingMode(BODY_IDLE_OPERATING_MODE);
  setupOperatingMode();

  Motors::start();
  Timer::start();

  // Run forever, because we are awesome.
  for (;;) {
    // This means that if the crypto engine is running, the lights will stop pulsing. 
    Crypto::manage();
    Lights::manage();
    Backpack::manage();
    setupOperatingMode();
  }
}
