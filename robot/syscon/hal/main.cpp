#include "motors.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      const u32 MAX_FAILED_TRANSFER_COUNT = 10;
      GlobalDataToHead g_dataToHead;
      GlobalDataToBody g_dataToBody;
    }
  }
}

void PowerInit();

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
  MotorsInit();
  PowerInit();
  SPIInit();
  
  UARTPutString("\r\nInitialized...\r\n");
  
  g_dataToHead.common.source = SPI_SOURCE_BODY;
  
  while (1)
  {
    u32 timerStart = GetCounter();
    // Exchange data with the head board
    SPITransmitReceive(
      sizeof(GlobalDataToBody),
      (const u8*)&g_dataToHead,
      (u8*)&g_dataToBody);

#if 0      
    u8* d = (u8*)&g_dataToBody;
    for (int i = 0; i < 0x10; i++)
    {
      UARTPutHex(d[i]);
      UARTPutChar(' ');
    }
    UARTPutChar('\n');
#endif
    
    // Verify the source
    if (g_dataToBody.common.source != SPI_SOURCE_HEAD)
    {
      if(++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
      {
        UARTPutString("\r\nToo many failed transfers\r\n");
        
        // Perform a full system reset in order to reinitialize the head board
        NVIC_SystemReset();
      }
    } else {
      failedTransferCount = 0;
      
      // Update motors
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        MotorsSetPower((MotorID)i, g_dataToBody.motorPWM[i]);
      }
      
      MotorsUpdate();
    }
    
    // Update at 200Hz
    //MicroWait(5000);
    while ((GetCounter() - timerStart) < 41666)
      ;
  }
  
  return 0;
}
