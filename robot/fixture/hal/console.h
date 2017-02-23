#ifndef CONSOLE_H
#define CONSOLE_H

#include "hal/portable.h"

void InitConsole(void);
int ConsoleGetCharNoWait(void);
int ConsoleGetCharWait(u32 timeout);
int ConsolePrintf(const char* format, ...);
int ConsoleWrite(char* s);
void ConsolePutChar(char c);
void ConsoleUpdate(void);
void ConsoleWriteHex(const u8* buffer, u32 numberOfBytes, u32 offset = 0);

#endif
