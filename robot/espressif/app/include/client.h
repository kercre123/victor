#ifndef __client_h_
#define __client_h_
/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "ip_addr.h"
#include "c_types.h"
#include "os_type.h"
#include "espconn.h"

/** Packet buffer size */
#define PKT_BUFFER_SIZE 1472

typedef enum
{
  PKT_BUF_AVAILABLE,
  PKT_BUF_RESERVED,
  PKT_BUF_QUEUED,
} UDPPacketState;


/** Buffer for storing UDP packets for sending or receiving
 */
typedef struct UDPPacket
{
  uint16 len; ///< Length of data
  uint8 data[PKT_BUFFER_SIZE]; ///< Packet data
  UDPPacketState state; ///< Used internally to this module @warning do not modify
} UDPPacket;

/** Initalize the client connection
 * @return 0 for okay or non-zero on error
 */
sint8 clientInit();

/** Get a packet buffer for writing
 * @return A pointer to a UDPPacket buffer for sending or NULL if none available. If a packet is provided, the packet
 * must be returned by either a call to queuePacket or releasePacket.
 */
UDPPacket* clientGetBuffer();

/** Queues a buffer to be sent to client
 * @param pkt The packet to queued. data and len must be set before calling.
 */
void clientQueuePacket(UDPPacket* pkt);

/** Releases a packet back to the pool without queuing it to send
 * @param pkt The packet to release.
 */
void clientFreePacket(UDPPacket* pkt);

#endif
