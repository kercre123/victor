/** A pure C implementation of the Anki reliable Transport Protocol Version 3.
 * for use on embedded systems.
 * @author Daniel Casner <daniel@anki.com>
 *
 * The embedded reliable transport implementation relies on a number of externalsymbols being defined.
 */

#ifndef __EmbeddedReliableTransport_H__
#define __EmbeddedReliableTransport_H__

#ifdef TARGET_ESPRESSIF
#include "c_types.h"
#else
#include <stdint.h>
#endif

#include "transport/reliableMessageTypes.h"
#include "transport/reliableSequenceId.h"
#include "transport/IUnreliableTransport.h"

#ifdef __cplusplus
// Bring some specific things into the namespace
#define EReliableMessageType Anki::Util::EReliableMessageType
#define eRMT_SingleReliableMessage   ::Anki::Util::eRMT_SingleReliableMessage
#define eRMT_SingleUnreliableMessage ::Anki::Util::eRMT_SingleUnreliableMessage
#define ReliableSequenceId   Anki::Util::ReliableSequenceId
extern "C" {
#else
#ifndef TARGET_ESPRESSIF
#include "utilEmbedded/cBool.h"
#endif
#endif

/** 9 byte prefix for reliable packet header
 * For the embedded implementation we just concatinate the Anki UDP Packet header and the reliable packet header
 * assuming that we will only be using the transport for this protocol. This saves another buffer copy.
 */
#define RELIABLE_PACKET_HEADER_PREFIX "COZ\x03RE\x01"
#define RELIABLE_PACKET_HEADER_PREFIX_LENGTH 7

#define GLOBAL_INVALID_TAG 0x00

/** Structure for reliable packet header
 */
#ifdef __APPLE__
typedef struct __attribute__((__packed__)) _AnkiReliablePacketHeader
#else
typedef struct _AnkiReliablePacketHeader
#endif
{
  uint8_t PREFIX[RELIABLE_PACKET_HEADER_PREFIX_LENGTH]; // 0+7
  EReliableMessageType type; // 7 + 1
  ReliableSequenceId seqIdMin; // 8 + 2
  ReliableSequenceId seqIdMax; // 10 + 2
  ReliableSequenceId lastReceivedId; // 12 + 2 = 14
} AnkiReliablePacketHeader;

/// Sub header in between multiple messages packed into one packet
#define ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH 3
/// How much we can send in one packet
#define ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE (UnreliableTransport_MAX_BYTES_PER_PACKET - sizeof(AnkiReliablePacketHeader) - ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH)

/// Unacked message retransmit interval in units of whatever GetCurrentTime returns
#define ReliableConnection_UNACKED_MESSAGE_SEPARATION_TIME 33
/// Maximum time without receiving a message before we consider it dead
#define ReliableConnection_CONNECTION_TIMEOUT 5000
/// A warning time before actual timeout
#define ReliableConnection_CONNECTION_PRETIMEOUT 1000

/** Structure for pending reliable messages in the queue
 * This is inserted as a header in between messages in the reliable message queue buffers
 */
typedef struct _PendingReliableMessage
{
  uint16_t sequenceNumber; ///< Reliable transport sequence number of individual message
  uint16_t messageSize; ///< The size of the message
  EReliableMessageType messageType; ///< Type of the individual message
} PendingReliableMessageMetaData;

/// Number of pending reliable messages we will queue per connection
#define ReliableConnection_PENDING_MESSAGE_QUEUE_LENGTH 32

/** Structure for maintaining information about a reliable connection
 * dest is a void pointer which is passed directly to the unreliable transport layer. We use void pointer so as not to
 * tie ourselves to any particular embedded implementation.
 * There are no ping statistcs tracked on the embedded side. We will just respond to pings from the base station but do
 * not send any ourselves.
 */
typedef struct _ReliableConnection
{
  void* dest; ///< Pointer to unreliable connection, must be first message so we can use as pointer to connection
  ReliableSequenceId nextOutSequenceId; ///< Sequence ID for the next outbound reliable message we send
  ReliableSequenceId nextInSequenceId; ///< The next sequence ID we are looking for
  uint32_t latestUnackedMessageSentTime; ///< The last time we sent unacknowledged messages
  uint32_t latestRecvTime; ///< latestRecvTime
  uint16_t txQueued; ///< The number of bytes of data queued up in txBuf
  uint16_t pendingReliableBytes; ///< The number of bytes of pending reliable messages stored
  uint8_t  numPendingReliableMessages; ///< The number of reliable messages currently queued
  uint8_t  txBuf[UnreliableTransport_MAX_BYTES_PER_PACKET]; ///< Buffer for forming packets
  uint8_t  canary1; ///< Here to die if somebody overruns txBuf.
  uint8_t  pendingMessages[ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE]; ///< Data storage for buffered reliable messages
  uint8_t  canary2;
  PendingReliableMessageMetaData pendingMsgMeta[ReliableConnection_PENDING_MESSAGE_QUEUE_LENGTH]; ///< Queue of pending messages
  uint8_t  canary3;
} ReliableConnection;

/** Initalizes the reliable transport data structures.
 */
void ReliableTransport_Init(void);

/** Initalizes a new ReliableConnection structure.
 * This method must be called on a ReliableConnection structure before it is passed to any of the below functions.
 * When a connection has been torn down / disconnected / timed out / etc. this method should be called again to reset
 * the connection before reusing it.
 * @param this A pointer to memory allocated to contain a ReliableConnection structure
 * @param dest the unreliable transport destination.
 */
void ReliableConnection_Init(ReliableConnection* self, void* dest);

/** Connect to a remote host
 * @param dest A connection to establish reliable communication over, passed to unreliable transport.
 * @return True if successful or false if an error
 */
bool ReliableTransport_Connect(ReliableConnection* connection);

/** Respond to a connection request from the basestation.
 * @param dest The connection that we are responding to, passed to unreliable transport.
 * @return True if successful or false if an error
 */
bool ReliableTransport_FinishConnection(ReliableConnection* connection);

/** Close a reliable connection.
 * @param connection The connection to close out. This method will free the connection so it must not be referenced
 * after calling this function.
 * @return True if successful or false if there was an error
 */
bool ReliableTransport_Disconnect(ReliableConnection* connection);

/** Queue transmission of a reliable message
 * @param buffer Pointer to the data to be sent
 * @param bufferSize The number of bytes to be sent (size of buffer)
 * @param connection The reliable connection structure associated with this transmission.
 * @param messageType Specify the message type, default eRMT_SingleReliableMessage. To send an unreliable message use
 *                    eRMT_SingleUnreliableMessage.
 * @param hot Whether this message needs to be transmitted imeediately or if we should wait for next transmit time or
 *            hot message.
 * @param tag If specified, buffer will be prepended with the specified one byte tag. Otherwise buffer is used directly.
 * @return True if packet successfully queued or false if there was no room.
 */
bool ReliableTransport_SendMessage(const uint8_t* buffer, const uint16_t bufferSize, ReliableConnection* connection,
                                   const EReliableMessageType messageType,
                                   const bool hot, const uint8_t tag);

/** ReceiveData function to be called by unreliable transport
 * @param connection A pointer to the reliable connection on which this was received
 * @param buffer Pointer to the data received
 * @param bufferSize The number of bytes received (size of buffer)
 * @return return true if ReliableTransport handled the data, false if it didn't take it
 */
int16_t ReliableTransport_ReceiveData(ReliableConnection* connection, uint8_t* buffer, uint16_t bufferSize);

/** Main loop tick update for the reliable connection.
 * This must be called periodically for each ReliableConnection in order to execute reliable transmission actions etc.
 * @param connection The ReliableConnection to process periodic updates for
 * @return returns false if the connection has timed out or true normally
 */
bool ReliableTransport_Update(ReliableConnection* connection);

/** Debugging function to print the state of the reliable connection
 * @param connection The connection instance to print
 */
void ReliableConnection_printState(ReliableConnection* connection);


// end extern "C"
#ifdef __cplusplus
}
#endif

#endif
