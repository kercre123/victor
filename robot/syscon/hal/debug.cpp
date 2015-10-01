#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "nrf51.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "head.h"

#include "hardware.h"

int UART::get() {
  return ;

  // Configure for read
  NRF_UART0->PSELRXD = PIN_TX_VEXT;
  NRF_UART0->PSELTXD = 0xFFFFFFFF;
  nrf_gpio_cfg_input(PIN_TX_VEXT, NRF_GPIO_PIN_NOPULL);

  // Wait for reception
  while (!NRF_UART0->EVENTS_RXDRDY) ;

  // Return data if the head didn't interrupt us
  return NRF_UART0->RXD;
}

static inline void put(unsigned char c) {
  return ;

  // Configure port for transmission
  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->PSELTXD = PIN_TX_VEXT;
  nrf_gpio_cfg_output(PIN_TX_VEXT);

  NRF_UART0->TXD=c;
  while (!NRF_UART0->EVENTS_TXDRDY);
  NRF_UART0->EVENTS_TXDRDY = 0; 
}

void UART::print( const char* fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);

  char out[256];
  char *p = out;
   
  vsnprintf(out, sizeof(out), fmt, vl);
  while (*p) put(*(p++));
}

void UART::dump(int count, char* data) {
  while(count-- > 0) {
    print("%2x ", *(data++));
  }
}
