#include "nrf.h"
#include "nrf_gpio.h"

#include "battery.h"
#include "motors.h"
#include "head.h"
#include "uart.h"
#include "timer.h"
#include "lights.h"
#include "tests.h"
#include "radio.h"

#include "anki/cozmo/robot/spineData.h"

const u32 MAX_FAILED_TRANSFER_COUNT = 10;

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

void EncoderIRQTest(void) {
  static bool direction = false;
  static int counter = 0;
  int now = GetCounter();
  
  return ;
    UART::put("\r\nForward: ");
    Motors::setPower(2, 0x6800);
    Motors::update();
    MicroWait(50000);
    Motors::update();

    MicroWait(500000);
    Motors::printEncodersRaw();

    UART::put("  Backward: ");
    Motors::setPower(2, -0x6800);
    Motors::update();
    MicroWait(50000);
    Motors::update();

    MicroWait(500000);
    Motors::printEncodersRaw();

}

int main(void)
{
  // 
  u32 failedTransferCount = 0;

  // Initialize the hardware peripherals
  BatteryInit();
  TimerInit();
  Motors::init();   // Must init before power goes on
  Head::init();

  UART::put("\r\nUnbrick me now...");
  u32 t = GetCounter();
  while ((GetCounter() - t) < 500 * COUNT_PER_MS)  // 0.5 second unbrick time
    ;
  UART::put("too late!\r\n");

  // Finish booting up
  Radio::init();
  PowerOn();

  #ifdef TESTS_ENABLED
  TestFixtures::run();
  #endif

  g_dataToHead.common.source = SPI_SOURCE_BODY;
  g_dataToHead.tail = 0x84;

  for (;;)
  {
    u32 timerStart = GetCounter();
    g_dataToBody.common.source = SPI_SOURCE_CLEAR;

    // If we're not on the charge contacts, exchange data with the head board
    if (!IsOnContacts())
    {
      Head::TxRx(
        sizeof(GlobalDataToBody),
        (const u8*)&g_dataToHead,
        (u8*)&g_dataToBody);
    }
    else // If not, reset the spine system
    {
      Head::spokenTo = false;
    }

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
    
    // Only call every loop through - not all the time
    Motors::update();
    BatteryUpdate();
    Lights::manage(g_dataToBody.backpackColors);
   
    // Update at 200Hz (5ms delay)
    const int RTC_CLOCK = 32768;
    const int SYNC_RATE = 200;
    const int TICKS_PER_CYCLE = (int)(RTC_CLOCK / (float)SYNC_RATE) << 8;

    while ((GetCounter() - timerStart) < TICKS_PER_CYCLE)
      ;

    Radio::manage();
  }
}
