#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "anki/cozmo/robot/spineData.h"
#include "MK02F12810.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "spi.h"
#include "uart.h"
#include "spine.h"
#include "watchdog.h"
#include "hardware.h"

#include <string.h>
#include <stdint.h>

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND,
  TRANSMIT_CRASHLOG
};

static const int uart_fifo_size = 8;

static union {
  uint8_t   txRxBuffer[MAX(sizeof(GlobalDataToBody), sizeof(GlobalDataToHead))];
  uint32_t  rx_source;
};

volatile bool Anki::Cozmo::HAL::UART::HeadDataReceived = false;

static TRANSFER_MODE uart_mode;

static int txRxIndex;
static int crashLogBytes;
static const uint8_t* crashLogData;

inline void transmit_mode(TRANSFER_MODE mode);

void Anki::Cozmo::HAL::UART::Init() {
  g_dataToBody.source = (uint32_t)SPI_SOURCE_HEAD;

  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;

  // Reset UART faults
  volatile u8 dummy;
  UART0_C2 = 0;
  dummy = UART0_S1;
  dummy = UART0_D;

  // Enable UART for this shiz
  UART0_BDH = 0;
  UART0_BDL = UART_BDL_SBR(BAUD_SBR(spine_baud_rate));
  UART0_C4 = UART_C4_BRFA(BAUD_BRFA(spine_baud_rate));


  UART0_C1 = 0; // 8 bit, 1 bit stop no parity (single wire)
  UART0_S2 |= UART_S2_RXINV_MASK;
  UART0_C3 = UART_C3_TXINV_MASK;

  UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(2) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(2) ;
  UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK ;

  SOURCE_SETUP(BODY_UART_RX, SourceAlt3);
  SOURCE_SETUP(BODY_UART_TX, SourceAlt3 | SourcePullDown);

  crashLogBytes = 0;

  transmit_mode(TRANSMIT_RECEIVE);
}

void Anki::Cozmo::HAL::UART::SendCrashLog(const void* data, int bytes) {
  crashLogBytes = bytes;
  crashLogData = (uint8_t*) data;
  
}

inline void transmit_mode(TRANSFER_MODE mode) { 
  switch (mode) {
    case TRANSMIT_CRASHLOG:
    {
      UART0_C2 = UART_C2_TE_MASK;
     
      txRxIndex = 0;
      break ;
    }
    case TRANSMIT_SEND:
    {
      Anki::Cozmo::HAL::Spine::Dequeue(g_dataToBody.cladData);
      memcpy(txRxBuffer, &g_dataToBody, sizeof(GlobalDataToBody));

      UART0_C2 = UART_C2_TE_MASK;
     
      txRxIndex = 0;
      break ;
    }
    case TRANSMIT_RECEIVE:
    {
      UART0_C2 = UART_C2_RE_MASK;
      
      txRxIndex = 4;
      rx_source = 0;
      break ;
    }
    default:
    {
      break ;
    }
  }

  uart_mode = mode;
}

void Anki::Cozmo::HAL::UART::pause(void) {
  UART0_C2 = 0;
}

bool Anki::Cozmo::HAL::UART::FoundSync() {
  return HeadDataReceived;
}

void Anki::Cozmo::HAL::UART::WaitForSync() {
  while (!HeadDataReceived) {
    __asm { WFI }
  }
  HeadDataReceived = false;
}

void Anki::Cozmo::HAL::UART::Transmit(void) { 
  // Attempt to clear out buffer overruns
  if (UART0_S1 & UART_S1_OR_MASK) {
    UART0->CFIFO |= UART_CFIFO_RXFLUSH_MASK;
    UART0->PFIFO &= ~UART_PFIFO_RXFE_MASK;
    uint8_t c = UART0->D;
    UART0->PFIFO |= UART_PFIFO_RXFE_MASK;
  }

  switch (uart_mode) {
    case TRANSMIT_RECEIVE:
      while (UART0_RCFIFO) {
        uint8_t data = UART0_D;

        if (rx_source != SPI_SOURCE_BODY) {
          // Shifty header
          rx_source = (rx_source >> 8) | (data << 24);
          continue ;
        } else {
          txRxBuffer[txRxIndex++] = data;
        }

        if (txRxIndex >= sizeof(GlobalDataToHead)) {
          // We received a full packet
          memcpy(&g_dataToHead, txRxBuffer, sizeof(GlobalDataToHead));
          Watchdog::kick(WDOG_SPINE_COMMS);
          HeadDataReceived = true;
          
          if (crashLogBytes > 0) {
            transmit_mode(TRANSMIT_CRASHLOG);
          } else {
            transmit_mode(TRANSMIT_SEND);
          }
        }
      }

      break ;
    case TRANSMIT_CRASHLOG:
      {  
        static const uint8_t crashheader[] = {'C', 'R', 'S', 'H'};
        
        while (UART0_TCFIFO < uart_fifo_size) {
          if (txRxIndex < sizeof(crashheader)) {
            UART0_D = crashheader[txRxIndex++];
          } else if (crashLogBytes > 0) {
            UART0_D = *(crashLogData++);
            crashLogBytes--;
          } else if (UART0_S1 & UART_S1_TC_MASK) {
            NVIC_SystemReset();
          }
        }

        break ;
      }

    case TRANSMIT_SEND:
      // Transmission was complete, start receiving bytes once transmission has completed
      if (txRxIndex >= sizeof(GlobalDataToBody)) {
        if (UART0_S1 & UART_S1_TC_MASK) {
          transmit_mode(TRANSMIT_RECEIVE);
        }
        
        return ;
      }

      // Enqueue transmissions
      while (txRxIndex < sizeof(GlobalDataToBody) && UART0_TCFIFO < uart_fifo_size) {
        UART0_D = txRxBuffer[txRxIndex++];
      }
      break ;
  }
}
