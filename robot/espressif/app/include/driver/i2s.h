#ifndef _I2S_H_
#define _I2S_H_

#include "anki/cozmo/robot/i2spiData.h" ///< I2SPI transaction parameters

/** Initalize the I2S peripheral, IO pins and DMA for bi-directional transfer
 * @return 0 on success or non-0 on an error
 */
int8_t i2sInit(void);

/** Start I2S data transfer
 * The first data to be sent should be queued up via i2sQueueTx before calling this function
 */
void i2sStart(void);

/** Stop I2S data transfer
 */
void i2sStop(void);

/** Queues a buffer to transmit over I2S
 * @param payload The data to be sent
 * @param tag The type dag for the payload
 * @return true if the data was successfully queued or false if it could not be queued.
 */
bool i2sQueueTx(I2SPIPayload* payload, I2SPIPayloadTag tag);

/** Callback for new received data from RTIP
 * @WARNING This function is called from an ISR so it must handle data quickly and return. Any long running data
 * processing should be scheduled into a task.
 * @param payload The received data
 * @param tag The type dag for the payload
 */
void i2sRecvCallback(I2SPIPayload* payload, I2SPIPayloadTag tag);

/// Retrieve the count of missed transmits (where we didn't send anything in a transfer)
uint32_t i2sGetMissedTransmits(void);
/// Retrieve the number of times we've received a message with the wrong sequence number
uint32_t i2sGetOutOfSequence(void);

#endif
