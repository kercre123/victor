/**
File: communication.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "communication.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/errorHandling.h"

#include "anki/common/shared/utilities_shared.h"

#include <stdio.h>

#define STRING_LENGTH 1024

using namespace Anki;

Serial::Serial()
  : isOpen(false)
{
} // Serial::Serial()

Result Serial::Open(
  s32 comPort,
  s32 baudRate,
  char parity,
  s32 dataBits,
  s32 stopBits
  )
{
#ifdef _MSC_VER
  AnkiConditionalErrorAndReturnValue(isOpen == false,
    RESULT_FAIL, "Serial::Open", "Port is already open");

  isOpen = true;

  const DWORD dwEvtMask = EV_RXCHAR | EV_CTS;

  char lpPortName[STRING_LENGTH];
  char dcbInitString[STRING_LENGTH];

  // TODO: safe version?
  // Opening a com port greater than 9 requires the weird "\\\\.\\" syntax
  snprintf(lpPortName, STRING_LENGTH, "\\\\.\\COM%d", comPort);

  comPortHandle = CreateFile(
    lpPortName,
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_OVERLAPPED,
    0);

  //std::cout << GetLastError() << "\n";

  AnkiConditionalErrorAndReturnValue(comPortHandle != NULL && comPortHandle != INVALID_HANDLE_VALUE,
    RESULT_FAIL, "Serial::Open", "Could not open port");

  memset(&this->readEventHandle, 0, sizeof(this->readEventHandle));
  memset(&this->writeEventHandle, 0, sizeof(this->writeEventHandle));

  readEventHandle.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  writeEventHandle.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  COMMTIMEOUTS commTimeOuts;
  commTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
  commTimeOuts.ReadTotalTimeoutMultiplier = 0;
  commTimeOuts.ReadTotalTimeoutConstant = 0;
  commTimeOuts.WriteTotalTimeoutMultiplier = 0;
  commTimeOuts.WriteTotalTimeoutConstant = 10000;

  snprintf(dcbInitString, STRING_LENGTH, "baud=%d parity=%c data=%d stop=%d", baudRate, parity, dataBits, stopBits);

  const bool areCommTimeoutsSet = SetCommTimeouts(comPortHandle, &commTimeOuts);
  AnkiConditionalErrorAndReturnValue(areCommTimeoutsSet,
    RESULT_FAIL, "Serial::Open", "Could not set comm timeouts");

  const bool isCommMaskSet = SetCommMask(comPortHandle, dwEvtMask);
  AnkiConditionalErrorAndReturnValue(isCommMaskSet,
    RESULT_FAIL, "Serial::Open", "Could not set comm mask");

  const bool gotCommState = GetCommState(comPortHandle, &dcb);
  AnkiConditionalErrorAndReturnValue(gotCommState,
    RESULT_FAIL, "Serial::Open", "Could not get comm state");

  dcb.fRtsControl = RTS_CONTROL_ENABLE;

  const bool builtCommDcb = BuildCommDCB(dcbInitString, &dcb);
  AnkiConditionalErrorAndReturnValue(builtCommDcb,
    RESULT_FAIL, "Serial::Open", "Could not build comm dcb");

  const bool setCommState = SetCommState(comPortHandle, &dcb);
  AnkiConditionalErrorAndReturnValue(setCommState,
    RESULT_FAIL, "Serial::Open", "Could not set comm state");

  const bool isPurged = PurgeComm(comPortHandle, PURGE_RXABORT|PURGE_TXABORT|PURGE_RXCLEAR|PURGE_TXCLEAR);
  AnkiConditionalErrorAndReturnValue(isPurged,
    RESULT_FAIL, "Serial::Open", "Could not purge comm");

  const bool clearedErrors = ClearCommError(comPortHandle, NULL, NULL);
  AnkiConditionalErrorAndReturnValue(isPurged,
    RESULT_FAIL, "Serial::Open", "Could not clear errors");

  CoreTechPrint("Com port %d opened at %d baud\n", comPort, baudRate);

  return RESULT_OK;
#else // #ifdef _MSC_VER
  AnkiError("Serial::Open", "Only MSVC supported");
  return RESULT_FAIL;
#endif // #ifdef _MSC_VER ... #else
} // Result Serial::Open()

Result Serial::Close()
{
#ifdef _MSC_VER
  if(readEventHandle.hEvent != NULL)
    CloseHandle(readEventHandle.hEvent);

  if(writeEventHandle.hEvent != NULL)
    CloseHandle(writeEventHandle.hEvent);

  CloseHandle(comPortHandle);

  isOpen = false;

  return RESULT_OK;
#else // #ifdef _MSC_VER
  AnkiError("Serial::Open", "Only MSVC supported");
  return RESULT_FAIL;
#endif // #ifdef _MSC_VER ... #else
} // Result Serial::Close()

Result Serial::Read(void * buffer, s32 bufferLength, s32 &bytesRead)
{
#ifdef _MSC_VER
  const s32 timeToWaitForRead = 50000;

  bytesRead = 0;

  AnkiConditionalErrorAndReturnValue(isOpen && comPortHandle!=NULL,
    RESULT_FAIL, "Serial::Open", "Port is not open");

  COMSTAT comStat;

  ClearCommError(comPortHandle, NULL, &comStat);
  if(comStat.cbInQue == 0)  {
    return RESULT_OK;
  }

  bytesRead = comStat.cbInQue;
  if(bufferLength < bytesRead)
    bytesRead = bufferLength;

  DWORD bytesReadWord;
  const bool fileRead = ReadFile(comPortHandle, buffer, bytesRead, &bytesReadWord, &readEventHandle);

  bytesRead = bytesReadWord;

  AnkiConditionalErrorAndReturnValue(fileRead,
    RESULT_FAIL, "Serial::Open", "Could not read from port");

  return RESULT_OK;
#else // #ifdef _MSC_VER
  AnkiError("Serial::Open", "Only MSVC supported");
  return RESULT_FAIL;
#endif // #ifdef _MSC_VER ... #else
} // Result Serial::Close()

Socket::Socket()
  : isOpen(false), socketHandle(0)
{
} // Socket::Socket()

Result Socket::Open(
  const char * ipAddress,
  const s32 port)
{
#ifdef _MSC_VER

  // Based on example at http://www.codeproject.com/Articles/13071/Programming-Windows-TCP-Sockets-in-C-for-the-Begin

  const s32 winsockVersion = 0x0202;

  WSADATA wsadata;

  const s32 error = WSAStartup(winsockVersion, &wsadata);

  if(error)
    return RESULT_FAIL_IO;

  if(wsadata.wVersion != winsockVersion) {
    WSACleanup(); //Clean up Winsock
    return RESULT_FAIL_IO;
  }

  SOCKADDR_IN target; //Socket address information

  target.sin_family = AF_INET; // address family Internet
  target.sin_port = htons (port); //Port to connect on
  target.sin_addr.s_addr = inet_addr (ipAddress); //Target IP

  socketHandle = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
  if (socketHandle == INVALID_SOCKET) {
    const s32 lastError = WSAGetLastError();
    socketHandle = NULL;
    return RESULT_FAIL_IO; //Couldn't create the socket
  }

  // Set timeout
  struct timeval tv;

  tv.tv_sec = 5;
  tv.tv_usec = 0;

  setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

  // connect

  const s32 connectResult = connect(socketHandle, (SOCKADDR *)&target, sizeof(target));
  if (connectResult < 0) {
    return RESULT_FAIL_IO; //Couldn't connect
  }

#else // #ifdef _MSC_VER

  struct sockaddr_in target; //Socket address information

  memset(&target, 0, sizeof(target));

  target.sin_family = AF_INET; // address family Internet
  target.sin_port = htons(port); //Port to connect on

  struct hostent *host = gethostbyname(ipAddress);
  
  if(!host) {
    return RESULT_FAIL;
  }
  
  memcpy(&(target.sin_addr.s_addr), host->h_addr, host->h_length);

  socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
  if (socketHandle < 0) {
    socketHandle = 0;
    return RESULT_FAIL_IO; //Couldn't create the socket
  }

  // Set timeout
  struct timeval tv;

  tv.tv_sec = 1;
  tv.tv_usec = 0;

  setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

  // connect
  const s32 connectResult = connect(socketHandle, (struct sockaddr *)&target, sizeof(target));
  if (connectResult < 0) {
    socketHandle = 0;
    return RESULT_FAIL_IO; //Couldn't connect
  }

#endif // #ifdef _MSC_VER ... #else

  this->isOpen = true;

  return RESULT_OK;
} // Result Socket::Open()

Result Socket::Close()
{
#ifdef _MSC_VER
  if(socketHandle)
    closesocket(socketHandle);
#else
  if(socketHandle)
    shutdown(socketHandle, 2);
#endif

  this->isOpen = false;

  return RESULT_OK;
} // Result Socket::Close()

Result Socket::Read(void * buffer, s32 bufferLength, s32 &bytesRead)
{
  bytesRead = static_cast<s32>(recv(socketHandle, reinterpret_cast<char*>(buffer), bufferLength, 0));

  if(bytesRead < 0) {
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(socketHandle, &fds);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    const s32 result = select(socketHandle + 1, &fds, NULL, NULL, &timeout);

    if(result <= 0) {
      this->Close();
      return RESULT_FAIL_IO;
    } else {
      return RESULT_FAIL_IO_TIMEOUT;
    }

    //printf("");

    /*s32 errorCode;
    getsockopt(socketHandle, SOL_SOCKET, SO_ERROR, &errorCode, sizeof(errorCode))*/

    //#ifdef _MSC_VER
    //    const s32 lastError = WSAGetLastError();
    //
    //    //printf("Last Error: %d 0x%x\n", lastError, lastError);
    //
    //    if(lastError == WSAETIMEDOUT) {
    //      return RESULT_FAIL_IO_TIMEOUT;
    //    } else {
    //      this->Close();
    //      return RESULT_FAIL_IO;
    //    }
    //
    //#else
    //    //printf("Last Error: %d 0x%x\n", errno, errno);
    //
    //    if(errno == EAGAIN) {
    //      return RESULT_FAIL_IO_TIMEOUT;
    //    } else {
    //      this->Close();
    //      return RESULT_FAIL_IO;
    //    }
    //#endif
  }

  return RESULT_OK;
} // Result Socket::Read()
