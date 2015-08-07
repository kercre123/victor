#include "hal.h"

#ifdef USE_UART

#include "hal_uart.h"

#define SOURCE_TX      5
#define PIN_TX         (1 << SOURCE_TX)
#define GPIO_TX        P0


static const char hexLookup[16] = {"0123456789ABCDEF"};

void PutDec(char i) reentrant 
{
  if(i == -128) // XXX
  {
    i = -127;
  }
  if(i<0)
  {
    PutChar('-');
    i=-i;
  }
  if(i > 9)
  {
    PutDec(i/10);
  }
  PutChar((i%10)+'0');
  
}

void PutHex(char h)
{
  hal_uart_putchar(hexLookup[((h&0xF0)>>4)&0x0F]);
  hal_uart_putchar(hexLookup[h&0x0F]);
}

char PutChar(char c)
{
  hal_uart_putchar(c);
  return c;
}

void PutString(char *s)
{
  while(*s != 0)
    PutChar(*s++);
}

void InitUart()
{
  // Set output pin
  #ifdef USE_EVAL_BOARD
  PIN_OUT(P1DIR, (1 << 0));
  #else
  PIN_OUT(P0DIR, PIN_TX); 
  #endif
     // Initializes the UART
  hal_uart_init(UART_BAUD_57K6);
  
      // Enable global interrupts
  EA = 1; // XXX is this needed?
  
  PutString("\r\nUART!\r\n");
}
#endif
