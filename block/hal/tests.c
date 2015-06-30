#include "hal.h"

//#define DO_SIMPLE_LED_TEST
//#define DO_LED_TEST
//#define DO_TAPS_TEST
//#define DO_MISSED_PACKET_TEST
//#define DO_RADIO_LED_TEST

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
  char i;
  InitTimer2();
  for(i = 0; i<13; i++)
  {
    SetLedValue(i,0);
  }
  while(1)
  {
    for(i=0; i<255; i++)
    {
      SetLedValue(4, i);
      SetLedValue(3, 0xFF-i);
      SetLedValue(5, 0xFF-i);
      delay_ms(5);
    }
    for(i=255; i>0; i--)
    {
      SetLedValue(4, i);
      SetLedValue(3, 0xFF-i);
      SetLedValue(5, 0xFF-i);
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