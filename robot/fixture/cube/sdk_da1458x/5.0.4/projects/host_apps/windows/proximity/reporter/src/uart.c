/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief Uart interface for HCI messages.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include <conio.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <stddef.h>     // standard definition
#include "queue.h"
#include "stdtypes.h"
#include "uart.h"

//#define COMM_DEBUG

// Packet type for fully embedded interface messages (RW BLE non-standard Higher Layer interface).
// See "RW BLE Host Interface Specification" (RW-BLE-HOST-IS).
#define FE_MSG_PACKET_TYPE 0x05

HANDLE hComPortHandle = NULL;
OVERLAPPED ovlRd,ovlWr;

/*
 ****************************************************************************************
 * @brief Write message to UART.
 *
 *  @param[in] size  Message's size.
 *  @param[in] data  Pointer to message's data.
 *
 * @return void.
 ****************************************************************************************
*/
void UARTSend(unsigned short size, unsigned char *data)
{
    unsigned char bTransmit232ElementArr[500];
    unsigned short bSenderSize;
    unsigned long dwWritten;

    bTransmit232ElementArr[0] = FE_MSG_PACKET_TYPE;
    memcpy(&bTransmit232ElementArr[1], data, size);

    bSenderSize = size + 1;

    ovlWr.Offset     = 0;
    ovlWr.OffsetHigh = 0;
    ResetEvent(ovlWr.hEvent);

    WriteFile(hComPortHandle, bTransmit232ElementArr, bSenderSize, &dwWritten, &ovlWr);
}

/*
 ****************************************************************************************
 * @brief Send message received from UART to application's main thread.
 *
 *  @param[in] length           Message's size.
 *  @param[in] bInputDataPtr    Pointer to message's data.
 *
 * @return void.
 ****************************************************************************************
*/
void SendToMain(unsigned short length, uint8_t *bInputDataPtr)
{
    unsigned char *bDataPtr = (unsigned char *) malloc(length);

    memcpy(bDataPtr, bInputDataPtr, length);

    WaitForSingleObject(UARTRxQueueSem, INFINITE);
    EnQueue(&UARTRxQueue, bDataPtr);
    ReleaseMutex(UARTRxQueueSem);
}

