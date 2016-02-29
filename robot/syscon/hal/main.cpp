#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "rtos.h"
#include "hardware.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "debug.h"
#include "timer.h"
#include "lights.h"
#include "tests.h"
#include "radio.h"
#include "rng.h"

#include "bootloader.h"

#include "anki/cozmo/robot/spineData.h"

#define SET_GREEN(v, b)  (b ? (v |= 0x00FF00) : (v &= ~0x00FF00))

static const u32 MAX_FAILED_TRANSFER_COUNT = 18000; // 1.5m for auto shutdown (if not on charger)

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

void EnterRecovery(void) {
  Motors::teardown();

  __enable_irq();
  __asm { SVC 0 };
}

void MotorsUpdate(void* userdata) {
  static int failedTransferCount = 0;

  // Verify the source
  if (!Head::spokenTo)
  {
    // Turn off the system if it hasn't talked to the head for a minute
    if(++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
    {
      #ifndef RADIO_TIMING_TEST
      Battery::powerOff();
      NVIC_SystemReset();
      #endif
    }
  } else {
    failedTransferCount = 0;
    // Copy (valid) data to update motors
    for (int i = 0; i < MOTOR_COUNT; i++)
    {
      Motors::setPower(i, g_dataToBody.motorPWM[i]);
    }

    Battery::setHeadlight(g_dataToBody.flags & BODY_FLASHLIGHT);
  }

  // Prevent overheating
  if (Battery::onContacts) {
    for (int i = 0; i < MOTOR_COUNT; i++)
    {
      Motors::setPower(i, 0);
    }
  }
}

int main(void)
{
  // Initialize our scheduler
  RTOS::init();

  // Initialize the hardware peripherals
  
  // WARNING: DO NOT CALL THIS UNLESS YOU GET APPROVAL FROM SOMEONE ON THE
  // FIRMWARE TEAM.  YOU CAN BRICK YOUR COZMO AND MAKE EVERYONE VERY SAD.

  Bootloader::init();
  
  Battery::init();
  Timer::Init();
  Motors::init();
  Lights::init();
  Random::init();
  Radio::init();
  Battery::powerOn();

  // Let the test fixtures run, if nessessary
  TestFixtures::run();

  // Only use Head/body if tests are not enabled
  Head::init();
  RTOS::schedule(MotorsUpdate);   


  // Start the scheduler
  RTOS::run();
}
