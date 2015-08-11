#include <stdarg.h>

#include "debug.h"
#include "nrf51.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "head.h"

#include "hardware.h"

static const u8 hex_chars[] = "0123456789ABCDEF";

int UART::get() {
  // We are currently transmitting to / from the head
  if (!Head::uartIdle) {
    return -1;
  }

  // Configure for read
  NRF_UART0->PSELRXD = PIN_TX_VEXT;
  NRF_UART0->PSELTXD = 0xFFFFFFFF;
  nrf_gpio_cfg_input(PIN_TX_VEXT, NRF_GPIO_PIN_NOPULL);
  DISABLE_UART_IRQ;

  // Wait for reception
  while (!NRF_UART0->EVENTS_RXDRDY) ;

  // Return data if the head didn't interrupt us
  return NRF_UART0->RXD;
}

static inline void put(unsigned char c) {
  NRF_UART0->TXD=c;
  while (!NRF_UART0->EVENTS_TXDRDY);
  NRF_UART0->EVENTS_TXDRDY = 0; 
}

// WARNING: THIS ONLT CHECKS TO SEE IF THE HEAD IS IDLE AT THE START
// THIS IS NOT SAFE TO USE DURING AN IRQ
void UART::print( const char* fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);

  // We are currently transmitting to / from the head
  if (!Head::uartIdle) {
    va_end(vl);
    return ;
  }

  // Configure port for transmission
  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->PSELTXD = PIN_TX_VEXT;
  nrf_gpio_cfg_output(PIN_TX_VEXT);  
  DISABLE_UART_IRQ;

  while(*fmt) {
    char ch = *(fmt++);

    // Basic character
    if (ch != '%') {
      put(ch);
      continue ;
    }

    // Prefix length value (partial implementation)
    int minlen = 0;
    while(*fmt >= '0' && *fmt <= '9') {
      minlen = (minlen * 10) + *(fmt++) - '0';
    }
    
    ch = *(fmt++);
    if (!ch) { break ; }  // Prematurely terminated string
    
    int tmp, len;
    char buff[10];
    bool show;

    switch(ch) {    
      // Character types
      case 'x':
        show = false;
        tmp = va_arg(vl, int);

        for (int i = 28; i >= 0; i -= 4) {
          int nybble = (tmp >> i) & 0xF;
          if (nybble || show || i < minlen*4) {
            put(hex_chars[nybble]);
            show = true;
          }
        }
        break ;
      
      case 'i':
        tmp = va_arg(vl, int);
        len = 0;
      
        if (tmp < 0) {
          put('-');
          tmp = -tmp;
        }
        
        if (!tmp) {
          put('0');
          break ;
        }

        while((tmp || len < minlen) && len < sizeof(buff)) {
          buff[len++] = tmp % 10;
          tmp /= 10;
        }
        
        while (len--) put(buff[len]+'0');
        break ;
      
      case 'c':
        put(va_arg(vl, int));
        break ; 
    }
  }
  
  va_end( vl);
}
