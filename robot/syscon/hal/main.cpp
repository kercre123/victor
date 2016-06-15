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

static BodyRadioMode current_operating_mode = BODY_BLUETOOTH_OPERATING_MODE;
static BodyRadioMode active_operating_mode = BODY_BLUETOOTH_OPERATING_MODE;

void enterOperatingMode(BodyRadioMode mode) {
  current_operating_mode = mode;
}

static void setupOperatingMode() { 
  if (active_operating_mode == current_operating_mode) {
    return ;
  }

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
  switch(current_operating_mode) {
    case BODY_IDLE_OPERATING_MODE:
      Motors::disable(true);  
      Battery::powerOff();
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
  
  active_operating_mode = current_operating_mode;
}

int main(void)
{
  using namespace Anki::Cozmo::RobotInterface;

  Storage::init();

  // Initialize our scheduler
  RTOS::init();

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
