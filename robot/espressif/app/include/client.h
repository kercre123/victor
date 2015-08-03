#ifndef __client_h_
#define __client_h_
/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "ip_addr.h"
#include "c_types.h"
#include "os_type.h"
#include "espconn.h"

/** Initalize the client connection
 * @return 0 for okay or non-zero on error
 */
sint8 clientInit();

/** Queues a buffer to be sent to client
 * @param Pointer to contiguous data to be sent
 * @param len The number of bytes to send
 * @return true if the packet was asuccessfully queued, false if it couldn't be queued
 */
bool clientQueuePacket(uint8* data, uint16 len);

#endif
