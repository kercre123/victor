#include "hal.h"

#ifdef USE_UART

#include "hal_uart.h"

#define SOURCE_TX      5
#define PIN_TX         (1 << SOURCE_TX)
#define GPIO_TX        P0


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

void InitUart()
{
  // Set output pin
  PIN_OUT(P0DIR, PIN_TX); 
     // Initializes the UART
  hal_uart_init(UART_BAUD_57K6);
  
      // Enable global interrupts
  EA = 1; // XXX is this needed?
  
  putstring("UART!\r\n");
}
#endif
