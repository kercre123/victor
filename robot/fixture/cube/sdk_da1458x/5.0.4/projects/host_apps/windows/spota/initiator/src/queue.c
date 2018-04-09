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
#include "uart.h"


BOOL StopRxTask; // Used to stop the uart RX task.

HANDLE UARTRxQueueSem;

HANDLE Rx232Id;  // Uart RX Thread handle

QueueRecord UARTRxQueue; // UART Rx Queue -> Main thread

/*
 ****************************************************************************************
 * @brief Initialize UART RX thread.
 *
 * @return void
 ****************************************************************************************
*/
void InitTasks(void)
{
    StopRxTask = FALSE;
    
    Rx232Id = (HANDLE) _beginthread(UARTProc, 10000, NULL);
    
    // Set thread priorities
    SetThreadPriority(Rx232Id, THREAD_PRIORITY_TIME_CRITICAL);
    
    UARTRxQueueSem = CreateMutex( NULL, FALSE, NULL );
}

/*
 ****************************************************************************************
 * @brief Adds an item to the queue
 *
 *  @param[in] rec    Queue.
 *  @param[in] vdata  Pointer to the item to be added.
 *
 * @return void
 ****************************************************************************************
*/
void EnQueue(QueueRecord *rec,void *vdata)
{
    struct QueueStorage *tmp;

    tmp = (struct QueueStorage *) malloc(sizeof(struct QueueStorage));
    tmp->Next = NULL;
    tmp->Data = vdata;
    if(rec->First == NULL)
    {
        rec->First=tmp;
        rec->Last=tmp;
    }
    else
    {
        rec->Last->Next=tmp;
        rec->Last=tmp;
    }
}

/*
 ****************************************************************************************
 * @brief Removes an item from the queue
 *
 *  @param[in] rec  Queue.
 *
 * @return pointer to the item that was removed
 ****************************************************************************************
*/
void *DeQueue(QueueRecord *rec)
{
    void *tmp;
    struct QueueStorage *tmpqe;

    if(rec->First == NULL)
        return NULL;
  
    tmpqe = rec->First;
    rec->First = tmpqe->Next;
    tmp = tmpqe->Data;
    free(tmpqe);
    
    return tmp;
}
