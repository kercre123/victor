#ifndef __client_h_
#define __client_h_
/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include <stdint.h>
#include "os_type.h"

/** Initalize the client connection
 * @return 0 for okay or non-zero on error
 */
sint8 clientInit();

/** Method for sending messages NOT PACKETS
 * @param msgID The ID (tag) of the message to be sent
 * @param buffer A pointer to the message to be sent
 * @param reliable Specifify if the message should be transferred reliably. Default true.
 * @param hot Specify if the message is hot and needs to be sent imeediately. Default false.
 * @return True if sucessfully queued, false otherwise
 */
bool clientSendMessage(const u8* buffer, const u16 size, const u8 msgID, const bool reliable, const bool hot);

/** Callback for new received data from WiFi client
 * This function is called by but not defined in the client module and must be defined by the user application.
 * @WARNING This function is called from radio service callback so it should be handled quickly and any long running
 * processing should be scheduled into a task.
 * @param dest A pointer to the network endpoint the message came from
 * @param payload The received data
 * @param tag The type dag for the payload
 */
void clientRecvCallback(const void* dest, u8* payload, const u16 len);

/** Periodic update function for client.
 */
void clientUpdate(void);

/// Check if client is connected.
bool clientConnected(void);

/** Cross link function for queuing image Data
 * @param imgData A pointer to the image data to queue
 * @param len     The number of bytes to take from imgData
 * @param eof     True if this is the end of an image
 */
void clientQueueImageData(uint8_t* imgData, uint8_t len, bool eof);

#endif
