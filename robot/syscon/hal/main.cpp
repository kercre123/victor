#include "battery.h"
#include "motors.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "anki/cozmo/robot/spineData.h"

//#define DO_MOTOR_TESTING
//#define DO_GEAR_RATIO_TESTING
//#define DO_FIXED_DISTANCE_TESTING
//#define DO_LIGHTS_TESTING

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

// Execute a particular test - indexed by cmd with 16-bit signed arg
void TestRun(u8 cmd, s16 arg)
{
  const u8 REPSTART = 0xFC, REPEND = 0xFD;
  UARTPutChar(REPSTART);

  // Execute motor setpower commands (0-3)
  if (cmd <= 4)
  {
    MotorsSetPower(cmd, arg);
    MotorsUpdate();
    MicroWait(5000);
    MotorPrintEncoder(cmd);
    MotorsUpdate();
    MicroWait(5000);
  }
  // Execute read sensors command (4)
  if (4 == cmd)
  {
  }

  UARTPutChar(REPEND);
}

// Test mode enables direct control of the platform via the rear UART
// All commands start with a request from the host and send a reply to the client
// Standard requests are 5 bytes long - 0xFA header, command, arg0, arg1, 0xFB footer
// Standard replies are 2 bytes long - 0xFC command started, 0xFD command complete
// A few replies will return data between the command started and command complete
void TestMode(void)
{
  const int REQLEN = 5; // Requests are 5 bytes long
  const u8 REQSTART = 0xFA, REQEND = 0xFB;

  UARTInit();

  u8 req[REQLEN]; // Current request
  u8 reqLen = 0;  // Number of request bytes received

  // Read requests byte by byte and dispatch them
  while (1)
  {
    int c = UARTGetChar();

    // Wait for a character to arrive
    if (-1 == c)
      continue;

    req[reqLen++] = c;

    // First byte must be REQSTART, else reset the request
    if (1 == reqLen && REQSTART != c) {
      reqLen = 0;

    // If last byte is REQEND, dispatch the request
    } else if (REQLEN == reqLen) {
      reqLen = 0;
      if (REQEND == c)
        TestRun(req[1], req[2] | (req[3] << 8));
    }
  }
}


void encoderAnalyzer(void);
int main(void)
{
#ifndef DO_MOTOR_TESTING
  u32 failedTransferCount = 0;
#endif

  // Initialize the hardware peripherals
  BatteryInit();
  TimerInit();
  MotorsInit();   // Must init before power goes on
#if defined(DO_MOTOR_TESTING)
  UARTInit();
#endif

  UARTPutString("\r\nUnbrick me now...");
  u32 t = GetCounter();
  while ((GetCounter() - t) < 500 * COUNT_PER_MS)  // 0.5 second unbrick time
    ;
  UARTPutString("too late!\r\n");

  // Finish booting up
#ifndef DO_MOTOR_TESTING
  SPIInit();
#endif
  PowerOn();

  g_dataToHead.common.source = SPI_SOURCE_BODY;
  g_dataToHead.tail = 0x84;

  // Status LED hack
//  nrf_gpio_cfg_output(PIN_LED1);
//  nrf_gpio_cfg_output(PIN_LED2);
  // LED1 set means on, LED1 clear means off
//  nrf_gpio_pin_clear(PIN_LED2);
//  nrf_gpio_pin_set(PIN_LED1);

#if defined(DO_MOTOR_TESTING)
  // Motor testing, loop forever
//  nrf_gpio_pin_clear(PIN_LED1);
//  nrf_gpio_pin_set(PIN_LED2);
	MicroWait(2000000);
  while (1)
  {
		#ifdef DO_FIXED_DISTANCE_TESTING
			MicroWait(3000000);
			while(DebugWheelsGetTicks(0) <= 3987) // go 0.25m
			{
				MotorsSetPower(0, 0x2000);
				MotorsSetPower(1, 0x2000);
				MotorsUpdate();
				MicroWait(5000);
				MotorsUpdate();
			}
			MotorsSetPower(0, 0);
			MotorsSetPower(1, 0);
			MotorsUpdate();
			MicroWait(5000);
			MotorsUpdate();

			MicroWait(3000000);
			while(DebugWheelsGetTicks(0) >= 0) // go back 0.25m
			{
				MotorsSetPower(0, -0x2000);
				MotorsSetPower(1, -0x2000);
				MotorsUpdate();
				MicroWait(5000);
				MotorsUpdate();
			}
			MotorsSetPower(0, 0);
			MotorsSetPower(1, 0);
			MotorsUpdate();
			MicroWait(5000);
			MotorsUpdate();
		#endif

    UARTPutString("\r\nForward ends with...");
    #ifndef DO_GEAR_RATIO_TESTING
      MotorsSetPower(2, 0x6800);
			/*for (int i = 0; i < 2; i++)
				MotorsSetPower(i, 0x4000);
			for (int i = 2; i < 4; i++)
				MotorsSetPower(i, 0x6800);*/
		#endif
    MotorsUpdate();
    MicroWait(5000);
    MotorsUpdate();

    MicroWait(500000);
    MotorsPrintEncodersRaw();

    UARTPutString("\r\nBackward ends with...");
    #ifndef DO_GEAR_RATIO_TESTING
    MotorsSetPower(2, -0x6800);
    /*
			for (int i = 0; i < 2; i++)
				MotorsSetPower(i, -0x4000);
			for (int i = 2; i < 4; i++)
				MotorsSetPower(i, -0x6800);
        */
    #endif
		MotorsUpdate();
    MicroWait(5000);
    MotorsUpdate();

    MicroWait(500000);
    MotorsPrintEncodersRaw();

    BatteryUpdate();
  }

#else
#ifdef DO_LIGHTS_TESTING	
	int j = 0;
	while(1)
	{
		for(int i = 0; i < 200; i++)
		{
			u32 timerStart = GetCounter();
			g_dataToBody.backpackColors[(j % 4)] = 0x00FF0000;
			g_dataToBody.backpackColors[(j+1) % 4] = 0x0000FF00;
			g_dataToBody.backpackColors[(j+2) % 4] = 0x000000FF;
			g_dataToBody.backpackColors[(j+3) % 4] = 0x00FF00FF;
			ManageLights(g_dataToBody.backpackColors);
			while ((GetCounter() - timerStart) < 41666)	;
		}
		j++;
	}
#endif	
  while (1)
  {
    u32 timerStart = GetCounter();
    g_dataToBody.common.source = SPI_SOURCE_CLEAR;

    // If we're not on the charge contacts, exchange data with the head board
    if (!IsOnContacts())
    {
      SPITransmitReceive(
        sizeof(GlobalDataToBody),
        (const u8*)&g_dataToHead,
        (u8*)&g_dataToBody);
    }
    else // If not, reset the spine system
    {
      SPISpokenTo = false;
    }

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
//      nrf_gpio_pin_set(PIN_LED1);   // Force LED on to indicate problems
//      nrf_gpio_pin_clear(PIN_LED2);   // Force LED on to indicate problems
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

//      static u8 s_blink = 0;
//      nrf_gpio_pin_clear(PIN_LED1);
//      nrf_gpio_pin_clear(PIN_LED2);
//      if (++s_blink > 40) {
//        nrf_gpio_pin_set(PIN_LED2);      
//        s_blink = 0;
 //     }
    }

    // Only call every loop through - not all the time
    MotorsUpdate();
    BatteryUpdate();
    ManageLights(g_dataToBody.backpackColors);

    // Update at 200Hz
    // 41666 ticks * 120 ns is roughly 5ms
    while ((GetCounter() - timerStart) < 41666)
      ;
  }
#endif
}
