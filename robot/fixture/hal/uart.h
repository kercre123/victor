#ifndef UART_H
#define UART_H

#include "hal/portable.h"
#include "hal/console.h"

// If set, then debug printing uses gun lights - gun lights are not available!
#define DEBUG_UART_ENABLED

#if defined(DEBUG_UART_ENABLED)
// Initialize serial lines for debug printing
void InitUART(void);
void SlowPutChar(char c);
void SlowPutString(const char *s);
void SlowPutInt(int i);
void SlowPutLong(int i);
void SlowPutHex(int i);
void SlowPutLongHex(int i);
//int SlowPrintf(const char* format, ...);
#define SlowPrintf ConsolePrintf
#else
#define InitUART()
#define SlowPutChar(c)
#define SlowPutString(s)
#define SlowPutInt(i)
#define SlowPutLong(i)
#define SlowPutHex(i)
#define SlowPutLongHex(i)
#define SlowPrintf(format, ...)
#endif

#endif
