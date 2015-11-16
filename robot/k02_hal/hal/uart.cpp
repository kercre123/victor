#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "syscon/sys_boot/hal/rec_protocol.h"
#include "anki/cozmo/robot/spineData.h"
#include "MK02F12810.h"

#include "uart.h"

#include <string.h>
#include <stdint.h>

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND
};

#define max(a,b)  ((a > b) ? a : b)

static union {
  uint8_t   txRxBuffer[max(sizeof(GlobalDataToBody), sizeof(GlobalDataToHead))];
  uint32_t  rx_source;
};

static int txRxIndex;
static bool transmitting;
static bool enter_recovery;
static int reboot_timeout;
static RECOVERY_STATE recoveryMode = STATE_UNKNOWN;

inline void transmit_mode(bool tx);

bool Anki::Cozmo::HAL::HeadDataReceived = false;
static const int uart_fifo_size = 8;
static const int MAX_REBOOT_TIMEOUT = 10000;  // 1.3seconds

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
  transmit_mode(false);
}

inline void transmit_mode(bool tx) { 
  if (tx) {
    // Special case mode where we force the head to enter recovery mode
    if (enter_recovery) {
      g_dataToBody.cubeStatus.ledDark = 0;
      g_dataToBody.cubeStatus.secret = secret_code;
      enter_recovery = false;
    }

    memcpy(txRxBuffer, &g_dataToBody, sizeof(GlobalDataToBody));

    PORTD_PCR6 = PORT_PCR_MUX(0);
    PORTD_PCR7 = PORT_PCR_MUX(3);
    UART0_C2 = UART_C2_TE_MASK;
  } else {
    reboot_timeout = 0;

    PORTD_PCR6 = PORT_PCR_MUX(3);
    PORTD_PCR7 = PORT_PCR_MUX(0);
    UART0_C2 = UART_C2_RE_MASK;
  }
  
  txRxIndex = 0;
  transmitting = tx;
}

static inline void HUPFifo(void) {
  UART0->CFIFO |= UART_CFIFO_RXFLUSH_MASK;
  UART0->PFIFO &= ~UART_PFIFO_RXFE_MASK;
  uint8_t c = UART0->D;
  UART0->PFIFO |= UART_PFIFO_RXFE_MASK;
}

void Anki::Cozmo::HAL::EnterRecovery(void) {
  enter_recovery = true;
}

void Anki::Cozmo::HAL::LeaveRecovery(void) {
  // TODO!
}

void Anki::Cozmo::HAL::UartTransmit(void) { 
  // Attempt to clear out buffer overruns
  if (UART0_S1 & UART_S1_OR_MASK) {
    HUPFifo();
  }

  while (UART0_RCFIFO) {
    // TODO: DETECT WHEN WE ARE IN RECOVERY MODE

    txRxBuffer[txRxIndex] = UART0_D;

    // Re-sync
    if (txRxIndex < 4) {
      uint32_t mask = ~(0xFFFFFFFF << (txRxIndex * 8));

      const uint32_t fail_spine = (SPI_SOURCE_BODY ^ rx_source) & mask;
      const uint32_t fail_recvr = (RECOVER_SOURCE_BODY ^ rx_source) & mask;

      if(fail_spine && fail_recvr) {
        recoveryMode = STATE_UNKNOWN;
        txRxIndex = 0;
        continue ;
      }
    }

    if (txRxIndex == 4 && rx_source == RECOVER_SOURCE_BODY) {
      recoveryMode = (RECOVERY_STATE) txRxBuffer[txRxIndex];
  
      // TODO: DEQUEUE RECOVERY MODE HERE!

      txRxIndex = 0;
      continue ;
    }
    
    // We received a full packet
    if (++txRxIndex >= sizeof(GlobalDataToHead)) {
      recoveryMode = STATE_RUNNING;
      
      memcpy(&g_dataToHead, txRxBuffer, sizeof(GlobalDataToHead));

      HeadDataReceived = true;
      transmit_mode(true);
    }
  }

  if (transmitting) {
    // Transmission was complete, start receiving bytes once transmission has completed
    if (txRxIndex >= sizeof(GlobalDataToBody)) {
      if (UART0_S1 & UART_S1_TC_MASK )
        transmit_mode(false);
      
      return ;
    }

    // Enqueue transmissions
    while (txRxIndex < sizeof(GlobalDataToBody) && UART0_TCFIFO < uart_fifo_size) {
      UART0_D = txRxBuffer[txRxIndex++];
    }
  }
}
