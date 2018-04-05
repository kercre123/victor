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


#include <stdlib.h>
#include <stddef.h>     // standard definition     


// Queue stuff.
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

extern QueueRecord SPIRxQueue, SPITxQueue; //Queues UARTRx -> Main thread /  Console -> Main thread

/*
 ****************************************************************************************
 * @brief Adds an item to the queue
 *
 *  @param[in] rec    Queue.
 *  @param[in] vdata  Pointer tothe item to be added.
 *
 * @return void
 ****************************************************************************************
*/
void EnQueue(QueueRecord *rec,void *vdata);
/*
 ****************************************************************************************
 * @brief Removes an item from the queue
 *
 *  @param[in] rec  Queue.
 *
 * @return pointer to the item that was removed
 ****************************************************************************************
*/
void *DeQueue(QueueRecord *rec);



#endif //QUEUE_H_
