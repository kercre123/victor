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
#include "drop.h"

#include "anki/cozmo/robot/spineData.h"

const u32 MAX_FAILED_TRANSFER_COUNT = 10;

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

int main(void)
{
  // 
  u32 failedTransferCount = 0;

  // Initialize the hardware peripherals
  Drop::init();
  Battery::init();
  TimerInit();
  Motors::init();   // Must init before power goes on
  Head::init();
  //Lights::init();

  UART::print("\r\nUnbrick me now...");
  u32 t = GetCounter();
  while ((GetCounter() - t) < 500 * COUNT_PER_MS)  // 0.5 second unbrick time
    ;
  UART::print("too late!\r\n");

  // Finish booting up
  Radio::init();
  Battery::powerOn();

  TestFixtures::run();

  g_dataToHead.common.source = SPI_SOURCE_BODY;
  g_dataToHead.tail = 0x84;

  for (;;)
  {
    u32 timerStart = GetCounter();
    g_dataToBody.common.source = SPI_SOURCE_CLEAR;

    // Only call every loop through - not all the time
    Motors::update();
    Radio::manage();
    Battery::update();

    #ifndef BACKPACK_DEMO
    Lights::manage(g_dataToBody.backpackColors);
    #endif

    // If we're not on the charge contacts, exchange data with the head board
    if (!Battery::onContacts)
    {
      Head::TxRx();
    }
    else // If not, reset the spine system
    {
      Head::spokenTo = false;
    }
    
    // Update at 200Hz (5ms delay)
    while ((GetCounter() - timerStart) < CYCLES_MS(35.0f / 7.0f))
      ;

    // Verify the source
    if (g_dataToBody.common.source != SPI_SOURCE_HEAD)
    {
      if(++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
      {
        // Perform a full system reset in order to reinitialize the head board
        //NVIC_SystemReset();

        // Stop all motors
        for (int i = 0; i < MOTOR_COUNT; i++)
          Motors::setPower(i, 0);
      }
    } else {
      failedTransferCount = 0;
      // Copy (valid) data to update motors
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        Motors::setPower(i, g_dataToBody.motorPWM[i]);
      }
    }
  }
}
