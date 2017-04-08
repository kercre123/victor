#include <string.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"
}

#include "hardware.h"
#include "portable.h"
#include "anki/cozmo/robot/crashLogs.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "timer.h"
#include "storage.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "backpack.h"
#include "lights.h"
#include "cubes.h"
#include "bluetooth.h"
#include "messages.h"
#include "watchdog.h"
#include "tests.h"

#include "clad/robotInterface/messageEngineToRobot.h"

extern GlobalDataToHead g_dataToHead;

// This is our near-realtime loop
void main_execution(void) {
  Motors::manage();
  Head::manage();
  Battery::manage();
  Bluetooth::manage();

  // Update our lights
  Lights::manage();
  Backpack::manage();

  #ifdef FACTORY
  TestFixtures::manage();
  #endif

  Watchdog::kick(WDOG_MAIN_LOOP);

  g_dataToHead.timestamp += TIME_STEP;
}

int main(void)
{
  using namespace Anki::Cozmo::RobotInterface;

  #ifndef FACTORY
  // don't start in fixture mode in master mode
  while (*FIXTURE_HOOK == 0xDEADFACE) ;
  #endif

  Storage::init();

  // Initialize our scheduler
  Watchdog::init();
  Spine::init();

  // Initialize the SoftDevice handler module.
  Bluetooth::init();
  Lights::init();

  // Setup all tasks
  Motors::init();
  Radio::init();
  Head::init();
  Battery::init();
  Backpack::init();
  Timer::init();

  // Startup the system
  #ifdef FACTORY
  if (*FIXTURE_HOOK != 0xDEADFACE) {
    Battery::setOperatingMode(BODY_STARTUP);
  } else {
    Battery::setOperatingMode(BODY_BLUETOOTH_OPERATING_MODE);
  }
  #else
  Battery::setOperatingMode(BODY_BLUETOOTH_OPERATING_MODE);
  #endif

  // NOTE: HERE DOWN SOFTDEVICE ACCESS IS NOT GUARANTEED
  // Run forever, because we are awesome.
  for (;;) {
    __asm { WFI }
    Battery::updateOperatingMode();
  }
}
