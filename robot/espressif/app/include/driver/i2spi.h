

#ifndef _I2S_H_
#define _I2S_H_

#include "os_type.h"
#include "anki/cozmo/robot/drop.h" ///< I2SPI transaction contract

/// Buffer size must match I2S TX FIFO depth
#define DMA_BUF_SIZE (512) /// This must be 512 Espressif DMA to work and for logic in i2spi.c to work
/// How often we will garuntee servicing the DMA buffers
#define DMA_SERVICE_INTERVAL_MS (5)
/// How many buffers are required given the above constraints. + 1 for ceiling function
#define DMA_BUF_COUNT ((I2SPI_RAW_BYTES_PER_SECOND * DMA_SERVICE_INTERVAL_MS / 1000 / DMA_BUF_SIZE) + 1)

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
 * @warning You must reboot the chip after calling i2spiStop, DMA WILL NOT WORK until reboot.
 */
void i2spiStop(void);

/** Queues a buffer to transmit over I2S
 * @param msgData a pointer to the data to be sent to the RTIP
 * @param msgLen the number of bytes of message data pointed to by msgData
 * @return true if the data was successfully queued or false if it could not be queued.
 */
bool i2spiQueueMessage(uint8_t* msgData, uint16_t msgLen);

/// Count how many tx underruns we've had
extern uint32_t i2spiTxUnderflowCount;
/// Count how many RX overruns we've had
extern uint32_t i2spiRxOverflowCount;
/// Count how many times the drop phase has jumped more than we expected it to
extern uint32_t i2spiPhaseErrorCount;
/// Count the integral drift in the I2SPI system
extern int32_t i2spiIntegralDrift;


#endif
