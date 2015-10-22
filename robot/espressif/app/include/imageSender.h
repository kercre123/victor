#ifndef __img_sender_h_
#define __img_sender_h_
/** @file A simple function to send image data to the engine in CLAD messages
 * @author Daniel Casner
 */
 
#include "os_type.h"

/** Cross link function for queuing image Data
 * @param imgData A pointer to the image data to queue
 * @param len     The number of bytes to take from imgData
 * @param eof     True if this is the end of an image
 */
void imageSenderQueueData(uint8_t* imgData, uint8_t len, bool eof);


#endif
