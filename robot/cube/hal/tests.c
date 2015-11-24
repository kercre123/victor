#include "hal.h"

#ifdef HUMAN_READABLE 
#define ACC_PRINT(x)            PutDec((x))
#else
#define ACC_PRINT(x)            PutHex((x))
#endif

void StreamAccelerometer()
{
  #ifdef STREAM_ACCELEROMETER
  volatile s8 accData[6];
  InitUart();
  InitAcc();
  while(1)
  {
    ReadAcc(accData);  
    if((accData[0] & 0x01) && (accData[2] & 0x01) && (accData[4] & 0x01)) // is all data new?
    {
      ACC_PRINT(accData[1]);
      ACC_PRINT(accData[3]);
      ACC_PRINT(accData[5]);
      PutString("\r\n");
    }
    //#define VERIFY_STREAMING_RATE
    #ifdef VERIFY_STREAMING_RATE
    else
    {
      PutChar('.');
    }
    #endif
    
  }
  #endif
}


// Blink lights for each tap
void TapsTest()
{
  #ifdef DO_TAPS_TEST
  u8 tapCount = 0, x = 0;
  u8 taps;
  u8 tapCountDelay[4] = {0,0,0,0};
  InitAcc();

  //InitUART();
  while(1)
  {
    #if 1
    tapCount += GetTaps();
    delay_ms(30);
    if(++x & 31)
      LightOn(tapCount % 12);
    #elif 0
    tapCount += GetTaps();
    LightOn(tapCount % 12);
    delay_ms(30);
    #else
    taps = GetTaps();
    tapCount += taps;
    delay_ms(30);
    LightOn(tapCountDelay[0] % 12);
    tapCountDelay[0] = tapCountDelay[1];
    tapCountDelay[1] = tapCountDelay[2];
    tapCountDelay[2] = tapCountDelay[3];
    tapCountDelay[3] = tapCount;
    #endif

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
  loop++;
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

static void RunStartupLights()
{
  // Do startup light sequence //
  // All R, G, B, 0.5 sec each
  u8 i,j;
  for(j = 0; j<3; j++)
  {
    for(i = 0; i<124; i++)
    {
      LightOn(0+j);
      delay_ms(1);
      LightOn(3+j);
      delay_ms(1);
      LightOn(6+j);
      delay_ms(1);
      LightOn(9+j);
      delay_ms(1);
    }
  }
  // All IR 0.5s
  for(i = 0; i<124; i++)
  {
    LightOn(12);
    delay_ms(1);
    LightOn(13);
    delay_ms(1);
    LightOn(14);
    delay_ms(1);
    LightOn(15);
    delay_ms(1);
  }
  // Cube ID
  j = (CBYTE[0x3FF0] & 0x3)+1;
  for(i = 0; i<124; i++)
  {
    LightOn(1);
    delay_ms(1);
    LightsOff();
    if(j==2)
      LightOn(4);
    delay_ms(1);
    LightsOff();
    if(j==3)
      LightOn(7);
    delay_ms(1);
    LightsOff();
    if(j==4)
      LightOn(10);
    delay_ms(1);
    LightsOff();
  }
  // All off
  LightsOff();

  
}

// Run tests
void RunTests()
{
  RunStartupLights();
  SelfTest();
  StreamAccelerometer();
  TapsTest();
  SimpleLedTest(true);
  LedTest();
}
