#include "string.h"

#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"

#include "hardware.h"

#include "anki/cozmo/robot/spineData.h"

#define MAX(a, b) ((a > b) ? a : b)

static const uint8_t HEAD_PREAMBLE[] = {SPI_SOURCE_HEAD, 0xFA, 0xF3, 0x20};

uint8_t TxRxBuffer[sizeof(GlobalDataToBody)];
bool Head::spokenTo;
volatile bool Head::uartIdle;
int TxRxIdx;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

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

  // Extremely low priorty IRQ
  NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Msk | UART_INTENSET_RXDRDY_Msk;
  NVIC_SetPriority(UART0_IRQn, 0);
  NVIC_EnableIRQ(UART0_IRQn);
  
  Head::uartIdle = true;
  Head::spokenTo = false;
}

// Transmit first, then wait for a reply
void Head::TxRx()
{
  NRF_UART0->EVENTS_RXDRDY = 0;
  NRF_UART0->EVENTS_TXDRDY = 0;
  ENABLE_UART_IRQ;
  
  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->PSELTXD = PIN_TX_HEAD;
  nrf_gpio_cfg_output(PIN_TX_HEAD);

  // Setup Buffers
  memcpy(TxRxBuffer, &g_dataToHead, sizeof(g_dataToHead));
  TxRxIdx = 0;
  Head::uartIdle = false;
  
  // Transmit first byte
  MicroWait(40);
  NRF_UART0->TXD = TxRxBuffer[TxRxIdx++];
}

extern "C"
void UART0_IRQHandler()
{
  // We transmitted a byte
  if (NRF_UART0->EVENTS_TXDRDY) {
    NRF_UART0->EVENTS_TXDRDY = 0;

    
    if (TxRxIdx < sizeof(g_dataToHead)) {
      NRF_UART0->TXD = TxRxBuffer[TxRxIdx++];
    } else {
      TxRxIdx = 0;

      nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);
      NRF_UART0->PSELTXD = 0xFFFFFFFF;  // Disconnect TX

      // Wait 10uS for turnaround - 80uS on the other side
      MicroWait(10);
      NRF_UART0->PSELRXD = PIN_TX_HEAD;
    }
  }

  // We received a byte
  if (NRF_UART0->EVENTS_RXDRDY) {
    NRF_UART0->EVENTS_RXDRDY = 0;

    TxRxBuffer[TxRxIdx] = NRF_UART0->RXD;
    
    if (TxRxIdx < sizeof(HEAD_PREAMBLE) && HEAD_PREAMBLE[TxRxIdx] != TxRxBuffer[TxRxIdx]) {
      TxRxIdx = 0;
      return ;
    }
    
    TxRxIdx++;
    
    if (TxRxIdx >= sizeof(g_dataToBody)) {
      memcpy(&g_dataToBody, TxRxBuffer, sizeof(g_dataToBody));
      Head::spokenTo = true;
      Head::uartIdle = true;
    }
  }
}
