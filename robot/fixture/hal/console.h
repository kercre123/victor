#ifndef CONSOLE_H
#define CONSOLE_H

#include "hal/portable.h"

void InitConsole(void);
int ConsoleGetCharNoWait(void);
int ConsoleGetCharWait(u32 timeout);
void ConsolePrintf(const char* format, ...);
void ConsoleWrite(char* s);
void ConsolePutChar(char c);
void ConsoleUpdate(void);
void ConsoleWriteHex(u8* buffer, u32 numberOfBytes);

#endif
