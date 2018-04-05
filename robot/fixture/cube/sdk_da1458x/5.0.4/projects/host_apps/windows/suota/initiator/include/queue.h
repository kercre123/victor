/**
 ****************************************************************************************
 *
 * @file queue.h
 *
 * @brief Header file for queues and threads definitions.
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

#ifndef QUEUE_H_
#define QUEUE_H_

#include <conio.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <stddef.h>     // standard definition


#include "stdtypes.h"     


struct QueueStorage {
  struct QueueStorage *Next;
  void *Data;
};

typedef struct {
  struct QueueStorage *First,*Last;
} QueueRecord;

typedef struct {
  unsigned char  bLength;
  unsigned char  bData[1];
} QueueElement;


extern BOOL StopRxTask; // Used to stop the uart RX task.

extern HANDLE UARTRxQueueSem; // mutex semaphore to protect the uart RX queue

extern HANDLE Rx232Id; // Uart RX Thread handle

extern QueueRecord UARTRxQueue; // UART Rx Queue -> Main thread 

void EnQueue(QueueRecord *rec,void *vdata);
void *DeQueue(QueueRecord *rec);

void InitTasks(void);


#endif //QUEUE_H_