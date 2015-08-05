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

/** Callback for new received data from WiFi client
 * This function is called by but not defined in the client module and must be defined by the user application.
 * @WARNING This function is called from radio service callback so it should be handled quickly and any long running
 * processing should be scheduled into a task.
 * @param payload The received data
 * @param tag The type dag for the payload
 */
void clientRecvCallback(char* data, unsigned short len);


#endif
