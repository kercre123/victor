#include "uart.h"
#include "nrf51.h"
#include "nrf_gpio.h"

namespace
{
  // It looks like we might not be able to go above 1Mbaud with this chip.
  const u32 UART_BAUDRATE = 1000000;

  const u8 PIN_TX = 17;
  
  bool m_isInitialized = false;
}

void UARTInit()
{
  // Configure the pins
  nrf_gpio_cfg_output(PIN_TX);
  //nrf_gpio_cfg_input(PIN_RX, NRF_GPIO_PIN_NOPULL);

  // Power on the peripheral
  NRF_UART0->POWER = 1;
  
  // Configure the peripheral for the physical pins are being used for UART
  NRF_UART0->PSELTXD = 0xFFFFffff; //PIN_TX;
  NRF_UART0->PSELRXD = 0xFFFFffff;  // Disconnect RX

  // Initialize the UART for the specified baudrate
  NRF_UART0->BAUDRATE = UART_BAUDRATE * 268;
  
  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;
  
  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->TASKS_STARTTX = 1;
  //NRF_UART0->TASKS_STARTRX = 1;
  //NRF_UART0->EVENTS_RXDRDY = 0;
  
  m_isInitialized = true;
}

void UARTPutChar(u8 c)
{
  if (!m_isInitialized)
    return;
  
  NRF_UART0->TXD = (u8)c;

  // Wait for the data to transmit
  while (NRF_UART0->EVENTS_TXDRDY != 1)
    ;
  
  NRF_UART0->EVENTS_TXDRDY = 0;
}

void UARTPutString(const char* s)
{
  while (*s)
    UARTPutChar(*s++);
}

void UARTPutHex(u8 c)
{
  static u8 hex[] = "0123456789ABCDEF";
  UARTPutChar(hex[c >> 4]);
  UARTPutChar(hex[c & 0xF]);
}

void UARTPutHex32(u32 value)
{
  UARTPutHex(value >> 24);
  UARTPutHex(value >> 16);
  UARTPutHex(value >> 8);
  UARTPutHex(value);
}

/*int UARTGetChar(u32 timeout)
{
  u32 now = GetMicroCounter();
  // Wait for data to be received
  while (NRF_UART0->EVENTS_RXDRDY != 1)
  {
    // Check for timeout
    if ((GetMicroCounter() - now) >= timeout)
    {
      return -1;
    }
  }
  
  NRF_UART0->EVENTS_RXDRDY = 0;
  return (u8)NRF_UART0->RXD;
}*/
