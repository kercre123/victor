#ifndef _CF_SERIAL_H
#define _CF_SERIAL_H
//============================================================================
// Taken from:
// 635 Linux Demonstration Code.
// serial,h: Ultra-simple 635 command-line communications example
// Copyright 2005, Crystalfontz America, Inc. Written by Brent A. Crosby
// www.crystalfontz.com, brent@crystalfontz.com
//============================================================================
#include "cf_typedefs.h"
int   cf_Serial_Init(char *devname, int baud_rate);
void  cf_Uninit_Serial();
void  cf_SendByte(unsigned char datum);
void  cf_Sync_Read_Buffer(void);
dword cf_BytesAvail(void);
ubyte cf_GetByte(void);
dword cf_PeekBytesAvail(void);
void  cf_Sync_Peek_Pointer(void);
void  cf_AcceptPeekedData(void);
ubyte cf_PeekByte(void);
void  cf_SendData(unsigned char *data,int length);
void  cf_SendString(char *data);

void  cf_SetScroll(int on);
void  cf_SetWrap(int on);
void  cf_SetPos(int row, int col);
void  cf_EraseLine(int row);
void  cf_SendStringRC(int row, int col, char *str);
void  cf_Bargraph(int row, int start_col, int end_col, int style, int percent);
void  cf_ClearDisplay(void);
void  cf_HideCursor(void);
//============================================================================
#endif
