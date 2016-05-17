// Simple 2m baud bit-bang UART on P11 (if you can get a wire on it)
// XXX-FEP:  Put this in the bootloader, use P10, init P10
#include "hal.h"

#ifdef DEBUG

// Send a character out P11 - assumes P11 is already set for output
void PrintChar(u8 tx)
{
  ACC = tx;           // Get TX into A, inform compiler tx is not dead
  EA = 0;
#pragma asm
  mov r7, #9          // Count out 9 bits (including start bit)
  clr c               // C = 0 (start bit)
?bitloop:
  mov P11, c          // 3 P11 = C
  rrc a               // 1 C = tx & 1; tx >>= 1;
  nop                 // 1 Pad to 8 cycles (2m baud)
  djnz r7, ?bitloop   // 3 while (--r7)

  setb P11            // 3 P11 = stop bit
#pragma endasm
  EA = 1;
}

// Convert hex to ASCII before printing it
void PrintDigit(u8 tx)
{
  tx += '0';
  if (tx > '9')
    tx += ('A' - '0' - 10);
  PrintChar(tx);
}

// At 2mbaud, UART-print a 1 character note followed by len bytes of hex data
void DebugPrint(u8 note, u8 idata *hex, u8 len)
{
  // Print the string, then a newline
  do {
    PrintChar(note);
    PrintDigit(*hex >> 4);
    PrintDigit(*hex & 15);
    note = '.';
    hex++;
  } while (--len);
  PrintChar('\n');
}

#endif
