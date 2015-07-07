#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "uart.h"

#include "hardware.h"

#include "anki/cozmo/robot/spineData.h"


// Don't speak until spoken to (part of handshaking)
bool Head::spokenTo;

void Blink() {
  nrf_gpio_pin_set(PIN_LED2);
  nrf_gpio_cfg_output(PIN_LED2);

  static bool toggle = false;
  toggle = !toggle;
  if (toggle) {
    nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
  } else {
    nrf_gpio_pin_clear(PIN_LED4);
    nrf_gpio_cfg_output(PIN_LED4);
  }
}

void Head::init()
{
  Head::spokenTo = false;

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

  UART::initialized = true;
  UART::configure();
}

void Head::TxPacket(u16 length, const u8* dataTX)
{
  MicroWait(80);    // Other side waits 40uS
  NRF_UART0->PSELRXD = 0xFFFFffff;  // Disconnect RX
  NRF_UART0->PSELTXD = PIN_TX_HEAD;
  nrf_gpio_cfg_output(PIN_TX_HEAD);

  for (int i = 0; i < length; i++)
  {
    NRF_UART0->TXD = dataTX[i];
    while (NRF_UART0->EVENTS_TXDRDY != 1) ;
    NRF_UART0->EVENTS_TXDRDY = 0;
  }
}

void Head::RxPacket(u16 length, u8* dataRX)
{
  u32 startTime = GetCounter();

   // Switch to receive mode and wait for a reply
  nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);
  NRF_UART0->PSELTXD = 0xFFFFFFFF;  // Disconnect TX

  // Wait 10uS for turnaround - 80uS on the other side
  MicroWait(10);
  NRF_UART0->PSELRXD = PIN_TX_HEAD;

  NRF_UART0->EVENTS_RXDRDY = 0;
 
  int i = 0;
  while (i < length)
  {
    // Timeout after 5ms of no communication
    while (NRF_UART0->EVENTS_RXDRDY != 1) {
      if (GetCounter() - startTime > CYCLES_MS(40.0f)) { // 4ms
        //Blink();
        dataRX[0] = 0;
        return;
      }
    }

    NRF_UART0->EVENTS_RXDRDY = 0; // XXX: Needed?
    dataRX[i] = NRF_UART0->RXD;

    // Header transmission header test
    const uint8_t preamble[] = {SPI_SOURCE_HEAD, 0xFA, 0xF3, 0x20};
    if (i < sizeof(preamble) && preamble[i] != dataRX[i]) {
      i = 0;
    } else {
      i++;
    }
  }

  // Wait before first reply
  Head::spokenTo = true;
}


// Transmit first, then wait for a reply
void Head::TxRx(u16 length, const u8* dataTX, u8* dataRX)
{
  Head::TxPacket(length, dataTX);
  Head::RxPacket(length, dataRX);
 
  UART::configure();
}
