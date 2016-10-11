#include "string.h"

#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "lights.h"
#include "storage.h"
#include "anki/cozmo/robot/crashLogs.h"

#include "cubes.h"
#include "watchdog.h"
#include "hardware.h"
#include "backpack.h"
#include "bluetooth.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

uint8_t txRxBuffer[MAX(MAX(sizeof(GlobalDataToBody), sizeof(GlobalDataToHead)), SPINE_CRASH_LOG_SIZE)];

enum TRANSMIT_MODE {
  TRANSMIT_UNKNOWN,
  TRANSMIT_SEND,
  TRANSMIT_RECEIVE,
};

static int txRxIndex;
static TRANSMIT_MODE uart_mode;

bool Head::spokenTo;
bool Head::synced;
bool lowPowerMode;

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

static void setTransmitMode(TRANSMIT_MODE mode);

void Head::init() 
{
  // Sync pattern
  memset(&g_dataToBody, 0, sizeof(g_dataToBody));
  memset(&g_dataToHead, 0, sizeof(g_dataToHead));
  g_dataToHead.source = SPI_SOURCE_BODY;
  Head::spokenTo = false;
  Head::synced = false;
  txRxIndex = 0;
  lowPowerMode = false;

  // Configure pin so it is open-drain
  NRF_GPIO->PIN_CNF[PIN_TX_HEAD] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                                | (GPIO_PIN_CNF_DRIVE_H0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                                | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                                | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

  // Power on the peripheral
  NRF_UART0->POWER = 1;

  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;

  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;

  // Extremely low priorty IRQ
  NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Msk | UART_INTENSET_RXDRDY_Msk;
  NVIC_SetPriority(UART0_IRQn, UART_PRIORITY);
  NVIC_EnableIRQ(UART0_IRQn);

  nrf_gpio_pin_set(PIN_TX_HEAD);
}

void Head::enterLowPowerMode(void) {
  using namespace Anki::Cozmo;
  
  EnterSleepMode msg;
  RobotInterface::SendMessage(msg);
  
  lowPowerMode = true;
}

void Head::leaveLowPowerMode(void) {
  // TODO: THIS IS INCOMPLETE
}

static void setTransmitMode(TRANSMIT_MODE mode) {
  if (uart_mode == mode) {
    return ;
  }

  // Shut it down (because reasons)
  NRF_UART0->TASKS_STOPRX = 1;
  NRF_UART0->TASKS_STOPTX = 1;
    
  uart_mode = mode;
  txRxIndex = 0;

  switch (mode) {
    case TRANSMIT_SEND:
      // Prevent debug words from transmitting
      NRF_UART0->PSELRXD = 0xFFFFFFFF;
      NRF_UART0->PSELTXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      NRF_UART0->TASKS_STARTTX = 1;

      break ;
    case TRANSMIT_RECEIVE:
      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      NRF_UART0->TASKS_STARTRX = 1;

      break ;
    default:
      break ;
  }
}

static inline void transmitByte() { 
  NVIC_DisableIRQ(UART0_IRQn);
  NRF_UART0->TXD = txRxBuffer[txRxIndex++];
  NVIC_EnableIRQ(UART0_IRQn);
}

void Head::manage() {
  // Head has powered off: Ignore lack of comms
  if (lowPowerMode && !Head::spokenTo) {
    Watchdog::kick(WDOG_UART);
  }

  Head::spokenTo = false;

  // Head body sync is disabled, so just kick the watchdog
  memcpy(txRxBuffer, &g_dataToHead, sizeof(GlobalDataToHead));
  txRxIndex = 0;
  
  setTransmitMode(TRANSMIT_SEND);
  transmitByte();
}

extern "C"
void UART0_IRQHandler()
{
  static int header_shift = 0;

  // We received a byte
  if (NRF_UART0->EVENTS_RXDRDY) {
    NRF_UART0->EVENTS_RXDRDY = 0;

    uint8_t data = NRF_UART0->RXD;

    if (TRANSMIT_RECEIVE == uart_mode) {
          // Re-sync to header
        if (txRxIndex < 4) {
          header_shift = (header_shift >> 8) | (data << 24);
          
          if (header_shift == SPI_SOURCE_HEAD || header_shift == SPI_SOURCE_CRASHLOG) {
            txRxIndex = 4;
            return ;
          }
        } else {
          txRxBuffer[txRxIndex] = data;
        }

        // Looks like the head crashed, log and reset
        if (header_shift == SPI_SOURCE_CRASHLOG) {
          txRxBuffer[txRxIndex++ - 4] = data;

          if (txRxIndex - 4 >= SPINE_CRASH_LOG_SIZE) {
            Storage::set(STORAGE_CRASH_LOG_K02, txRxBuffer, SPINE_CRASH_LOG_SIZE);
            NVIC_SystemReset();
          }
          return ;
        }
        
        // We received a full packet
        if (++txRxIndex >= sizeof(GlobalDataToBody)) {
          memcpy(&g_dataToBody, txRxBuffer, sizeof(GlobalDataToBody));

          Spine::processMessages(g_dataToBody.cladData);
          Spine::dequeue(g_dataToHead.cladData);

          Head::synced = true;
          Head::spokenTo = true;
          Watchdog::kick(WDOG_UART);
        }
    }
  }

  // We transmitted a byte
  if (NRF_UART0->EVENTS_TXDRDY) {
    NRF_UART0->EVENTS_TXDRDY = 0;

    if (TRANSMIT_SEND == uart_mode) {
        // We are in regular head transmission mode
        if (txRxIndex >= sizeof(GlobalDataToHead)) {
          setTransmitMode(TRANSMIT_RECEIVE);
          header_shift = 0;
        } else {
          NRF_UART0->TXD = txRxBuffer[txRxIndex++];
        }
    }
  }
}
