
/**
****************************************************************************************
*
* @file queue.c
*
* @brief Software for queues and threads.
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

#include "queue.h"
//////////#include "console.h"
#include "uart.h"

// Used to stop the tasks.
BOOL StopRxTask;

HANDLE UARTRxQueueSem;     // mutex semaphore to protect TestbusFrameQueue

HANDLE Rx232Id; // Thread handles

QueueRecord UARTRxQueue; //Queues UARTRx -> Main thread /  Console -> Main thread

HANDLE QueueHasAvailableData;

void InitTasks(void)
{
   StopRxTask = FALSE;

   Rx232Id   = (HANDLE) _beginthread(UARTProc, 10000, NULL);

   // Set thread priorities
   SetThreadPriority(Rx232Id, THREAD_PRIORITY_TIME_CRITICAL);

   UARTRxQueueSem = CreateMutex( NULL, FALSE, NULL );

   QueueHasAvailableData = CreateEvent(0, TRUE, FALSE, NULL);
}

void EnQueue(QueueRecord *rec,void *vdata)
{
  struct QueueStorage *tmp;
  tmp=(struct QueueStorage *) malloc(sizeof(struct QueueStorage));
  tmp->Next=NULL;
  tmp->Data=vdata;
  if(rec->First==NULL) {
    rec->First=tmp;
    rec->Last=tmp;
  } else {
    rec->Last->Next=tmp;
    rec->Last=tmp;
  }
  SetEvent(QueueHasAvailableData);
}

void *DeQueue(QueueRecord *rec)
{
  void *tmp;
  struct QueueStorage *tmpqe;
  if(rec->First==NULL)
  {
	  ResetEvent(QueueHasAvailableData);
    return NULL;
  }
  tmpqe=rec->First;
  rec->First=tmpqe->Next;
  tmp=tmpqe->Data;
  free(tmpqe);
  if(rec->First==NULL) 
	  ResetEvent(QueueHasAvailableData);
  return tmp;
}
