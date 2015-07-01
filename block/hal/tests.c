#include "hal.h"



// Blink lights for each tap
void TapsTest()
{
  char tapCount = 0;
  InitAcc();
  while(1)
  {
    tapCount += GetTaps();
    LightOn(tapCount % 12);
    delay_ms(30);
  }
}


// Blink lights
void SimpleLedTest(bool loop)
{
  char i;
  while(loop)
  {
    for(i = 0; i<13; i++)
    {
      LightOn(i);
      delay_ms(333);
    }
  }
}


// Test light controller
void LedTest()
{
  u8 i;
  InitTimer2();
  for(i = 0; i<13; i++)
  {
    SetLedValue(i,0);
  }
  SetLedValue(12,255);
  while(1)
  {
    
    for(i=0; i<255; i++)
    {
      SetLedValue(3, i);
      SetLedValue(7, i);
      SetLedValue(11, i);
      delay_ms(5);
    }
    for(i=0; i<255; i++)
    {
      SetLedValue(3, 0xFF-i);
      SetLedValue(7, 0xFF-i);
      SetLedValue(11, 0xFF-i);
      delay_ms(5);
    }
  }
}


// Test LEDs visually, test accelerometer communication, test radio
void SelfTest()
{
  // Blink lights
  SimpleLedTest(false);
  // Check accelerometer
  InitAcc();
  // Try radio
  // TODO
}


// Run tests
void RunTests()
{
  #ifdef DO_SELF_TEST
  SelfTest();
  #endif
  
  #ifdef DO_TAPS_TEST
  TapsTest();
  #endif
  
  #ifdef DO_SIMPLE_LED_TEST
  SimpleLedTest(true);
  #endif
  
  #ifdef DO_LED_TEST
  LedTest();
  #endif
}