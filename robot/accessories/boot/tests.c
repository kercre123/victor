#include "hal.h"

// Simple 0.77ms delay routine - before timers/beats are running
void Sleep()
{
  u8 i = 0, j = 16;    // 16*771 cycles = 0.77ms
  do
    do {} while (--i);  // 768+3 cycles
  while (--j);
}

// Flash the red/green/blue LED sequence indicating a successful boot
void LedTest()
{
  // On power-up (first reset), light LEDs: "red", "green", "blue", "IR"
  u8 i, j;
  if (!RSTREAS)
  {
    i = j = 0;
    do {
      do {
        LightOn((i & 3) | j);
        Sleep();
        LightsOff();
        Sleep();
      } while (--i);  // About 0.4 sec (256*2*0.77ms), 162Hz blink rate per LED
      j+=4;
    } while (j != 16);
    
    // Blink the accessory number, but only on cubes
    if (IsCharger())
      return;    
    do {
      if ((i & 3) < GetModel())
        LightOn((i & 3) | 4);   // Use green LEDs
      Sleep();
      LightsOff();
      Sleep();
    } while (--i);    // About 0.4 sec (256*0.77ms)
  }
  RSTREAS = 0;
}

// Run startup self-tests for fixture, accelerometer, and LEDs
// Fixture simply monitors startup current to test accelerometer and LEDs
void RunTests()
{ 
  // Turn on a red LED to indicate a reboot - making watchdog reboots more visible
  LightOn(2);
  
  // If accelerometer is okay, blink the LEDs red, green, and blue
  // Note:  This often fails because IMU is jammed in sleep mode
  if (AccelInit())
    LedTest();
}
