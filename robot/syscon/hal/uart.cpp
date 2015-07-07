#include "uart.h"
#include "nrf51.h"
#include "nrf_gpio.h"
#include "timer.h"

#include "hardware.h"

bool UART::initialized = false;
bool m_isTransmit = false;

void UART::configure()
{
  if (m_isTransmit) {
    NRF_UART0->PSELRXD = 0xFFFFFFFF;
    NRF_UART0->PSELTXD = PIN_TX_DEBUG;
    nrf_gpio_cfg_output(PIN_TX_DEBUG);  
  } else {
    NRF_UART0->PSELRXD = PIN_TX_DEBUG;
    NRF_UART0->PSELTXD = 0xFFFFFFFF;
    nrf_gpio_cfg_input(PIN_TX_DEBUG, NRF_GPIO_PIN_NOPULL);
  }
}

// Set/clear transmit mode - you can't receive while in transmit mode
void UART::setTransmit(bool doTransmit)
{
  if (m_isTransmit == doTransmit) {
    return ;
  }
  
  m_isTransmit = doTransmit;
  UART::configure();
}

int UART::get()
{
  if(!UART::initialized) {
    return -1;
  }

  UART::setTransmit(false);
  
  // Wait for data to be received
  if (NRF_UART0->EVENTS_RXDRDY != 1)
    return -1;
  
  NRF_UART0->EVENTS_RXDRDY = 0;
  return (u8)NRF_UART0->RXD;
}

void UART::put(uint8_t c)
{
  if(!UART::initialized) {
    return ;
  }

  UART::setTransmit(true);
  NRF_UART0->TXD = c;

  // Wait for the data to transmit
  while (NRF_UART0->EVENTS_TXDRDY != 1)
    ;
  
  NRF_UART0->EVENTS_TXDRDY = 0;
}

void UART::put(const char* s)
{
  while (*s)
    UART::put(*s++);
}

void UART::dump(const uint8_t* data, uint32_t len) {
  while(len--) { UART::put(' '); UART::hex(*(data++)); }
}

void UART::hex(uint8_t c)
{
  static const u8 hex[] = "0123456789ABCDEF";
  UART::put(hex[c >> 4]);
  UART::put(hex[c & 0xF]);
}

void UART::hex(uint32_t value)
{
  for (int i = 24; i >= 0; i -= 8) {
    UART::hex((u8)(value >> i));
  }
}

void UART::dec(int value)
{
  if (value < 0) {
    UART::put('-');
    value = -value;
  }
  if (value > 9)
    UART::dec(value / 10);
  UART::put((value % 10) + '0');
}
