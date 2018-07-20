#ifndef CONSOLE_H
#define CONSOLE_H

#include "hal/portable.h"

void InitConsole(void);
int ConsoleReadChar(void);
int ConsoleGetCharNoWait(void);
int ConsoleGetCharWait(u32 timeout);
int ConsolePrintf(const char* format, ...);
int ConsoleWrite(char* s);
void ConsolePutChar(char c);
void ConsoleUpdate(void);
int  ConsoleGetIndex_(bool clear=false); //get internal console index - clear wipes input
void ConsoleWriteHex(const u8* buffer, u32 numberOfBytes, u32 offset = 0);

char* ConsoleGetLine(int timeout_us, int *out_len = NULL);
int ConsoleFlushLine(void); //@return # chars discarded

#endif
