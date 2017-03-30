#include "string.h"

#include "head.h"
#include "timer.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "lights.h"
#include "storage.h"
#include "anki/cozmo/robot/crashLogs.h"
#include "anki/cozmo/robot/rec_protocol.h"

#include "cubes.h"
#include "watchdog.h"
#include "hardware.h"
#include "backpack.h"
#include "bluetooth.h"
#include "battery.h"
#include "tests.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

uint8_t txBuffer[sizeof(GlobalDataToHead)];
uint8_t rxBuffer[MAX(sizeof(GlobalDataToBody), SPINE_CRASH_LOG_SIZE)];

enum TRANSMIT_MODE {
  TRANSMIT_UNKNOWN,
  TRANSMIT_SEND,
  #ifdef FACTORY
  TRANSMIT_CHARGER_RX,
  #endif
  TRANSMIT_RECEIVE,
};

static int txRxIndex;
static TRANSMIT_MODE uart_mode;

bool Head::spokenTo;
bool Head::synced;
static bool lowPowerMode;

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

static void setTransmitMode(TRANSMIT_MODE mode);

#ifdef FACTORY
static bool factoryComms;

void Head::enableFixtureComms(bool enable) {
  factoryComms = enable;
}
#endif

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

  #ifdef FACTORY
  factoryComms = true;
  #endif

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
                                    | (GPIO_PIN_CNF_DRIVE_H0D1 << GPIO_PIN_CNF_DRIVE_Pos)
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

    #ifdef FACTORY
    case TRANSMIT_CHARGER_RX:
      nrf_gpio_cfg_input(PIN_TX_VEXT, (*FIXTURE_HOOK == 0xDEADFACE) ? NRF_GPIO_PIN_NOPULL : NRF_GPIO_PIN_PULLUP);

      NRF_UART0->BAUDRATE = NRF_BAUD(charger_baud_rate);
      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_VEXT;
      NRF_UART0->EVENTS_RXDRDY = 0;
      { volatile uint8_t data = NRF_UART0->RXD; }
      
      // This magic trick will reset start bit processing
      NRF_UART0->ENABLE = 0;
      NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
      NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Msk | UART_INTENSET_RXDRDY_Msk;
      
      NRF_UART0->TASKS_STARTRX = 1;
      break ;
    #endif

    default:
      break ;
  }
}

static inline void transmitByte() { 
  NVIC_DisableIRQ(UART0_IRQn);
  NRF_UART0->TXD = txBuffer[txRxIndex++];
  NVIC_EnableIRQ(UART0_IRQn);
}

void Head::manage() {
  #ifdef FACTORY
  //if bootloader says we're connected to fixture, don't try talking to the head
  if (*FIXTURE_HOOK == 0xDEADFACE) {
    NRF_UART0->PSELRXD = 0xFFFFFFFF;
    nrf_gpio_pin_set(PIN_TX_VEXT);
    nrf_gpio_cfg_output(PIN_TX_VEXT);
    MicroWait(5);
    uart_mode = TRANSMIT_UNKNOWN;
    NVIC_EnableIRQ(UART0_IRQn);
    setTransmitMode(TRANSMIT_CHARGER_RX);
    Watchdog::kick(WDOG_UART);
    return ;
  }
  #endif

  // Head has powered off: Ignore lack of comms
  // Does not communicate while in low power mode
  if (lowPowerMode && !Head::spokenTo) {
    Watchdog::kick(WDOG_UART);
    return ;
  }

  if (Head::spokenTo) {
    Backpack::hasSync();
    memcpy(&g_dataToBody, rxBuffer, sizeof(GlobalDataToBody));

    Spine::processMessages(g_dataToBody.cladData);
    Spine::dequeue(g_dataToHead.cladData);
    Head::spokenTo = false;
  }

  // Head body sync is disabled, so just kick the watchdog
  memcpy(txBuffer, &g_dataToHead, sizeof(GlobalDataToHead));
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

    #ifdef FACTORY
    if (uart_mode == TRANSMIT_CHARGER_RX) {
      static const uint32_t PREFIX = 0x57740000;
      static const uint32_t MASK = 0xFFFF0000;
      static uint32_t fixture_data = 0;
      
      fixture_data = (fixture_data << 8) | data;
    
      // Acknowledge reception of character (for fixture robustness)
      MicroWait(100);
      nrf_gpio_pin_set(PIN_TX_VEXT);
      nrf_gpio_cfg_output(PIN_TX_VEXT);
      NRF_UART0->PSELRXD = 0xFFFFFFFF;
      MicroWait(10);
      nrf_gpio_pin_clear(PIN_TX_VEXT);
    
      if ((fixture_data & MASK) == PREFIX) {
        using namespace Anki::Cozmo;
        uint8_t test = fixture_data & 0xff;
        uint8_t param = (fixture_data>>8) & 0xff;
        TestFixtures::dispatch(test, param);
        fixture_data = 0;
      }
    }
    #endif

    if (TRANSMIT_RECEIVE == uart_mode) {
      // Re-sync to header
      if (txRxIndex < 4) {
        header_shift = (header_shift >> 8) | (data << 24);
        
        if (header_shift == SPI_SOURCE_HEAD || header_shift == SPI_SOURCE_CRASHLOG) {
          txRxIndex = 4;
          return ;
        }
      } else {
        rxBuffer[txRxIndex] = data;
      }

      // Looks like the head crashed, log and reset
      if (header_shift == SPI_SOURCE_CRASHLOG) {
        rxBuffer[txRxIndex++ - 4] = data;

        if (txRxIndex - 4 >= SPINE_CRASH_LOG_SIZE) {
          Storage::set(STORAGE_CRASH_LOG_K02, rxBuffer, SPINE_CRASH_LOG_SIZE);
          NVIC_SystemReset();
        }
        return ;
      }
      
      // We received a full packet
      if (++txRxIndex >= sizeof(GlobalDataToBody)) {
        Head::synced = true;
        Head::spokenTo = true;
        Watchdog::kick(WDOG_UART);
        
        #ifdef FACTORY
        if (factoryComms) setTransmitMode(TRANSMIT_CHARGER_RX);
        #endif
      }
      #ifdef FACTORY
      else if (factoryComms && txRxIndex + 1 == sizeof(GlobalDataToBody)) {
        // Let our fixture know we are almost ready to receive data
        nrf_gpio_pin_set(PIN_TX_VEXT);
        nrf_gpio_cfg_output(PIN_TX_VEXT);
      }
      #endif
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
          NRF_UART0->TXD = txBuffer[txRxIndex++];
        }
    }
  }
}
