#include <string.h>
#include <stdint.h>

#include "board.h"
#include "uart.h"

const int max_fifo_size = 8;

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND
};

static volatile uint8_t dataFrom[64];
static const uint8_t dataTo[64] = {'H', 0xFA, 0xF3, 0x20};
static volatile int transferIndex;
static volatile int transferMode;

inline void transmit_mode(bool tx);

void uart_init() {
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK;

  // Enable UART for this shiz
  PORTD_PCR7 = PORT_PCR_MUX(3) | PORT_PCR_ODE_MASK; // Open drain on the UART pin

  int sbr = BAUD_SBR(spine_baud_rate);
  int brfa = BAUD_BRFA(spine_baud_rate);

  UART0_BDL = UART_BDL_SBR(sbr);
  UART0_BDH = 0;

  UART0_C1 = UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK; // 8 bit, 1 bit stop no parity (single wire)
  UART0_C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;      // Enable one wire (requires both units be enabled)
  UART0_C2 |= UART_C2_TCIE_MASK | UART_C2_RIE_MASK;  // Enable a few interrupts
  UART0_C4 = UART_C4_BRFA(brfa);

  // Setup a 64-byte RX/TX fifo (and flush it)
  UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(6) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(6) ;
  UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK;

  UART0_C3 = UART_C3_ORIE_MASK | UART_C3_FEIE_MASK;

  transmit_mode(false);

  /*
  NVIC_SetPriority(UART0_ERR_IRQn, 3);
  NVIC_EnableIRQ(UART0_ERR_IRQn);

  NVIC_SetPriority(UART0_RX_TX_IRQn, 3);
  NVIC_EnableIRQ(UART0_RX_TX_IRQn);
  */
}

void transmit_blob(void) {
  while (UART0_TCFIFO < max_fifo_size && transferIndex < sizeof(dataTo)) {
    UART0_D = dataTo[transferIndex++];
  }
}

inline void transmit_mode(bool tx) {
  transferIndex = 0;

  if (tx) {
    transferMode = TRANSMIT_SEND;
    UART0_C3 |= UART_C3_TXDIR_MASK;
  } else {
    transferMode = TRANSMIT_RECEIVE;
    UART0_C3 &= ~UART_C3_TXDIR_MASK;
  }
}

extern "C" void UART0_RX_TX_IRQHandler(void) {
  uint8_t status = UART0_S1; // This is only useful to clear interrupt

  while (UART0_RCFIFO) {
    uint8_t data = UART0_D;

    PRINTF("%2x", data);
  }

  /*
  switch (transferMode) {
    case TRANSMIT_RECEIVE:
      while (UART0_RCFIFO) {
        uint8_t data = UART0_D;

        if (transferIndex == 0 && data != 'B') { continue ; }

        dataFrom[transferIndex++] = data;
        
        if(transferIndex >= sizeof(dataFrom)) {
          PRINTF("%2x", dataFrom[49]);
          transmit_mode(true);

          transmit_blob();
        }
      }

      break ;
    case TRANSMIT_SEND:
      if (UART0_TCFIFO > 0 || transferIndex < sizeof(dataTo)) {
        transmit_blob();
      } else {
        transmit_mode(false);
      }
      break ;
  }
  */
}

extern "C" void UART0_ERR_IRQHandler(void) {
  uint8_t status = UART0_S1;

  if (status & (UART_S1_FE_MASK | UART_S1_OR_MASK)) {
    PRINTF("E%c", UART0_D);
    UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK;
    
    transferIndex = 0;
  }
}
