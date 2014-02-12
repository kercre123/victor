#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "nrf.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      const u32 MAX_FAILED_TRANSFER_COUNT = 10;
      GlobalData g_dataTX;
      GlobalData g_dataRX;
    }
  }
}

extern "C"
void SystemInit(void) 
{
  /* If desired, switch off the unused RAM to lower consumption by the use of RAMON register */

  /* Prepare the peripherals for use as indicated by the PAN 25 "System: Manual setup is required
     to enable the use of peripherals" found at Product Anomaly document version 1.2 found at 
     https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822/PAN-028. */
  //if ((((*(uint32_t *)0xF0000FE8) >> 4) & 0x0000000F) == 1)
  {
    *(volatile u32 *)0x40000504 = 0xC007FFDF;
    *(volatile u32 *)0x40006C18 = 0x00008000;
  }
}

int main(void)
{
  using namespace Anki::Cozmo::HAL;
  
  s32 result;
  u32 failedTransferCount = 0;
  
  // Initialize the hardware peripherals
  TimerInit();
  UARTInit();
  SPIInit();
  //MotorsInit();
  
  UARTPutString("\r\nInitialized...\r\n");
  
  GetMicroCounter();
  
  while (1)
  {
    result = SPITransmitReceive(
      sizeof(GlobalData), 
      (const u8*)&g_dataTX,
      (u8*)&g_dataRX);
    
    if (result)
    {
      if(++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
      {
        UARTPutString("\r\nToo many failed transfers\r\n");
        
        // Perform a full system reset in order to reinitialize the other board
        NVIC_SystemReset();
      }
    } else {
      // TODO:
      // Update motors and such
      // ...
    }
    
    // 200Hz
    MicroWait(5000);
  }
  
  return 0;
}
