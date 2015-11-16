#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "hardware.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "debug.h"
#include "timer.h"
#include "lights.h"
#include "tests.h"
#include "radio.h"

#include "anki/cozmo/robot/spineData.h"

static const u32 MAX_FAILED_TRANSFER_COUNT = 12000;

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern void EnterRecovery(void) {
  __asm { SVC 0 };
}

int main(void)
{
  //u32 failedTransferCount = 0;

  // Initialize the hardware peripherals
  Battery::init();
  TimerInit();
  Motors::init();   // Must init before power goes on
  Head::init();
  Lights::init();
  Radio::init();
  Battery::powerOn();

  TestFixtures::run();

  u32 timerStart = GetCounter();
  for (;;)
  {
    // Only call every loop through - not all the time
    Head::manage();
    Motors::update();
    Battery::update();

    #ifndef BACKPACK_DEMO
    Lights::manage(g_dataToBody.backpackColors);
    #endif

    // Update at 200Hz (5ms delay)
    timerStart += CYCLES_MS(5.0f);
    while ( timerStart > GetCounter()) ;
 
    // Update at 200Hz (5ms delay) - with unsigned subtract to handle wraparound
    const u32 DELAY = CYCLES_MS(5.0f);
    while (GetCounter() - timerStart < DELAY)
      ;
    timerStart += DELAY;

    /*
    // Verify the source
    if (Head::spokenTo)
    {
      // Turn off the system if it hasn't talked to the head for a minute
      if(++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
      {
        #ifndef RADIO_TIMING_TEST
        Battery::powerOff();
        return -1;
        #endif
      }
    } else {
      failedTransferCount = 0;
      // Copy (valid) data to update motors
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        Motors::setPower(i, g_dataToBody.motorPWM[i]);
      }
    }
    */
  }
}
