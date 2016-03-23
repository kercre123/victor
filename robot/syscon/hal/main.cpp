#include <string.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"

  #include "softdevice_handler.h"  
}
  
#include "hardware.h"

#include "rtos.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "debug.h"
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
  
  
  nrf_gpio_pin_set(PIN_LED1);
  nrf_gpio_cfg_output(PIN_LED1);
  nrf_gpio_pin_set(PIN_LED2);
  nrf_gpio_cfg_output(PIN_LED2);
  nrf_gpio_pin_set(PIN_LED3);
  nrf_gpio_cfg_output(PIN_LED3);
  nrf_gpio_pin_set(PIN_LED4);
  nrf_gpio_cfg_output(PIN_LED4);

  
  MicroWait(1000000);
  
  NVIC_SystemReset();
}

extern void EnterRecovery(void) {
  Motors::teardown();

  MAGIC_LOCATION = SPI_ENTER_RECOVERY;
  NVIC_SystemReset();
}

int main(void)
{
  // Initialize the SoftDevice handler module.
  SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_SYNTH_250_PPM, NULL);

  // Initialize our scheduler
  RTOS::init();
  Crypto::init();

  // Setup all tasks (reverse priority)
  Radio::init();
  #ifndef RUN_TESTS
  Head::init();
  #endif
  Motors::init();
  Battery::init();
  Bluetooth::init();
  Timer::init();
  Backpack::init();
  Lights::init();

  // Startup the system
  Battery::powerOn();

  //Radio::shutdown();
  //Bluetooth::advertise(); 

  Bluetooth::shutdown();
  Radio::advertise();

  // Let the test fixtures run, if nessessary
  #ifdef RUN_TESTS
  TestFixtures::run();
  #endif

  // THIS IS ONLY HERE FOR DEVELOPMENT PURPOSES
  Bootloader::init();
  
  Timer::start();

  // Run forever, because we are awesome.
  for (;;) {
    __asm { WFI };
    Crypto::manage();
  }
}
