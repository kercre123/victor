#include "hal.h"

void StreamAccelerometer()
{
  #ifdef STREAM_ACCELEROMETER
  volatile s8 accData[3];
  s8 accDataLast[3];
  InitUart();
  InitAcc();

  while(1)
  {
    ReadAcc(accData);
    puthex(accData[0]);
    //puthex(accData[0]-accDataLast[0]);
    putchar('\t');
    puthex(accData[1]);
    //puthex(accData[1]-accDataLast[1]);
    putchar('\t');
    puthex(accData[2]);
    //puthex(accData[2]-accDataLast[2]);
    putstring('\r\n');
    memcpy(accDataLast, accData, sizeof(accData));
    delay_us(700); // 50 hz loop.
  }
  #endif
}


// Blink lights for each tap
void TapsTest()
{
  #ifdef DO_TAPS_TEST
  char tapCount = 0;
  InitAcc();
  while(1)
  {
    tapCount += GetTaps();
    LightOn(tapCount % 12);
    delay_us(1500);
  }
  #endif
}


// Blink lights
void SimpleLedTest(bool loop)
{
  #ifdef DO_SIMPLE_LED_TEST
  char i;
  while(loop)
  {
    for(i = 0; i<13; i++)
    {
      LightOn(i);
      delay_ms(333);
    }
  }
  #endif
}


// Test light controller
void LedTest()
{
  #ifdef DO_LED_TEST
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
  #endif
}

// Test LEDs visually, test accelerometer communication, test radio
void SelfTest()
{
  #ifdef DO_SELF_TEST
  // Blink lights
  SimpleLedTest(false);
  // Check accelerometer
  InitAcc();
  // Try radio
  // TODO
  #endif
}


// Run tests
void RunTests()
{
  SelfTest();
  StreamAccelerometer();
  TapsTest();
  SimpleLedTest(true);
  LedTest();
}
