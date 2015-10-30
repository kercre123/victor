#ifndef __I_RECEIVER_H
#define __I_RECEIVER_H

#include "transport/reliableTransport.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Callback when a new reliable connetion request is received
 */
void Receiver_OnConnectionRequest(ReliableConnection* connection);

/** Callback when a connection request is acknowledged
 * The connection has been established
 */
void Receiver_OnConnected(ReliableConnection* connection);

/** Callback when the remote end requests a disconnect
 */
void Receiver_OnDisconnect(ReliableConnection* connection);

/** Callback when new data has been received
 * @param buffer A pointer to the message payload
 * @param bufferSize The number of bytes of payload
 * @param connection A pointer to the reliable connection structure
 */
void Receiver_ReceiveData(uint8_t* buffer, uint16_t bufferSize, ReliableConnection* connection);

// end extern "C"
#ifdef __cplusplus
}
#endif

#endif
