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
#include "crypto.h"
#include "bluetooth.h"

#include "sha1.h"

#include "bootloader.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#define SET_GREEN(v, b)  (b ? (v |= 0x00FF00) : (v &= ~0x00FF00))

__attribute((at(0x20003FFC))) static uint32_t MAGIC_LOCATION = 0;

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

void enterOperatingMode(BodyOperatingMode mode) {
  switch (mode) {
    case BLUETOOTH_OPERATING_MODE:
      Timer::lowPowerMode(true);
      Backpack::lightMode(RTC_LEDS);

      Radio::shutdown();
      Bluetooth::advertise();
      break ;
    
    case WIFI_OPERATING_MODE:
      Bluetooth::shutdown();
      Radio::advertise();

      Backpack::lightMode(TIMER_LEDS);
      Timer::lowPowerMode(false);

      break ;
  }
}

int main(void)
{
  Storage::init();
  
  // Initialize our scheduler
  RTOS::init();

  // Initialize the SoftDevice handler module.
  Bluetooth::init();
  Crypto::init();

  // Setup all tasks
  Motors::init();
  Radio::init();
  Head::init();
  Battery::init();
  Timer::init();
  Backpack::init();
  Lights::init();

  // Startup the system
  Battery::powerOn();

  // Let the test fixtures run, if nessessary
  #ifdef RUN_TESTS
  Head::enabled(false);
  TestFixtures::run();
  #endif

  enterOperatingMode(WIFI_OPERATING_MODE);

  Timer::start();

  // Run forever, because we are awesome.
  for (;;) {
    // This means that if the crypto engine is running, the lights will stop pulsing. 
    Crypto::manage();
    Lights::manage();
    Backpack::manage();
    Radio::updateLights();
  }
}
