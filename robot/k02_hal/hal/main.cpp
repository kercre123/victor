#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/hardware.h"

#include "uart.h"
#include "oled.h"
#include "spi.h"
#include "dac.h"
#include "wifi.h"
#include "hal/i2c.h"
#include "hal/imu.h"

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern int StartupSelfTest(void);

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Import init functions from all HAL components
      void CameraInit(void);
      void CameraStart(void);
      void TimerInit(void);
      void PowerInit(void);

      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}
      u32 GetID() { return 0; } ///< Stub for K02 for now, need to get this from Espressif Flash storage

      // This method is called at 7.5KHz (once per scan line)
      // After 7,680 (core) cycles, it is illegal to run any DMA or take any interrupt
      // So, you must hit all the registers up front in this method, and set up any DMA to finish quickly
      void HALExec(void)
      {
        I2C::Enable();
        UART::Transmit();
        IMU::Manage();
      }
    }
  }
}

extern "C" const int __ESPRESSIF_SERIAL_NUMBER;

int main (void)
{
  using namespace Anki::Cozmo::HAL;

  // Power up all ports
  SIM_SCGC5 |=
    SIM_SCGC5_PORTA_MASK |
    SIM_SCGC5_PORTB_MASK |
    SIM_SCGC5_PORTC_MASK |
    SIM_SCGC5_PORTD_MASK |
    SIM_SCGC5_PORTE_MASK;

  // Initialize everything we can, while waiting for Espressif to boot
  UART::DebugInit();
  TimerInit();
  PowerInit();

  DAC::Init();
  DAC::Tone();
  
  I2C::Init();
  IMU::Init();
  OLED::Init();
  UART::Init();
  CameraInit();

  Anki::Cozmo::Robot::Init();
  DAC::Mute();

  // Wait for Espressif to toggle out 4 words of I2SPI
  for (int i = 0; i < 4; i++)
  {
    while (GPIO_READ(GPIO_WS) & PIN_WS)     ;
    while (!(GPIO_READ(GPIO_WS) & PIN_WS))  ;
  }
  
  // Switch to 10MHz Espressif/external reference and 100MHz clock
  MCG_C1 &= ~MCG_C1_IREFS_MASK;
  // Wait for IREF to turn off
  while((MCG->S & MCG_S_IREFST_MASK))   ;
  // Wait for FLL to lock
  while((MCG->S & MCG_S_CLKST_MASK))    ;

  MicroWait(100);     // Because of erratum e7735: Wait 2 IRC cycles (or 2/32.768KHz)
  
  // We can now safely start camera DMA, which shortly after starts HALExec
  // This function returns after the first call to HALExec is complete
  SPI::Init();
  OLED::ErrorCode(255);
  CameraStart();
  
  // IT IS NOT SAFE TO CALL ANY HAL FUNCTIONS (NOT EVEN DebugPrintf) AFTER CameraStart() 
  //StartupSelfTest();

  // Run the main thread
  do {
    // Wait for head body sync to occur
    UART::WaitForSync();
    Anki::Cozmo::HAL::IMU::Update();
    Anki::Cozmo::HAL::WiFi::Update();
  } while (Anki::Cozmo::Robot::step_MainExecution() == Anki::RESULT_OK);
}
