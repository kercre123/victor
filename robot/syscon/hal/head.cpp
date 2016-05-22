#include "string.h"

#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "lights.h"

#include "radio.h"
#include "rtos.h"
#include "hardware.h"
#include "backpack.h"
#include "bluetooth.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

#define MAX(a, b) ((a > b) ? a : b)

uint8_t txRxBuffer[MAX(sizeof(GlobalDataToBody), sizeof(GlobalDataToHead))];

enum TRANSMIT_MODE {
  TRANSMIT_UNKNOWN,
  TRANSMIT_SEND,
  TRANSMIT_RECEIVE,
  TRANSMIT_CHARGER_RX,
};

static int txRxIndex;
static TRANSMIT_MODE uart_mode;
static bool m_enabled;
static const int charger_baud_rate = 100000;

bool Head::spokenTo = false;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

static void setTransmitMode(TRANSMIT_MODE mode);

void Head::init() 
{
  // Sync pattern
  memset(&g_dataToBody, 0, sizeof(g_dataToBody));
  g_dataToHead.source = SPI_SOURCE_BODY;
  Head::spokenTo = false;
  txRxIndex = 0;

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

  // We begin in receive mode (slave)
  m_enabled = true;

  RTOS::schedule(Head::manage);
}

void Head::enable(bool enable) {
  m_enabled = enable;
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
      // Configure pin so it is open-drain
      NRF_GPIO->PIN_CNF[PIN_TX_HEAD] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                                    | (GPIO_PIN_CNF_DRIVE_H0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                                    | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                                    | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                    | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

      // Prevent debug words from transmitting
      NRF_UART0->PSELRXD = 0xFFFFFFFF;
      NRF_UART0->PSELTXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      NRF_UART0->TASKS_STARTTX = 1;

      break ;
    case TRANSMIT_RECEIVE:
      nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);

      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      NRF_UART0->TASKS_STARTRX = 1;

      break ;
    case TRANSMIT_CHARGER_RX:
      nrf_gpio_cfg_input(PIN_TX_VEXT, NRF_GPIO_PIN_PULLUP);

      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_VEXT;
      NRF_UART0->BAUDRATE = NRF_BAUD(charger_baud_rate);

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

void Head::manage(void* userdata) {
  // Head body sync is disabled, so just kick the watchdog
  if (!m_enabled) {
    return ;
  }

  Spine::Dequeue(&(g_dataToHead.cladBuffer));
  memcpy(txRxBuffer, &g_dataToHead, sizeof(GlobalDataToHead));
  g_dataToHead.cladBuffer.length = 0;
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

    switch (uart_mode) {
      case TRANSMIT_RECEIVE:
          // Re-sync to header
        if (txRxIndex < 4) {
          header_shift = (header_shift >> 8) | (data << 24);
          
          if (header_shift == SPI_SOURCE_HEAD) {
            txRxIndex = 4;
            return ;
          }
        } else {
          txRxBuffer[txRxIndex] = data;
        }

        // We received a full packet
        if (++txRxIndex >= sizeof(GlobalDataToBody)) {
          memcpy(&g_dataToBody, txRxBuffer, sizeof(GlobalDataToBody));
          Spine::ProcessHeadData();
          Head::spokenTo = true;
          RTOS::kick(WDOG_UART);
          
          setTransmitMode(TRANSMIT_CHARGER_RX);
        } else if (txRxIndex + 1 == sizeof(GlobalDataToBody)) {
          // Let our fixture know we are almost ready to receive data
          nrf_gpio_pin_set(PIN_TX_VEXT);
          nrf_gpio_cfg_output(PIN_TX_VEXT);
        }

        break ;
      case TRANSMIT_CHARGER_RX:
        static const uint32_t PREFIX = 0x57746600;
        static const uint32_t MASK = 0xFFFFFF00;
        static uint32_t fixture_data = 0;
        
        fixture_data = (fixture_data << 8) | data;
      
        if ((fixture_data & MASK) == PREFIX) {
          using namespace Anki::Cozmo;

          RobotInterface::EnterFactoryTestMode msg;
          msg.mode = fixture_data & ~MASK;
          RobotInterface::SendMessage(msg);
          
          fixture_data = 0;
        }

        break ;
      default:
        break ;
    }
  }

  // We transmitted a byte
  if (NRF_UART0->EVENTS_TXDRDY) {
    NRF_UART0->EVENTS_TXDRDY = 0;

    switch(uart_mode) {
      case TRANSMIT_SEND:
        // We are in regular head transmission mode
        if (txRxIndex >= sizeof(GlobalDataToHead)) {
          setTransmitMode(TRANSMIT_RECEIVE);
          header_shift = 0;
        } else {
          transmitByte();
        }
        break ;
      default:
        break ;
    }
  }
}
