#include <string.h>
#include <stdint.h>

#include "board.h"
#include "uart.h"

#include "fsl_debug_console.h"

const uint32_t perf_clock = 80000000;
const uint32_t baud_rate = 625000;
const int max_fifo_size = 8;

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND
};

static volatile uint8_t dataFrom[64];
static const uint8_t dataTo[64] = {'H', 0xFA, 0xF3, 0x20};
static volatile int transferIndex;
static volatile int transferMode;

void uart_init() {
  // AHHHHHH DO NO USE!
  
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);
  
  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK;

  // Enable UART for this shiz
  PORTD_PCR7 = PORT_PCR_MUX(3) | PORT_PCR_ODE_MASK; // Open drain on the UART pin
  
  int sbr = BAUD_SBR(baud_rate);
  int brfa = BAUD_BRFA(baud_rate);
  
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

  transferMode = TRANSMIT_RECEIVE;
  transferIndex = 0;

  NVIC_SetPriority(UART0_ERR_IRQn, 3);
  NVIC_EnableIRQ(UART0_ERR_IRQn);

  NVIC_SetPriority(UART0_RX_TX_IRQn, 3);
  NVIC_EnableIRQ(UART0_RX_TX_IRQn);
}

void transmit_blob(void) {
  while (UART0_TCFIFO < max_fifo_size && transferIndex < sizeof(dataTo)) {
    UART0_D = dataTo[transferIndex++];
  }
}

void UART0_RX_TX_IRQHandler(void) {
  uint8_t status = UART0_S1; // This is only useful to clear interrupt

  switch (transferMode) {
    case TRANSMIT_RECEIVE:
      while (UART0_RCFIFO) {
        uint8_t data = UART0_D;

        if (transferIndex == 0 && data != 'B') { continue ; }

        dataFrom[transferIndex++] = data;
        
        if(transferIndex >= sizeof(dataFrom)) {
          PRINTF("%2x", dataFrom[49]);
          transferMode = TRANSMIT_SEND;
          transferIndex = 0;
          UART0_C3 |= UART_C3_TXDIR_MASK; // (transmit)

          transmit_blob();
        }
      }

      break ;
    case TRANSMIT_SEND:
      if (UART0_TCFIFO > 0 || transferIndex < sizeof(dataTo)) {
        transmit_blob();
      } else {
        transferMode = TRANSMIT_RECEIVE;
        transferIndex = 0;

        // We might want a delay in here, so the other processor has time to turn around
        UART0_C3 &= ~UART_C3_TXDIR_MASK; // (receive)
      }
      break ;
  }
}

void UART0_ERR_IRQHandler(void) {
  uint8_t status = UART0_S1;

  if (status & (UART_S1_FE_MASK | UART_S1_OR_MASK)) {
    PRINTF("E%c", UART0_D);
    UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK;
    
    transferIndex = 0;
  }
}
