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

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern "C" const uint32_t CurrentRobotBootloader;
static const int FLASH_BLOCK_SIZE = 0x800;
static const int BOOTLOADER_LENGTH = 0x1000;

static int SendCommand()
{
  const int FSTAT_ERROR = FTFA_FSTAT_FPVIOL_MASK | FTFA_FSTAT_ACCERR_MASK | FTFA_FSTAT_FPVIOL_MASK;

  // Wait for idle and clear errors
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;
  FTFA->FSTAT = FSTAT_ERROR;

  // Start flash command (and wait for completion)
  FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;

  // Return if there was an error
  return FTFA->FSTAT & FSTAT_ERROR;
}

static bool FlashSector(int target, const uint32_t* data)
{
  volatile const uint32_t* original = (uint32_t*)target;
  volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;

  // Test for sector erase nessessary
  FTFA->FCCOB0 = 0x09;
  FTFA->FCCOB1 = target >> 16;
  FTFA->FCCOB2 = target >> 8;
  FTFA->FCCOB3 = target;
    
  // Erase block
  if (SendCommand()) { 
    return false;
  }

  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t)) {
    // Program word does not need to be written
    if (data[i] == original[i]) {
      continue ;
    }

    // Write program word
    flashcmd[0] = (target & 0xFFFFFF) | 0x06000000;
    flashcmd[1] = data[i];
    
    // Did command succeed
    if (SendCommand()) { 
      return false;
    }
  }

  return true;
}

static void UpdateBootloader() {
  const uint32_t* data = &CurrentRobotBootloader;
  for (int i = 0; i < BOOTLOADER_LENGTH; i += FLASH_BLOCK_SIZE) {
    FlashSector(i, &data[i / sizeof(uint32_t)]);
  }
}

namespace Anki
{
  namespace Cozmo
  {
    namespace Messages {
      void Messages::ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        // K02 supervisor removed for code space
      }
    } // namespace Messages

    namespace RobotInterface {
      int SendLog(const LogLevel level, const uint16_t name, const uint16_t formatId, const uint8_t numArgs, ...)
      {
        return 0;
      }
    } // namespace RobotInterface

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
  if (RCM_SRS0 & RCM_SRS0_WDOG_MASK) {
    if (WDOG_RSTCNT > MAXIMUM_RESET_COUNT) {
      Anki::Cozmo::HAL::SPI::EnterRecoveryMode();
    }
  }

  UpdateBootloader();

  // Enable reset filtering
  RCM_RPFC = RCM_RPFC_RSTFLTSS_MASK | RCM_RPFC_RSTFLTSRW(2);
  RCM_RPFW = 16;

  // Workaround a hardware bug (missing pull-down) until UARTInit gets called much later
  SOURCE_SETUP(GPIO_BODY_UART_TX, SOURCE_BODY_UART_TX, SourceGPIO | SourcePullDown);
  
  Watchdog::init();
  Power::enableExternal();
  Watchdog::kickAll();

  UART::DebugInit();
  DAC::Init();

  // Sequential I2C bus initalization (Non-interrupt based)
  I2C::Init();
  IMU::Init();
  OLED::Init();

  CameraInit();

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
  }
}
