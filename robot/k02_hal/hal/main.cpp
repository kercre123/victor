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
#include "spine.h"
#include "power.h"
#include "watchdog.h"
#include "i2c.h"
#include "imu.h"

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern int StartupSelfTest(void);

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      bool videoEnabled_ = false;

      // Import init functions from all HAL components
      void CameraInit(void);
      void CameraStart(void);

      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}
      u32 GetID() { return *(uint32_t*) 0xFFC; }
      void SetImageSendMode(const ImageSendMode mode, const ImageResolution res) { videoEnabled_ = (mode != Off); }
      bool IsVideoEnabled() { return videoEnabled_; }

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

  // Force recovery mode if watchdog count gets too high
  if (WDOG_RSTCNT > MAXIMUM_RESET_COUNT) {
    Anki::Cozmo::HAL::SPI::EnterRecoveryMode();
  }

  // Enable reset filtering
  RCM_RPFC = RCM_RPFC_RSTFLTSS_MASK | RCM_RPFC_RSTFLTSRW(2);
  RCM_RPFW = 16;

  Power::enableEspressif();

  UART::DebugInit();
  #ifndef ENABLE_FCC_TEST
  SPI::Init();
  #endif
  DAC::Init();

  I2C::Init();
  IMU::Init();
  OLED::Init();
  

  CameraInit();

  Anki::Cozmo::Robot::Init();

  // We can now safely start camera DMA, which shortly after starts HALExec
  // This function returns after the first call to HALExec is complete
  SPI::Init();
  Watchdog::init();
  CameraStart();

  // IT IS NOT SAFE TO CALL ANY HAL FUNCTIONS (NOT EVEN DebugPrintf) AFTER CameraStart() 

  // Run the main thread (lite)
  for(;;)
  {
    Watchdog::kick(WDOG_MAIN_EXEC);
    
    // Pump Wifi clad as quickly as possible
    do {
      WiFi::Update();
    } while (!UART::FoundSync());
    
    // Wait for head body sync to occur
    UART::WaitForSync();
    Spine::Manage();

    if (Anki::Cozmo::Robot::step_MainExecution() != Anki::RESULT_OK)
    {
      NVIC_SystemReset();
    }
  }
}
