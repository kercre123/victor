#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/buildTypes.h"
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
#include "fcc.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern int StartupSelfTest(void);
void UpdateBootloader();

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
      void SetTimeStamp(TimeStamp_t t) {
        using namespace Anki::Cozmo::RobotInterface;
        
        AdjustTimestamp msg;
        msg.timestamp = t;
        RobotInterface::SendMessage(msg);
      }

      u32 GetID() { return *(uint32_t*) 0xFFC; }
      void SetImageSendMode(const ImageSendMode mode, const ImageResolution res) { videoEnabled_ = (mode != Off); }
      bool IsVideoEnabled() { return videoEnabled_; }

      void HALInit(void) {
        Watchdog::kickAll();

        // Configure the I2C junk
        I2C::Start();
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
        Watchdog::kick(WDOG_HAL_EXEC);
        Watchdog::pet();
      }

      void HALSafe(void)
      {
        IMU::Manage();
      }
      
      u16 GetWatchdogResetCounter(void)
      {
        return WDOG_RSTCNT;
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
  #ifndef FCC_TEST
  if (RCM_SRS0 & RCM_SRS0_WDOG_MASK) {
    if (WDOG_RSTCNT >= MAXIMUM_RESET_COUNT) {
      Anki::Cozmo::HAL::SPI::EnterRecoveryMode();
    }
  }
  #endif

  // Enable reset filtering
  RCM_RPFC = RCM_RPFC_RSTFLTSS_MASK | RCM_RPFC_RSTFLTSRW(2);
  RCM_RPFW = 16;

  // Workaround a hardware bug (missing pull-down) until UARTInit gets called much later
  SOURCE_SETUP(BODY_UART_TX, SourceGPIO | SourcePullDown);

  UART::DebugInit();
  UART::DebugPutc('\xAA');  
  Watchdog::init();
  Power::enableExternal();
  Watchdog::kickAll();

  #ifndef FACTORY
  // Check the bootloader patch is applied - if not, apply it now
  UpdateBootloader();
  #endif

  DAC::Init();

  // Sequential I2C bus initalization (Non-interrupt based)
  I2C::Init();
  IMU::Init();
  OLED::Init();

  #ifdef FCC_TEST
  HALInit();
  I2C::Enable();

  MicroWait(1000000);
  FCC::start();
  for(;;) {
    UART::Transmit();
    FCC::mainDTMExecution();
  }
  #else
  CameraInit();

  Anki::Cozmo::Robot::Init();

  // We can now safely start camera DMA, which shortly after starts HALExec
  // This function returns after the first call to HALExec is complete
  SPI::Init();
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

    // Copy through our timestamp
    t_ = g_dataToHead.timestamp;

    if (Anki::Cozmo::Robot::step_MainExecution() != Anki::RESULT_OK)
    {
      NVIC_SystemReset();
    }
  }
  #endif
}
