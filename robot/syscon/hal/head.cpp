#include "string.h"

#include "debug.h"
#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"

#include "radio.h"

#include "hardware.h"

#include "anki/cozmo/robot/spineData.h"

#define MAX(a, b) ((a > b) ? a : b)

static union {
  uint8_t txRxBuffer[MAX(sizeof(GlobalDataToBody), sizeof(GlobalDataToHead))];
  uint32_t  rxHeader;
};

static const int DEBUG_BYTES = 32;

static int txRxIndex;
static int debugSafeWords;
static bool transmitting;

bool Head::spokenTo;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

extern void EnterRecovery(void);
static void setTransmit(bool tx);

void Head::init() 
{
  // Sync pattern
  g_dataToHead.source = SPI_SOURCE_BODY;
  Head::spokenTo = false;
  txRxIndex = 0;

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

  // Extremely low priorty IRQ
  NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Msk | UART_INTENSET_RXDRDY_Msk;
  NVIC_SetPriority(UART0_IRQn, 1);
  NVIC_EnableIRQ(UART0_IRQn);

  // We begin in receive mode (slave)
  setTransmit(false);
  MicroWait(80);
}

static void setDebugTransmit() {
  if (!UART::DebugQueue()) return ;

  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->PSELTXD = PIN_TX_VEXT;

  // Configure pin so it is open-drain
  nrf_gpio_cfg_output(PIN_TX_HEAD);

  // Clear our UART interrupts
  NRF_UART0->EVENTS_RXDRDY = 0;
  NRF_UART0->EVENTS_TXDRDY = 0;
  
  // We are in debug transmit mode, these are the safe bytes
  debugSafeWords = DEBUG_BYTES;
  transmitting = false;
  
  NRF_UART0->TXD = UART::DebugChar();
}

static void setTransmit(bool tx) {
  // Set UART half-duplex pin mapping and i/o type
  if (tx) {
    // Prevent debug words from transmitting
    debugSafeWords = 0;
    transmitting = true;

    NRF_UART0->PSELRXD = 0xFFFFFFFF;
    NRF_UART0->PSELTXD = PIN_TX_HEAD;

    // Configure pin so it is open-drain
    nrf_gpio_cfg_output(PIN_TX_HEAD);
  } else {
    nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD = 0xFFFFFFFF;
    MicroWait(10);
    NRF_UART0->PSELRXD = PIN_TX_HEAD;
  }

  // Clear our UART interrupts
  NRF_UART0->EVENTS_RXDRDY = 0;
  NRF_UART0->EVENTS_TXDRDY = 0;
}

inline void transmitByte() { 
  NRF_UART0->TXD = txRxBuffer[txRxIndex++];
}

void Head::manage(void) {
  memcpy(txRxBuffer, &g_dataToHead, sizeof(GlobalDataToHead));
  txRxIndex = 0;

  setTransmit(true);
  transmitByte();
}

extern "C"
void UART0_IRQHandler()
{
  // We received a byte
  if (NRF_UART0->EVENTS_RXDRDY) {
    NRF_UART0->EVENTS_RXDRDY = 0;

    txRxBuffer[txRxIndex] = NRF_UART0->RXD;

    // Re-sync to header
    if (txRxIndex < 4) {
      const uint32_t head_target = SPI_SOURCE_HEAD;
      const uint8_t header = head_target >> (txRxIndex * 8);

      if(txRxBuffer[txRxIndex] != header) {
        txRxIndex = 0;
        return ;
      }
    }

    // We received a full packet
    if (++txRxIndex >= 64) {
      txRxIndex = 0;
      memcpy(&g_dataToBody, txRxBuffer, sizeof(GlobalDataToBody));
      
      // Secret recovery flag, set dark byte to zero, and set secret to a magic number
      if (g_dataToBody.cubeStatus.secret == secret_code && g_dataToBody.cubeStatus.ledDark == 0) {
        EnterRecovery();
      } else {
        setDebugTransmit();
      }
    }
  }

  // We transmitted a byte
  if (NRF_UART0->EVENTS_TXDRDY) {
    NRF_UART0->EVENTS_TXDRDY = 0;

    if (transmitting) {
      // We are in regular head transmission mode
      if (txRxIndex >= sizeof(GlobalDataToHead)) {
        setTransmit(false);
      } else {
        transmitByte();
      }
    } else if (debugSafeWords-- > 0) {
      // We are stuffing debug words
      if (UART::DebugQueue()) {
        NRF_UART0->TXD = UART::DebugChar();
        return ;
      }
    }
  }
}
