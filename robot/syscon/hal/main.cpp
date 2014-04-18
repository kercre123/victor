#include "battery.h"
#include "motors.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "spiData.h"

const u32 MAX_FAILED_TRANSFER_COUNT = 10;
GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern void MotorsPrintRaw();

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
  u32 failedTransferCount = 0;
  
  // Initialize the hardware peripherals
  BatteryInit();
  TimerInit();
  MotorsInit();
  SPIInit();
  UARTInit();
  nrf_gpio_pin_clear(17);  // Multiplex blinking LED with UART
  nrf_gpio_cfg_output(17);
  
  UARTPutString("\r\nInitialized...\r\n");
  
  g_dataToHead.common.source = SPI_SOURCE_BODY;
  
  /*g_dataToBody.motorPWM[0] = 0; //0x7fff * 0.7;
  g_dataToBody.motorPWM[1] = 0; //0x7fff * 0.7;
  g_dataToBody.motorPWM[2] = 0; //0x7fff * 0.7;
  g_dataToBody.motorPWM[3] = 0; //0x7fff * -0.3;*/
  
  // Force charger on for now
  nrf_gpio_pin_clear(16);
  nrf_gpio_cfg_output(16);
  
  while (1)
  {
#if 0
    // Motor testing...
    for (int i = 0; i < MOTOR_COUNT; i++)
    {
      MotorsSetPower(i, 0x7fff);
    }
    
    MotorsUpdate();
    
    u32 t = GetCounter();
    while ((GetCounter() - t) < 8333333)  // 1 second
    {
      MotorsPrintEncodersRaw();
    }
    
    for (int i = 0; i < MOTOR_COUNT; i++)
    {
      MotorsSetPower(i, -0x7fff);
    }
      
    MotorsUpdate();
    
    t = GetCounter();
    while ((GetCounter() - t) < 8333333)
    {
      MotorsPrintEncodersRaw();
    }
    
    //BatteryUpdate();
    
#else
    
    u32 timerStart = GetCounter();
    // Exchange data with the head board
    SPITransmitReceive(
      sizeof(GlobalDataToBody),
      (const u8*)&g_dataToHead,
      (u8*)&g_dataToBody);
    
#if 1
    NRF_UART0->PSELTXD = 17;
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
      // TODO: Remove 0. For now, needed to do head debugging
      if(0 && ++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
      {
        // Perform a full system reset in order to reinitialize the head board
        NVIC_SystemReset();
      }
    } else  {
      failedTransferCount = 0;
      
      static u32 s = 0;
      if (++s < 10)
      {
        NRF_UART0->PSELTXD = 0xffffffff;
        //nrf_gpio_pin_toggle(17);
      } else if (++s > 20) {
        s = 0;
      }
      
      // Update motors
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        MotorsSetPower(i, g_dataToBody.motorPWM[i]);
      }
      
      MotorsUpdate();
    }
    
    // Update at 200Hz
    // 41666 ticks * 120 ns is roughly 5ms
    while ((GetCounter() - timerStart) < 41666)
      ;
#endif
  }
  
  return 0;
}
