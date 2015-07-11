#include "hal.h"

void StreamAccelerometer()
{
  #ifdef STREAM_ACCELEROMETER
  volatile s8 accData[3];
  //s8 accDataLast[3];
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
    putstring("\r\n");
    //ReadFifo();
    //memcpy(accDataLast, accData, sizeof(accData));
    //delay_ms(9); // 100 hz loop.
    /*delay_ms(8);
    delay_us(850);*/
   // delay_us(700); //500 hz loop
    delay_ms(2);
  }
  #endif
}


// Blink lights for each tap
void TapsTest()
{
  #ifdef DO_TAPS_TEST
  u8 tapCount = 0;
  InitAcc();
  //InitUART();
  while(1)
  {
    tapCount += GetTaps();
    LightOn(tapCount % 12);
    delay_ms(25);
      // Initialize watchdog watchdog
  WDSV = 0; // 2 seconds to get through startup
  WDSV = 1;  
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
  // Do startup light sequence //
  u8 i;
  for(i = 0; i<12; i++)
  {
    LightOn(i);
    delay_ms(50);
  }
  LightsOff();
  SelfTest();
  StreamAccelerometer();
  TapsTest();
  SimpleLedTest(true);
  LedTest();
}
