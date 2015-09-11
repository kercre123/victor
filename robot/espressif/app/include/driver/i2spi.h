

#ifndef _I2S_H_
#define _I2S_H_

#include "anki/cozmo/robot/drop.h" ///< I2SPI transaction contract

/// SLC can only move 256 byte chunks so that's what we use
#define DMA_BUF_SIZE (256)
/// How often we will garuntee servicing the DMA buffers
#define DMA_SERVICE_INTERVAL_MS (5)
/// How many buffers are required given the above constraints. + 1 for ceiling function
#define DMA_BUF_COUNT ((DROP_SIZE * DROPS_PER_SECOND * DMA_SERVICE_INTERVAL_MS / 1000 / DMA_BUF_SIZE) + 1)

/// Task priority level for processing I2SPI data
#define I2SPI_PRIO USER_TASK_PRIO_2

/** Initalize the I2S peripheral, IO pins and DMA for bi-directional transfer
 * @return 0 on success or non-0 on an error
 */
int8_t i2spiInit(void);

/** Start I2S data transfer
 * The first data to be sent should be queued up via i2sQueueTx before calling this function
 */
void i2spiStart(void);

/** Stop I2S data transfer
 */
void i2spiStop(void);

/** Queues a buffer to transmit over I2S
 * @param msgData a pointer to the data to be sent to the RTIP
 * @param msgLen the number of bytes of message data pointed to by msgData
 * @param tag The type of data to be sent to the RTIP
 * @return true if the data was successfully queued or false if it could not be queued.
 */
bool i2spiQueueMessage(uint8_t* msgData, uint8_t msgLen, ToRTIPPayloadTag tag);

#endif
