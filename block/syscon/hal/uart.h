#ifndef UART_H
#define UART_H

#include "portable.h"

// Initialize the UART peripheral on the designated pins in the source file.
void UARTInit();

// Set/clear transmit mode - you can't receive while in transmit mode
void UARTSetTransmit(u8 enabled);

// Return true if we are receiving a break from the host
int UARTIsBreak();

void UARTPutChar(u8 c);
void UARTPutString(const char* s);
void UARTPutHex(u8 c);
void UARTPutHex32(u32 value);
void UARTPutDec(s32 value);

int UARTGetChar();

#endif
