/**
File: communications.h
Author: Peter Barnum
Created: 2013

Simple serial and socket connection routines

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _SERIAL_H_
#define _SERIAL_H_

// If using windows, use WIN32 API
// Otherwise, try Posix
#ifdef _MSC_VER
#include <windows.h>
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "anki/common/robot/config.h"

using namespace Anki;

// Serial is only supported on Windows
class Serial
{
public:
  Serial();

  Result Open(
    s32 comPort,
    s32 baudRate,
    char parity = 'N',
    s32 dataBits = 8,
    s32 stopBits = 1);

  Result Close();

  Result Read(
    void * buffer,
    s32 bufferLength,
    s32 &bytesRead);

protected:
  bool isOpen;

#ifdef _MSC_VER
  HANDLE comPortHandle;
  OVERLAPPED readEventHandle;
  OVERLAPPED writeEventHandle;
  DCB dcb;
#else
  // Only MSCV supported
#endif
}; // class Serial

// Socket is supported on Windows, Mac, and probably most Posix
class Socket
{
public:
  Socket();

  Result Open(
    const char * ipAddress,
    const s32 port);

  Result Close();

  Result Read(
    void * buffer,
    s32 bufferLength,
    s32 &bytesRead);

protected:
  bool isOpen;

#ifdef _MSC_VER
  SOCKET socketHandle;
#else
  int socketHandle;
#endif
}; // class Socket

#endif // #ifndef _SERIAL_H_