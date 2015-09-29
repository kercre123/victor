#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "board.h"
#include "uart.h"

void DebugInit() {
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART1TXSRC_MASK | SIM_SOPT5_UART1RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART1TXSRC(0) | SIM_SOPT5_UART1RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART1_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  // Enable UART for this shiz
  PORTE_PCR16 = PORT_PCR_MUX(3);

  int sbr = BAUD_SBR(debug_baud_rate);
  int brfa = BAUD_BRFA(debug_baud_rate);

  UART1_BDL = UART_BDL_SBR(sbr);
  UART1_BDH = 0;

  UART1_C1 = UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK; // 8 bit, 1 bit stop no parity (single wire)
  UART1_C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;      // Enable one wire (requires both units be enabled)
  UART1_C2 |= UART_C2_TCIE_MASK | UART_C2_RIE_MASK;  // Enable a few interrupts
  UART1_C4 = UART_C4_BRFA(brfa);

  // Setup a 64-byte RX/TX fifo (and flush it)
  UART1_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(6) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(6) ;
  UART1_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK;

  UART1_C3 = UART_C3_ORIE_MASK | UART_C3_FEIE_MASK | UART_C3_TXDIR_MASK;
}

void DebugPrintf(const char *format, ...)
{
  char buffer[512];
  
  va_list aptr;
  int chars;

  va_start(aptr, format);
  chars = vsnprintf(buffer, sizeof(buffer), format, aptr);
  va_end(aptr);

  char *write = buffer;
  while (chars-- > 0) {
    while (UART1_TCFIFO) ;  // Wait for FIFO to empty
    UART1_D = *(write++);
  }
}
