#include <string.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"
}

#include "hardware.h"

#include "timer.h"
#include "storage.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "backpack.h"
#include "lights.h"
#include "cubes.h"
#include "random.h"
#include "tasks.h"
#include "bluetooth.h"
#include "messages.h"
#include "watchdog.h"
#include "temp.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#include "clad/robotInterface/messageEngineToRobot.h"

extern "C" void HardFault_Handler(void) {
  __disable_irq();
  NVIC_SystemReset();
}

// This is our near-realtime loop
extern "C" void SWI0_IRQHandler(void) {
  static const int PERIOD = CYCLES_MS(5.0f);
  static int target = PERIOD + GetCounter();

  int ticks = target - GetCounter();

  if (ticks > 0) {
    return ;
  }
  
  target += PERIOD;

  Head::manage();
  Motors::manage();
  Battery::manage();
  Bluetooth::manage();

  Watchdog::kick(WDOG_MAIN_LOOP);
}

int main(void)
{
  using namespace Anki::Cozmo::RobotInterface;

  Storage::init();

  // Initialize our scheduler
  Watchdog::init();
  Spine::init();

  // Initialize the SoftDevice handler module.
  Bluetooth::init();
  Tasks::init();
  Lights::init();

  // Setup all tasks
  Temp::init();
  Motors::init();
  Radio::init();
  Head::init();
  Battery::init();
  Timer::init();
  Backpack::init();

  // Startup the system
  Battery::setOperatingMode(BODY_BLUETOOTH_OPERATING_MODE);

  // NOTE: HERE DOWN SOFTDEVICE ACCESS IS NOT GUARANTEED
  // Run forever, because we are awesome.
  for (;;) {
    Battery::updateOperatingMode();

    // This means that if the crypto engine is running, the lights will stop pulsing. 
    Tasks::manage();
    Lights::manage();
    Backpack::manage();
    Temp::manage();
  }
}
