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
#include "bootloader.h"

#include "uart.h"
#include "oled.h"
#include "spi.h"
#include "dac.h"
#include "wifi.h"
#include "spine.h"
#include "power.h"
#include "watchdog.h"
#include "i2c.h"
#include "imu.h"
#include "fcc.h"

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

      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}
      u32 GetID() { return *(uint32_t*) 0xFFC; }

      void HALInit(void) {
        UART::Init();
        DAC::Sync();
      }
      
      // This method is called at 7.5KHz (once per scan line)
      // After 7,680 (core) cycles, it is illegal to run any DMA or take any interrupt
      // So, you must hit all the registers up front in this method, and set up any DMA to finish quickly
      void HALExec(void)
      {
        I2C::Enable();
        SPI::ManageDrop();
        UART::Transmit();
        IMU::Manage();
        Watchdog::kick(WDOG_HAL_EXEC);
        Watchdog::pet();
      }
    }
  }
}

// This silences exception allocations
extern "C" 
void * __aeabi_vec_ctor_nocookie_nodtor(   void* user_array,
                                           void* (*constructor)(void*),
                                           size_t element_size,
                                           size_t element_count) 

{
    size_t ii = 0;
    char *ptr = (char*) (user_array);
    if ( constructor != NULL )
        for( ; ii != element_count ; ii++, ptr += element_size )
            constructor( ptr );
    return user_array;
}

int main (void)
{
  using namespace Anki::Cozmo::HAL;

  // Enable reset filtering
  RCM_RPFC = RCM_RPFC_RSTFLTSS_MASK | RCM_RPFC_RSTFLTSRW(2);
  RCM_RPFW = 16;

  update_bootloader();
  
  Power::enableEspressif();

  UART::DebugInit();
  #ifndef ENABLE_FCC_TEST
  Watchdog::init();
  SPI::Init();
  #endif
  DAC::Init();

  I2C::Init();
  IMU::Init();
  OLED::Init();
  
  #ifdef ENABLE_FCC_TEST
  UART::Init();
  FCC::start();
  I2C::Enable();
  for (;;) {
    UART::Transmit();

    if (UART::HeadDataReceived) {
      UART::HeadDataReceived = false;
      FCC::mainDTMExecution();
    }
  }
  #else
    CameraInit();

    Anki::Cozmo::Robot::Init();

    // We can now safely start camera DMA, which shortly after starts HALExec
    // This function returns after the first call to HALExec is complete
    SPI::Init();
    CameraStart();

    // IT IS NOT SAFE TO CALL ANY HAL FUNCTIONS (NOT EVEN DebugPrintf) AFTER CameraStart() 
  
    #if defined(FACTORY_FIRMWARE)
      // Run the main thread (lite)
      for(;;) {
        // Wait for head body sync to occur
        UART::WaitForSync();
        Spine::Manage();
        WiFi::Update();
      }
   #else
      // Run the main thread
      do {
        // Wait for head body sync to occur
        UART::WaitForSync();
      } while (Anki::Cozmo::Robot::step_MainExecution() == Anki::RESULT_OK);
    #endif
  #endif
}
