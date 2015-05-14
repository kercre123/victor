#include "spi.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "spi_master.h"

#define NRF_BAUD(x) (int)(x * 268.435456)   // 2^28/1MHz

const u32 UART_BAUDRATE = NRF_BAUD(350000);
const u8 PIN_TX = 2;   // 3.0

// Don't speak until spoken to (part of handshaking)
bool SPISpokenTo;

void SPIInit()
{
  SPISpokenTo = false;

  // Power on the peripheral
  NRF_UART0->POWER = 1;

  // Initialize the UART for the specified baudrate
  // Mike noticed the baud rate is almost rate*268 - it's actually (2^28 / 1MHz)
  NRF_UART0->BAUDRATE = UART_BAUDRATE;

  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;

  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->TASKS_STARTTX = 1;
  NRF_UART0->TASKS_STARTRX = 1;
  NRF_UART0->EVENTS_RXDRDY = 0;
}

// Transmit first, then wait for a reply
void SPITransmitReceive(u16 length, const u8* dataTX, u8* dataRX)
{
  // If spoken to, switch to transmit mode and send the data
  if (SPISpokenTo)
  {
    MicroWait(80);    // Other side waits 40uS
    NRF_UART0->PSELRXD = 0xFFFFffff;  // Disconnect RX
    NRF_UART0->PSELTXD = PIN_TX;
    nrf_gpio_cfg_output(PIN_TX);
    for (int i = 0; i < length; i++)
    {
      NRF_UART0->TXD = dataTX[i];
      while (NRF_UART0->EVENTS_TXDRDY != 1)
        {}
      NRF_UART0->EVENTS_TXDRDY = 0;   // XXX: Needed?
    }
  }

  // Switch to receive mode and wait for a reply
  nrf_gpio_cfg_input(PIN_TX, NRF_GPIO_PIN_NOPULL);
  NRF_UART0->PSELTXD = 0xFFFFffff;  // Disconnect TX

  // Wait 10uS for turnaround - 80uS on the other side
  MicroWait(10);
  NRF_UART0->PSELRXD = PIN_TX;

  u32 startTime = GetCounter();

  NRF_UART0->EVENTS_RXDRDY = 0;   // XXX: Needed?

  dataRX[0] = 0;

  int i = 0;
  while (i < length)
  {
    // Timeout after 5ms of no communication
    while (NRF_UART0->EVENTS_RXDRDY != 1)
      if (SPISpokenTo && GetCounter() - startTime > 41666*2) // 5ms
        return;
    NRF_UART0->EVENTS_RXDRDY = 0;   // XXX: Needed?
    u8 byte = NRF_UART0->RXD;
    switch (i) {
      case 0:
      {
        i = (byte == 'H') ? 1 : 0;
        break;
      }
      case 1:
      {
        i = (byte == 0xFA) ? 2 : 0;
        break;
      }
      case 2:
      {
        i = (byte == 0xF3) ? 3 : 0;
        break;
      }
      case 3:
      {
        i = (byte == 0x20) ? 4 : 0;
        break;
      }
      default:
      {
        dataRX[i++] = byte;
      }
    }
  }
  dataRX[0] = 'H';

  // Wait before first reply
  if (!SPISpokenTo) {
    MicroWait(5000);
    SPISpokenTo = true;
  }
}
