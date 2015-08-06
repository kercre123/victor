#include <string.h>
#include <stdint.h>

#include "board.h"
#include "uart.h"

#include "fsl_debug_console.h"

const uint32_t perf_clock = 80000000;
const uint32_t baud_rate = 312500;

#define BAUD_SBR(baud)  (perf_clock * 2 / baud_rate / 32)
#define BAUD_BRFA(baud) (perf_clock * 2 / baud_rate % 32)

void uart_init() {
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);
  
  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK;

  // Enable UART for this shiz
  PORTD_PCR7 = PORT_PCR_MUX(3) | PORT_PCR_ODE_MASK; // Open drain on the UART pin
  //PORTD_PCR7 |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; // PULLUP WHILE DISCONECTED
  
  int sbr = BAUD_SBR(baud_rate);
  int brfa = BAUD_BRFA(baud_rate);
  
  UART0_BDL = UART_BDL_SBR(sbr);
  UART0_BDH = 0;
  
  UART0_C1 = UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK; // 8 bit, 1 bit stop no parity (single wire)
  UART0_C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;      // Enable one wire (requires both units be enabled)
  UART0_C4 = UART_C4_BRFA(brfa);

  // Setup a 64-byte RX/TX fifo (and flush it)
  UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(6) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(6) ;
  UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK;

  // RECEIVE
  UART0_C3 = 0; // (no transmit)

  // TRANSMIT
  //UART0_C3 = UART_C3_TXDIR_MASK; // (transmit)

  uint8_t receive[64];
  int idx = 0;

  for (;;) {
    uint8_t status = UART0_S1;

    if (status & (UART_S1_FE_MASK | UART_S1_OR_MASK)) {
      PRINTF("err %2x", UART0_D);
      UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK;
      PRINTF(" %i", UART0_RCFIFO);
    }

    while (UART0_RCFIFO) { 
      uint8_t data = UART0_D;
      
      if (idx == 0 && data != 'B') { continue ; }
      
      receive[idx++] = data;

      if (idx >= 64) {
        PRINTF("\n\rSYNC: ");
        for (int i = 0; i < 64; i++) PRINTF("%2x", receive[i]);
        
        idx = 0;
      }
    }
    
    /*
    while (~UART0_SFIFO & UART_SFIFO_RXEMPT_MASK) {
      PRINTF("%i %i\n\r", UART0_RCFIFO, UART0_SFIFO & UART_SFIFO_RXEMPT_MASK);
    }
    */
    
    //PRINTF("\n\r");
  }
}
