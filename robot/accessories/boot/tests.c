#include "hal.h"

// Simple 1.54ms delay routine - before timers/beats are running
void Sleep()
{
  u8 i = 0, j = 32;    // 32*771 cycles = 1.54ms
  do
    do {} while (--i);  // 768+3 cycles
  while (--j);
}

// Flash the red/green/blue LED sequence indicating a successful boot
void LedTest()
{
  // On power-up (first reset), light LEDs: "red", "green", "blue", "IR"
  u8 i = 0, j;
  if (!RSTREAS)
  {
    RSTREAS = 0;
    j = 0;
    do {
      do {
        LightOn((i & 3) | j);
        Sleep();
        LightsOff();
      } while (--i);  // About 0.4 sec (256*1.54ms)
      j+=4;
    } while (j != 16);
    
    // Then blink the accessory number
    do {
      if ((i & 3) < GetModel())
        LightOn((i & 3) | 4);   // Use green LEDs
      Sleep();
      LightsOff();
    } while (--i);    // About 0.4 sec (256*1.54ms)
  }
}

// Run startup self-tests for fixture, accelerometer, and LEDs
void RunTests()
{ 
  // Turn on a red LED to indicate a reboot - making watchdog reboots more visible
  LightOn(2);
  
  // XXX: TODO FEP - figure out how in-fixture radio test will work

  // If accelerometer is okay, blink the LEDs red, green, and blue
  if (AccelInit())
    LedTest();
}
