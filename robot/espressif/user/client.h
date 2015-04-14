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
#define PKT_BUFFER_SIZE 1500

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
  struct UDPPacket* next; ///< Used internally to this module @warning to not modify
  uint16 len; ///< Length of data
  uint8 data[PKT_BUFFER_SIZE]; ///< Packet data
  UDPPacketState state; ///< Used internally to this module @warning do not modify
} UDPPacket;

/** Callback type for data received by client
 * @param data The packet data
 * @param len The number of bytes of data received
 */
typedef void (* clientReceiveCB)(uint8* data, uint16 len);

/** Initalize the client connection
 * @param recvFtn Callback function for receiving packets
 * @return 0 for okay or non-zero on error
 */
sint8 clientInit(clientReceiveCB recvFtn);

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
