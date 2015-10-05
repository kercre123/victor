#include "string.h"

#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"

#include "radio.h"

#include "hardware.h"

#include "anki/cozmo/robot/spineData.h"

#define MAX(a, b) ((a > b) ? a : b)

static uint8_t rxBuffer[sizeof(GlobalDataToBody)];
static uint8_t txBuffer[sizeof(GlobalDataToHead)];
static int rxIndex, txIndex;

bool Head::spokenTo;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

void setTransmit(bool tx) {
  if (tx) {
    NRF_UART0->PSELRXD = 0xFFFFFFFF;
    NRF_UART0->PSELTXD = PIN_TX_HEAD;

    // Configure pin so it is open-drain
    nrf_gpio_cfg_output(PIN_TX_HEAD);

    NRF_GPIO->PIN_CNF[PIN_TX_HEAD] = 
      (NRF_GPIO->PIN_CNF[PIN_TX_HEAD] & ~GPIO_PIN_CNF_DRIVE_Msk) | 
      (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos);
  } else {
    NRF_UART0->PSELRXD = PIN_TX_HEAD;
    NRF_UART0->PSELTXD = 0xFFFFFFFF;
    nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);
  }
}

void Head::init() 
{
  // Sync pattern
  g_dataToHead.common.source = SPI_SOURCE_BODY;

  Head::spokenTo = false;
  rxIndex = 0;
  txIndex = 0;

  // Power on the peripheral
  NRF_UART0->POWER = 1;

  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;

  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->TASKS_STARTTX = 1;
  NRF_UART0->TASKS_STARTRX = 1;

  // Initialize the UART for the specified baudrate
  NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

  // We begin in receive mode (slave)
  setTransmit(false);
  MicroWait(80);

  // Clear our UART interrupts
  NRF_UART0->EVENTS_RXDRDY = 0;
  NRF_UART0->EVENTS_TXDRDY = 0;

  // Extremely low priorty IRQ
  NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Msk | UART_INTENSET_RXDRDY_Msk;
  NVIC_SetPriority(UART0_IRQn, 1);
  NVIC_EnableIRQ(UART0_IRQn);
}

inline void transmitByte() {
  if(txIndex == 0) {
    memcpy(txBuffer, &g_dataToHead, sizeof(GlobalDataToHead));
  }

  NRF_UART0->TXD = txBuffer[txIndex];
  txIndex = (txIndex + 1) % sizeof(GlobalDataToHead);
}

extern "C"
void UART0_IRQHandler()
{
  // We received a byte
  if (NRF_UART0->EVENTS_RXDRDY) {
    NRF_UART0->EVENTS_RXDRDY = 0;

    rxBuffer[rxIndex] = NRF_UART0->RXD;
    
    const uint32_t target = SPI_SOURCE_HEAD;
    
    // Re-sync
    if (rxIndex < 4) {
      const uint8_t header = target >> (rxIndex * 8);
      
      if(rxBuffer[rxIndex] != header) {
        rxIndex = 0;
        return ;
      }
    }
        
    // We received a full packet
    if (++rxIndex >= 64) {
      rxIndex = 0;
      memcpy(&g_dataToBody, rxBuffer, sizeof(GlobalDataToBody));
    }

    // Turn around uart when we hit a chunk baby.
    if (rxIndex % uart_chunk_size == 0) {
      setTransmit(true);
      MicroWait(20);
      transmitByte();
    }
  }

  // We transmitted a byte
  if (NRF_UART0->EVENTS_TXDRDY) {
    NRF_UART0->EVENTS_TXDRDY = 0;

    if (txIndex % uart_chunk_size == 0) {
      setTransmit(false);
    } else {
      transmitByte();
    }
  }
}
