#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "MK02F12810.h"

#include "anki/cozmo/robot/spineData.h"

#include "uart.h"

void Anki::Cozmo::HAL::DebugInit() {
  // Enable clocking to the UART
  SIM_SOPT5 &= ~(SIM_SOPT5_UART1TXSRC_MASK | SIM_SOPT5_UART1RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART1TXSRC(0) | SIM_SOPT5_UART1RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART1_MASK;

  // Enable UART for this shiz
  PORTE_PCR16 = PORT_PCR_MUX(3);

  UART1_BDL = UART_BDL_SBR(BAUD_SBR(debug_baud_rate));
  UART1_BDH = 0;

  UART1_C1 = 0; // 8 bit, 1 bit stop no parity (single wire)
  UART1_C2 = UART_C2_TE_MASK; // Enable one wire (requires both units be enabled)
  UART1_C3 = 0;
  UART1_C4 = UART_C4_BRFA(BAUD_BRFA(debug_baud_rate));

  // Setup a 1-byte RX/TX fifo (and flush it)
  UART1_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(0);
  UART1_CFIFO = UART_CFIFO_TXFLUSH_MASK;
}

void Anki::Cozmo::HAL::DebugPutc(char c)
{
  while (UART1_TCFIFO) ;  // Wait for FIFO to empty
  UART1_D = c;
}

void Anki::Cozmo::HAL::DebugPrintf(const char *format, ...)
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
