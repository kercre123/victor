#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "nrf51.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "head.h"

#include "hardware.h"

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

bool UART::waitIdle() {
  int start = GetCounter();
  
  while ((GetCounter() - start) < CYCLES_MS(35.0f / 7.0f)) {
    if (Head::uartIdle) return true;
  }
  
  return false;
}

// WARNING: THIS IS NOT SAFE TO USE DURING AN IRQ
void UART::print( const char* fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);

  // Block until the UART is available to print stuff (simply abort on timeout)
  if (!waitIdle()) {
    va_end(vl);
    return ;
  }

  char out[256];
  char *p = out;
   
  // Initialize the UART for the specified baudrate
  NRF_UART0->BAUDRATE = DEBUG_BAUDRATE;

  // Configure port for transmission
  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->PSELTXD = PIN_TX_VEXT;
  nrf_gpio_cfg_output(PIN_TX_VEXT);
  DISABLE_UART_IRQ;

  vsnprintf(out, sizeof(out), fmt, vl);
  while (*p) put(*(p++));
}

void UART::dump(int count, char* data) {
  const char hex[] = "0123456789ABCDEF";
  while(count-- > 0) {
    char ch = *(data++);
    put(hex[ch>>4]);
    put(hex[ch&0xF]);
  }
}
