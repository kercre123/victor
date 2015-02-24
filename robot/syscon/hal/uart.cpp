#include "uart.h"
#include "nrf51.h"
#include "nrf_gpio.h"
#include "timer.h"

// It looks like we might not be able to go above 1Mbaud with this chip.
#define NRF_BAUD(x) (int)(x * 268.435456)   // 2^28/1MHz
const u32 UART_BAUDRATE = NRF_BAUD(1000000);

const u8 PIN_TX = 1;   // 3.0

bool m_isInitialized = false;

void UARTInit()
{
  // Power on the peripheral
  NRF_UART0->POWER = 1;

  // Initialize the UART for the specified baudrate
  NRF_UART0->BAUDRATE = UART_BAUDRATE;
  
  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;
  
  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
 
  // Transmit is disabled by default
  UARTSetTransmit(0);
  
  m_isInitialized = true;
}

// Set/clear transmit mode - you can't receive while in transmit mode
static u8 m_transmitting = 0;
void UARTSetTransmit(u8 doTransmit)
{
  // Connect the peripheral to the pin and change directions
  if (doTransmit) {
    NRF_UART0->PSELTXD = PIN_TX;
    NRF_UART0->PSELRXD = 0xFFFFffff;
    NRF_UART0->TASKS_STARTTX = 1;
    nrf_gpio_cfg_output(PIN_TX);
  } else {
    nrf_gpio_cfg_input(PIN_TX, NRF_GPIO_PIN_PULLUP);
    NRF_UART0->TASKS_STARTRX = 1;
    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->PSELRXD = PIN_TX;
    NRF_UART0->PSELTXD = 0xFFFFffff; 
  }
  // Leave time for turnaround (about two bytes worth)
  MicroWait(20);
  m_transmitting = doTransmit;
}

void UARTPutChar(u8 c)
{
  if (!m_isInitialized)
    return;
  if (!m_transmitting)
    UARTSetTransmit(1);
  
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

void UARTPutDec(s32 value)
{
  if (value < 0) {
    UARTPutChar('-');
    value = -value;
  }
  if (value > 9)
    UARTPutDec(value / 10);
  UARTPutChar((value % 10) + '0');
}

int UARTGetChar()
{
  if (m_transmitting)
    UARTSetTransmit(0);
  
  // Wait for data to be received
  if (NRF_UART0->EVENTS_RXDRDY != 1)
    return -1;
  
  NRF_UART0->EVENTS_RXDRDY = 0;
  return (u8)NRF_UART0->RXD;
}
