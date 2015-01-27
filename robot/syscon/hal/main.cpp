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

const u8 PIN_CHGEN = 5;  // 3.0

const u8 PIN_LED1 = 18;
const u8 PIN_LED2 = 19;

void encoderAnalyzer(void);
int main(void)
{
  u32 failedTransferCount = 0;
  
  // Initialize the hardware peripherals
  BatteryInit();
  TimerInit();
  MotorsInit();   // Must init before power goes on
  //UARTInit();  // XXX - do not enable unless you want neck cable to break
  
  UARTPutString("\r\nUnbrick me now...");
  u32 t = GetCounter();
  while ((GetCounter() - t) < 500 * COUNT_PER_MS)  // 0.5 second unbrick time
    ;
  UARTPutString("too late!\r\n");

  // Finish booting up
  SPIInit();
  PowerInit(); 
  
  g_dataToHead.common.source = SPI_SOURCE_BODY;
  g_dataToHead.tail = 0x84;
  
  // Force charger on for now
  nrf_gpio_pin_clear(PIN_CHGEN);
  nrf_gpio_cfg_output(PIN_CHGEN);
  
  // Status LED hack
  nrf_gpio_cfg_output(PIN_LED1);
  nrf_gpio_cfg_output(PIN_LED2);
  // LED1 set means on, LED1 clear means off
  nrf_gpio_pin_clear(PIN_LED2);
  nrf_gpio_pin_set(PIN_LED1);    

#if 0
  // Motor testing, loop forever
  while (1)
  {
    UARTPutString("\nForward ends with...");
    for (int i = 2; i < 4; i++)
      MotorsSetPower(i, 0x5fff);   
    MotorsUpdate();
    MicroWait(5000);
//    encoderAnalyzer();
    MotorsUpdate();

    MicroWait(500000);
//    encoderAnalyzer();
    MotorsPrintEncodersRaw();
    
    UARTPutString("\nBackward ends with...");
    
    for (int i = 2; i < 4; i++)    
      MotorsSetPower(i, -0x5fff);
    MotorsUpdate();
    MicroWait(5000);
//    encoderAnalyzer();
    MotorsUpdate();

    MicroWait(500000);
//    encoderAnalyzer();
    MotorsPrintEncodersRaw();
    
    BatteryUpdate();
  }
  
#else
  while (1)
  {    
    u32 timerStart = GetCounter();
    g_dataToBody.common.source = SPI_SOURCE_CLEAR;
    
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
      nrf_gpio_pin_set(PIN_LED1);   // Force LED on to indicate problems
      // TODO: Remove 0. For now, needed to do head debugging
      if(++failedTransferCount > MAX_FAILED_TRANSFER_COUNT)
      {
        // Perform a full system reset in order to reinitialize the head board
        //NVIC_SystemReset();
        
        // Stop all motors
        for (int i = 0; i < MOTOR_COUNT; i++)
          MotorsSetPower(i, 0);
      }
    } else  {
      failedTransferCount = 0;
      // Copy (valid) data to update motors
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        MotorsSetPower(i, g_dataToBody.motorPWM[i]);
      }
      
      static u8 s_blink = 0;
      nrf_gpio_pin_clear(PIN_LED1);
      if (++s_blink > 40) {
        nrf_gpio_pin_set(PIN_LED1);      
        s_blink = 0;
      }
    }
         
    // Only call every loop through - not all the time
    MotorsUpdate();
    
    // Update at 200Hz
    // 41666 ticks * 120 ns is roughly 5ms
    while ((GetCounter() - timerStart) < 41666)
      ;
  }
#endif
}
