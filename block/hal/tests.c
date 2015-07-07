#include "hal.h"
#ifdef USE_UART
#include "hal_uart.h"
#endif
#define SOURCE_TX      5
#define PIN_TX         (1 << SOURCE_TX)
#define GPIO_TX        P0

#ifdef USE_UART
const char hexLookup[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void puthex(char h)
{
  hal_uart_putchar(hexLookup[((h&0xF0)>>4)&0x0F]);
  hal_uart_putchar(hexLookup[h&0x0F]);
}

char putchar(char c)
{
  hal_uart_putchar(c);
  return c;
}

void putstring(char *s)
{
  while(*s != 0)
    putchar(*s++);
}

void InitUART()
{
  // Set output pin
  PIN_OUT(P0DIR, PIN_TX); 
     // Initializes the UART
  hal_uart_init(UART_BAUD_57K6);
  
      // Enable global interrupts
  EA = 1; // XXX is this needed?
}

void StreamAccelerometer()
{
  volatile s8 accData[3];
  s8 accDataLast[3];
  InitUART();
  InitAcc();

    putchar('\r');
    putchar('\n');
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
    putchar('\r');
    putchar('\n');
    memcpy(accDataLast, accData, sizeof(accData));
    delay_us(700); // 50 hz loop.
  }
  
}
#endif

// Blink lights for each tap
void TapsTest()
{
  char tapCount = 0;
  InitAcc();
  while(1)
  {
    tapCount += GetTaps();
    LightOn(tapCount % 12);
    delay_us(1500);
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

  #ifdef STREAM_ACCELEROMETER
  StreamAccelerometer();
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