/**
File: serial.h
Author: Peter Barnum
Created: 2013

Simple serial connection routines

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <windows.h>
#include <winsock.h>

#include "anki/common/robot/config.h"

using namespace Anki::Embedded;

class Serial
{
public:
  Serial();

  Result Open(
    s32 comPort,
    s32 baudRate,
    char parity = 'N',
    s32 dataBits = 8,
    s32 stopBits = 1
    );

  Result Close();

  Result Read(void * buffer, s32 bufferLength, DWORD &bytesRead);

protected:
  bool isOpen;

  HANDLE comPortHandle;

  OVERLAPPED readEventHandle;
  OVERLAPPED writeEventHandle;

  DCB dcb;
}; // class Serial

class Socket
{
public:
  Socket();

  Result Open(
    const char * ipAddress,
    const s32 port);

  Result Close();

  Result Read(void * buffer, s32 bufferLength, DWORD &bytesRead);

protected:
  bool isOpen;

  SOCKET socketHandle;
}; // class Socket

#endif // #ifndef _SERIAL_H_