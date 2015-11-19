#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "anki/cozmo/robot/spineData.h"
#include "MK02F12810.h"

#include "uart.h"

#include <string.h>
#include <stdint.h>

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND,
  TRANSMIT_RECOVERY
};

static const int uart_fifo_size = 8;
static const int MAX_REBOOT_TIMEOUT = 10000;  // 1.3seconds

#define MAX(a,b)  ((a > b) ? a : b)

static union {
  uint8_t   txRxBuffer[MAX(sizeof(GlobalDataToBody), sizeof(GlobalDataToHead))];
  uint32_t  rx_source;
};

volatile RECOVERY_STATE Anki::Cozmo::HAL::recoveryMode = STATE_UNKNOWN;
volatile bool Anki::Cozmo::HAL::HeadDataReceived = false;
volatile bool Anki::Cozmo::HAL::RecoveryStateUpdated = false;

static TRANSFER_MODE uart_mode;

static int txRxIndex;
static bool enter_recovery;

inline void transmit_mode(TRANSFER_MODE mode);

// Recovery mode data FIFO
static uint8_t recovery_fifo[32];
static int rec_count = 0;
static int rec_first = 0;
static int rec_last = 0;

void Anki::Cozmo::HAL::UartInit() {
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;

  // Enable UART for this shiz
  UART0_BDL = UART_BDL_SBR(BAUD_SBR(spine_baud_rate));
  UART0_BDH = 0;

  UART0_C1 = 0; // 8 bit, 1 bit stop no parity (single wire)
  UART0_S2 |= UART_S2_RXINV_MASK;
  UART0_C3 = UART_C3_TXINV_MASK;
  UART0_C4 = UART_C4_BRFA(BAUD_BRFA(spine_baud_rate));

  UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(2) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(2) ;
  UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK ;

  g_dataToBody.source = (uint32_t)SPI_SOURCE_HEAD;
  
  MicroWait(1000);
  transmit_mode(TRANSMIT_RECEIVE);
}

inline void transmit_mode(TRANSFER_MODE mode) { 
  switch (mode) {
    case TRANSMIT_SEND:
      // Special case mode where we force the head to enter recovery mode
      if (enter_recovery) {
        g_dataToBody.cubeStatus.ledDark = 0;
        g_dataToBody.recover = recovery_secret_code;
        enter_recovery = false;
      }

      memcpy(txRxBuffer, &g_dataToBody, sizeof(GlobalDataToBody));
      g_dataToBody.recover = 0; // Don't keep sending this

      PORTD_PCR6 = PORT_PCR_MUX(0);
      PORTD_PCR7 = PORT_PCR_MUX(3);
      UART0_C2 = UART_C2_TE_MASK;
      break ;

    case TRANSMIT_RECOVERY:
      PORTD_PCR6 = PORT_PCR_MUX(0);
      PORTD_PCR7 = PORT_PCR_MUX(3);
      UART0_C2 = UART_C2_TE_MASK;
      break ;

    case TRANSMIT_RECEIVE:
      PORTD_PCR6 = PORT_PCR_MUX(3);
      PORTD_PCR7 = PORT_PCR_MUX(0);
      UART0_C2 = UART_C2_RE_MASK;
      break ;
  }
  
  uart_mode = mode;
  txRxIndex = 0;
}

static inline void HUPFifo(void) {
  UART0->CFIFO |= UART_CFIFO_RXFLUSH_MASK;
  UART0->PFIFO &= ~UART_PFIFO_RXFE_MASK;
  uint8_t c = UART0->D;
  UART0->PFIFO |= UART_PFIFO_RXFE_MASK;
}

void Anki::Cozmo::HAL::EnterBodyRecovery(void) {
  enter_recovery = true;
}

void Anki::Cozmo::HAL::SendRecoveryData(const uint8_t* data, int bytes) {
  while (bytes-- > 0 && rec_count < sizeof(recovery_fifo)) {
    recovery_fifo[rec_last++] = *(data++);
    
    rec_count++;
    if (rec_last > sizeof(recovery_fifo)) {
      rec_last %= sizeof(recovery_fifo);
    }
  }
}

static bool HaveRecoveryData(void) {
  return rec_count > 0;
}

static bool TransmitRecoveryData(void) {
  if (!HaveRecoveryData()) {
    return false;
  }
  
  while (UART0_TCFIFO < uart_fifo_size && HaveRecoveryData()) {
    UART0_D = recovery_fifo[rec_first++];

    rec_count--;
    if (rec_first > sizeof(recovery_fifo)) {
      rec_first %= sizeof(recovery_fifo);
    }
  }
  
  return true;
}

static void ChangeRecoveryState(RECOVERY_STATE mode) {
  using namespace Anki::Cozmo::HAL;

  RecoveryStateUpdated = true;
  recoveryMode = mode;
}

void Anki::Cozmo::HAL::WaitForSync() {
  while (recoveryMode != STATE_RUNNING || !HeadDataReceived) {
    __asm { WFI }
  }
  HeadDataReceived = false;
}

void Anki::Cozmo::HAL::UartTransmit(void) { 
  // Attempt to clear out buffer overruns
  if (UART0_S1 & UART_S1_OR_MASK) {
    HUPFifo();
  }

  switch (uart_mode) {
    case TRANSMIT_RECEIVE:
      while (UART0_RCFIFO) {
        txRxBuffer[txRxIndex] = UART0_D;

        // Words are big endian
        const uint16_t RECOVERY_HEADER = (COMMAND_HEADER << 8) | (COMMAND_HEADER >> 8);

        // Re-sync
        if (txRxIndex < 4) {
          uint32_t body_mask = ~(0xFFFFFF00 << (txRxIndex * 8));
          uint32_t recv_mask = body_mask & 0xFFFF; // we only care about the bottom two bits here
          
          // Verify that the header is valid (resync)
          if ((rx_source & body_mask) != (SPI_SOURCE_BODY & body_mask) &&
              (rx_source & recv_mask) != (RECOVERY_HEADER & recv_mask)) {
            txRxIndex = 0;
            ChangeRecoveryState(STATE_UNKNOWN);
            continue ;
          }
        }

        if (txRxIndex == 4) {
          if ((rx_source & 0xFFFF) == RECOVERY_HEADER) {
            ChangeRecoveryState((RECOVERY_STATE)(__rev(rx_source) & 0xFFFF));
            txRxIndex = 0;
          }
        }
        
        txRxIndex++;
        
        if (txRxIndex >= sizeof(GlobalDataToHead)) {
          // We received a full packet
          ChangeRecoveryState(STATE_RUNNING);
          memcpy(&g_dataToHead, txRxBuffer, sizeof(GlobalDataToHead));
          HeadDataReceived = true;
          
          transmit_mode(TRANSMIT_SEND);
        }
      }
      
      // We want to send data to the body
      if (recoveryMode != STATE_RUNNING && 
          recoveryMode != STATE_UNKNOWN &&
          HaveRecoveryData()) {
        transmit_mode(TRANSMIT_RECOVERY);
      }
      
      break ;
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
    case TRANSMIT_RECOVERY:
      if (!TransmitRecoveryData() && UART0_S1 & UART_S1_TC_MASK) {
        transmit_mode(TRANSMIT_RECEIVE);
      }

      break ;
  }
}