/*
 ****************************************************************************************
 * @brief UART Reception thread loop.
 *
 * @return void.
 ****************************************************************************************
*/
void UARTProc(PVOID unused)
{
    unsigned long dwBytesRead;
    unsigned char tmp;
    unsigned short wReceive232Pos=0;
    unsigned short wDataLength;
    unsigned char bReceive232ElementArr[1000];
    unsigned char bReceiveState = 0;
    unsigned char bCkeckSum = 0;
    unsigned char bHdrBytesRead = 0;

    while(StopRxTask == FALSE)
    {
        ovlRd.Offset     = 0;
        ovlRd.OffsetHigh = 0;
        ResetEvent(ovlRd.hEvent);

        // use overlapped read, not because of async read, but, due to
        // multi thread read/write
        ReadFile( hComPortHandle, &tmp, 1, &dwBytesRead, &ovlRd );

        GetOverlappedResult( hComPortHandle,
                            &ovlRd,
                            &dwBytesRead,
                            TRUE );

        switch(bReceiveState)
        {
         case 0:   // Receive FE_MSG
            if(tmp == FE_MSG_PACKET_TYPE)
            {
               bReceiveState = 1; 
               wDataLength = 0;
               wReceive232Pos = 0;
               bHdrBytesRead = 0;

               bReceive232ElementArr[wReceive232Pos]=tmp;
               wReceive232Pos++;

                #ifdef COMM_DEBUG
                    printf("\nI: ");
                    printf("%02X ", tmp);
                #endif   
            }
            else
            {
                  #ifdef COMM_DEBUG
                     printf("%02X ", tmp);
                  #endif
            }
            break;

         case 1:   // Receive Header size = 6
               #ifdef COMM_DEBUG
                  printf("%02X ", tmp);
               #endif
             bHdrBytesRead++;
             bReceive232ElementArr[wReceive232Pos] = tmp;
             wReceive232Pos++;

             if (bHdrBytesRead == 6)
                 bReceiveState = 2;
                
             break;
         case 2:   // Receive LSB of the length
            #ifdef COMM_DEBUG
                printf("%02X ", tmp);
            #endif
            wDataLength += tmp;
            if(wDataLength > MAX_PACKET_LENGTH)
            {
                 bReceiveState = 0;
            }
            else
            {
                bReceive232ElementArr[wReceive232Pos] = tmp;
                wReceive232Pos++;
                bReceiveState = 3;
            }
          break;
         case 3:   // Receive MSB of the length
               #ifdef COMM_DEBUG
                  printf("%02X ", tmp);
               #endif
            wDataLength += (unsigned short) (tmp*256);
            if(wDataLength > MAX_PACKET_LENGTH)
            {

                #ifdef COMM_DEBUG
                    printf("\nSIZE: %d ", wDataLength);
                #endif
                bReceiveState = 0;
            }
            else if(wDataLength == 0)
            {
                #ifdef COMM_DEBUG
                    printf("\nSIZE: %d ", wDataLength);
                #endif
                SendToMain((unsigned short) (wReceive232Pos-1), &bReceive232ElementArr[1]);
                bReceiveState = 0;
            }
            else
            {
               bReceive232ElementArr[wReceive232Pos] = tmp;
               wReceive232Pos++;
               bReceiveState = 4;
            }
            break;
         case 4:   // Receive Data
            #ifdef COMM_DEBUG
                printf("%02X ", tmp);
            #endif
            bReceive232ElementArr[wReceive232Pos] = tmp;
            wReceive232Pos++;
            
            if(wReceive232Pos == wDataLength + 9 ) // 1 ( first byte = FE_MSG_PACKET_TYPE) + 2 (Type) + 2 (dstid) + 2 (srcid) + 2 (lengths size)
            {
               // Sendmail program
               SendToMain((unsigned short) (wReceive232Pos-1), &bReceive232ElementArr[1]);
               bReceiveState = 0;
                #ifdef COMM_DEBUG
                    printf("\nSIZE: %d ", wDataLength);
                #endif
            }
           break;

        }

    }
    StopRxTask = TRUE;   // To indicate that the task has stopped
    
    PurgeComm(hComPortHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

    Sleep(100);

    CloseHandle(hComPortHandle);

    ExitThread(0);
}

/*
 ****************************************************************************************
 * @brief Init UART iface.
 *
 *  @param[in] Port         COM prot number.
 *  @param[in] BaudRate     Baud rate.
 *
 * @return -1 on failure / 0 on success.
 ****************************************************************************************
*/
uint8_t InitUART(int Port, int BaudRate)
{
    DCB dcb;
    DWORD dwErrorCode;
    BOOL fSuccess;
    COMSTAT stat;
    DWORD error;
    COMMTIMEOUTS commtimeouts;
    char CPName[13];
    //DWORD dwEvtMask;

    sprintf(CPName, "\\\\.\\COM%d", Port);
    printf("Connecting to %s\n", &CPName[4]);

    ovlRd.hEvent = CreateEvent( NULL,FALSE,FALSE,NULL );
    ovlWr.hEvent = CreateEvent( NULL,FALSE,FALSE,NULL );

    hComPortHandle = CreateFile(CPName,
                                GENERIC_WRITE | GENERIC_READ,
                                0, //FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED,
                                NULL );

    if(hComPortHandle == INVALID_HANDLE_VALUE)
    {
        dwErrorCode = GetLastError();
        return -1;
    }

    ClearCommError( hComPortHandle, &error, &stat );
   
    memset(&dcb, 0x0, sizeof(DCB) );
    fSuccess = GetCommState(hComPortHandle, &dcb);
    if(!fSuccess)
    {
        return -1;
    }

    // Fill in the DCB
    dcb.BaudRate = BaudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = 1;
    dcb.fOutxCtsFlow = TRUE; // enable RTS/CTS flow control
    dcb.fOutxDsrFlow = 0;
    dcb.fDtrControl  = DTR_CONTROL_DISABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE; // enable RTS/CTS flow control
    dcb.fInX         = 0;
    dcb.fOutX        = 0;
    dcb.fErrorChar   = 0;
    dcb.fNull        = 0;
    dcb.fAbortOnError = 0;

    fSuccess = SetCommState(hComPortHandle, &dcb);
    if(!fSuccess)
    {
        printf("Failed to set DCB!\n");
        return -1;
    }
    commtimeouts.ReadIntervalTimeout = 1000; 
    commtimeouts.ReadTotalTimeoutMultiplier = 0; 
    commtimeouts.ReadTotalTimeoutConstant = 0; 
    commtimeouts.WriteTotalTimeoutMultiplier = 0; 
    commtimeouts.WriteTotalTimeoutConstant = 0;

    fSuccess = SetCommTimeouts( hComPortHandle,
                                &commtimeouts );
 
    printf("%s succesfully opened, baud rate %d\n", &CPName[4], BaudRate);

    return 0;
}
