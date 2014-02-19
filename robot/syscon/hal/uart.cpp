#include "uart.h"
#include "anki/cozmo/robot/hal.h"
#include "nrf51.h"
#include "nrf_gpio.h"

namespace
{
  // It looks like we might not be able to go above 1Mbaud with this chip.
  const u32 UART_BAUDRATE = 1000000;

  const u8 PIN_TX = 21;
  const u8 PIN_RX = 23;
}

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      int UARTPrintf(const char* format, ...)
      {
        return 0;
      }
      
      int UARTPutChar(int c)
      {
        NRF_UART0->TXD = (u8)c;

        // Wait for the data to transmit
        while (NRF_UART0->EVENTS_TXDRDY != 1)
          ;
        
        NRF_UART0->EVENTS_TXDRDY = 0;
  
        return c;
      }
      
      void UARTPutString(const char* s)
      {
        while (*s)
          UARTPutChar(*s++);
      }
      
      int UARTGetChar(u32 timeout)
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
      }
    }
  }
}

void UARTInit()
{
  // Configure the pins
  nrf_gpio_cfg_output(PIN_TX);
  nrf_gpio_cfg_input(PIN_RX, NRF_GPIO_PIN_NOPULL);

  // Power on the peripheral
  NRF_UART0->POWER = 1;
  
  // Configure the peripheral for the physical pins are being used for UART
  NRF_UART0->PSELTXD = PIN_TX;
  NRF_UART0->PSELRXD = PIN_RX;

  // Initialize the UART for the specified baudrate
  NRF_UART0->BAUDRATE = UART_BAUDRATE * 268;
  
  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;
  
  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->TASKS_STARTTX = 1;
  NRF_UART0->TASKS_STARTRX = 1;
  NRF_UART0->EVENTS_RXDRDY = 0;
}
