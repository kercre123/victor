#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "nrf51.h"
#include "nrf_gpio.h"
#include "timer.h"
#include "head.h"

#include "hardware.h"

static const int MAX_DEBUG_PRINT = 512;
static uint8_t debug_print_out[MAX_DEBUG_PRINT];
static int debug_print_first = 0;
static int debug_print_last = 0;
static int debug_print_count = 0;

int UART::DebugQueue() {
  return debug_print_count;
}

uint8_t UART::DebugChar() {
  if (debug_print_count <= 0) return -1;
  
  uint8_t o = debug_print_out[debug_print_first];
  debug_print_first = (debug_print_first+1) % MAX_DEBUG_PRINT;
  debug_print_count++;
  
  return o;
}

int UART::get() {
  return -1;

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
  if (debug_print_count >= MAX_DEBUG_PRINT) return ;
  
  debug_print_out[debug_print_last] = c;
  debug_print_last = (debug_print_last+1) % MAX_DEBUG_PRINT;
  debug_print_count++;
}

void UART::print( const char* fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);

  char buffer[MAX_DEBUG_PRINT];
  char* p = buffer;

  int buffered = vsnprintf(buffer, MAX_DEBUG_PRINT - debug_print_count, fmt, vl);
  while (buffered-- > 0) put(*(p++));
}

void UART::dump(int count, char* data) {
  const char hex[] = "0123456789ABCDEF";
  while(count-- > 0) {
    char ch = *(data++);
    put(hex[ch>>4]);
    put(hex[ch&0xF]);
  }
}
