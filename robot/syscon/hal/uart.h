#ifndef UART_H
#define UART_H

#include "portable.h"

// Initialize the UART peripheral on the designated pins in the source file.
void UARTInit();

void UARTPutChar(u8 c);
void UARTPutString(const char* s);
void UARTPutHex(u8 c);
void UARTPutHex32(u32 value);

#endif
