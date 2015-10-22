/** Pure C impelementation of embedded reliable transport
 * @author Daniel Casner <daniel@anki.com>
 */

#include "transport/reliableTransport.h"
#include "transport/IReceiver.h"

#if defined(TARGET_ESPRESSIF)
#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"
#define printf os_printf
#define memcpy os_memcpy
#define strncmp os_strncmp
#define GetMicroCounter system_get_time
#else
#include <string.h>
#include <stdio.h>
#define IRAM_ATTR
#endif


#ifdef ROBOT_HARDWARE
// Disable printf when on the real robot
void board_printf(char *format, ...);
void FaceUnPrintf(void);
void FacePrintf(const char *format, ...);
#define printf FacePrintf
#define printClear FaceUnPrintf
#else
#define printClear() (void)0
#endif

/// Utility function to print the state of a connection object
void ICACHE_FLASH_ATTR ReliableConnection_printState(ReliableConnection* connection)
{
  printf("nO %u nI %u\r\nlUmst %u\r\nlrt   %u\r\ntxq %u\r\nprb %u prm %u\r\n",
             connection->nextOutSequenceId,
             connection->nextInSequenceId,
             (unsigned int)connection->latestUnackedMessageSentTime,
             (unsigned int)connection->latestRecvTime,
             connection->txQueued,
             connection->pendingReliableBytes,
             connection->numPendingReliableMessages);
}

/// Reset the reliable connection TX queue state
static void resetTxQueue(ReliableConnection* self)
{
  AnkiReliablePacketHeader* header = (AnkiReliablePacketHeader*)self->txBuf;
  header->type     = eRMT_MultipleMixedMessages;
  header->seqIdMin = k_InvalidReliableSeqId;
  header->seqIdMax = k_InvalidReliableSeqId;
  self->txQueued = sizeof(AnkiReliablePacketHeader);
}

bool haveDataToSend(ReliableConnection* self)
{
  return self->txQueued > sizeof(AnkiReliablePacketHeader);
}

void ICACHE_FLASH_ATTR ReliableTransport_Init()
{
  //printf("ReliableTransport Initalizing\r\n");
}

void ICACHE_FLASH_ATTR ReliableConnection_Init(ReliableConnection* self, void* dest)
{
  uint8_t i;

  self->dest = dest;
  self->nextOutSequenceId    = k_MinReliableSeqId;
  self->nextInSequenceId     = k_MinReliableSeqId;
  self->latestUnackedMessageSentTime = 0;
  self->latestRecvTime = GetMicroCounter();
  memcpy(self->txBuf, RELIABLE_PACKET_HEADER_PREFIX, RELIABLE_PACKET_HEADER_PREFIX_LENGTH);
  resetTxQueue(self);
  for (i=0; i<ReliableConnection_PENDING_MESSAGE_QUEUE_LENGTH; ++i)
  {
    self->pendingMsgMeta[i].sequenceNumber = k_InvalidReliableSeqId;
    self->pendingMsgMeta[i].messageSize = 0;
    self->pendingMsgMeta[i].messageType = eRMT_Invalid;
  }
  self->pendingReliableBytes = 0;
  self->numPendingReliableMessages = 0;
  self->canary1 = 0;
  self->canary2 = 0;
  self->canary3 = 0;
}

/// Add a reliable message to the pending reliable message queue
static bool QueueMessage(const uint8_t* buffer, const uint16_t bufferSize, ReliableConnection* connection, const EReliableMessageType messageType, const uint8_t tag)
{
  if (bufferSize > ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE)
  {
    printf("ERROR: Reliable transport can't ever send message of %d bytes > MTU %d\r\n", bufferSize, (int)(ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE));
    return false;
  }
  else if (connection->numPendingReliableMessages >= ReliableConnection_PENDING_MESSAGE_QUEUE_LENGTH)
  {
    printf("WARN: No slots left for pending reliable messages\r\n");
    return false;
  }
  else if ((bufferSize + ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH + 1) > /* +1 for tag, easier to assume it's there than check. */
           (ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE - connection->pendingReliableBytes))
  {
    printf("WARN: No room for pending reliable message data. %d bytes, %d messages queued\r\n", connection->pendingReliableBytes, connection->numPendingReliableMessages);
    return false;
  }
  else
  {
    uint8_t* msg = &(connection->pendingMessages[connection->pendingReliableBytes]);
    PendingReliableMessageMetaData* meta = &(connection->pendingMsgMeta[connection->numPendingReliableMessages]);
    uint16_t msgSubHeaderSizeField = bufferSize;

    // Update the connection status
    connection->pendingReliableBytes += ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH + bufferSize + 1*(tag != GLOBAL_INVALID_TAG);
    connection->numPendingReliableMessages += 1;

    // Store the meta data
    meta->sequenceNumber = connection->nextOutSequenceId;
    connection->nextOutSequenceId = NextSequenceId(connection->nextOutSequenceId);
    meta->messageSize = bufferSize + ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH + 1*(tag != GLOBAL_INVALID_TAG); // Number of bytes committed to this message
    meta->messageType = messageType;

    // Add the sub message header to the buffer
    *msg = messageType; msg++;
    *msg = msgSubHeaderSizeField & 0xff; msg++; // Have to do two writes here because unaligned write isn't supported on espressif
    *msg = msgSubHeaderSizeField >> 8;   msg++;
    // Add tag if specified
    if (tag != GLOBAL_INVALID_TAG)
    {
      *msg = tag; msg ++;
    }

    // Copy in the message
    if (bufferSize)
    {
      memcpy(msg, buffer, bufferSize);
    }

    return true;
  }
}

/** Append as many pending reliable messages as possible to the outgoing packet
 * @param connection The ReliableConnection in question
 * @param newestFirst If true (default), the pending message queue will be fifo, otherwise lifo
 * @return The number of messages which were appended
 */
uint8_t AppendPendingMessages(ReliableConnection* connection, bool newestFirst)
{
  int16_t i; // Loop variable
  if (connection->numPendingReliableMessages == 0) // Nothing to append
  {
    connection->latestUnackedMessageSentTime = GetMicroCounter();
    return 0;
  }
  else
  {
    uint16_t available = UnreliableTransport_MAX_BYTES_PER_PACKET - connection->txQueued; // Room in the packet
    uint16_t numBytesToSend    = 0; // Amount we're adding
    uint8_t  numMessagesToSend = 0; // Number of messages we're adding

    i = newestFirst ? connection->numPendingReliableMessages-1 : 0;
    while (((newestFirst == true) && (i >= 0)) || ((newestFirst == false) && (i < connection->numPendingReliableMessages)))
    {
      // Don't need to add header length because it's already in messageSize
      const uint16_t numBytesIfAdded = numBytesToSend + connection->pendingMsgMeta[i].messageSize;
      if (numBytesIfAdded <= available) // There's room for this one
      {
        numBytesToSend = numBytesIfAdded;
        numMessagesToSend += 1;
      }
      else // No room for this
      {
        break;
      }
      i = newestFirst ? i-1 : i+1;
    }

    if (numMessagesToSend == 0)
    {
      return 0;
    }
    else
    {
      AnkiReliablePacketHeader* header = (AnkiReliablePacketHeader*)connection->txBuf;
      uint16_t pendingOffset = 0;
      const uint8_t firstToSend = newestFirst ? connection->numPendingReliableMessages-numMessagesToSend : 0;
      const uint8_t lastToSend  = newestFirst ? connection->numPendingReliableMessages-1                 : numMessagesToSend-1;
      for (i=0; i<firstToSend; ++i)
      {
        pendingOffset += connection->pendingMsgMeta[i].messageSize;
      }
      header->seqIdMin = connection->pendingMsgMeta[firstToSend].sequenceNumber;
      header->seqIdMax = connection->pendingMsgMeta[lastToSend].sequenceNumber;
      memcpy(connection->txBuf + connection->txQueued, connection->pendingMessages + pendingOffset, numBytesToSend);
      connection->txQueued += numBytesToSend;
      if (firstToSend == 0)
      {
	       connection->latestUnackedMessageSentTime = GetMicroCounter();
      }
      return numMessagesToSend;
    }
  }
}

/// Send the contents of the TxBuf if possible
bool SendTxBuf(ReliableConnection* connection)
{
  AnkiReliablePacketHeader* header = (AnkiReliablePacketHeader*)connection->txBuf;
  // Last thing we've received is one previous to the next thing we're expecting by definition
  header->lastReceivedId = PreviousSequenceId(connection->nextInSequenceId);

  if (UnreliableTransport_SendPacket(connection->txBuf, connection->txQueued) == false)
  {
    //printf("WARN: ReliableTransport could't send pending messages\r\n");
    return false;
  }
  else
  {
    //printf("  Sent: siz=%d typ=%d seq=%d..%d ack=%d\r\n", connection->txQueued,
    //  header->type, header->seqIdMin, header->seqIdMax, header->lastReceivedId);
    resetTxQueue(connection);
    return true;
  }
}

/// Trigger sending reliable messages
bool SendPendingMessages(ReliableConnection* connection, bool newestFirst)
{
  AppendPendingMessages(connection, newestFirst);
  if (haveDataToSend(connection))
  {
    return SendTxBuf(connection);
  }
  else
  {
    return false;
  }
}

bool ReliableTransport_Connect(ReliableConnection* connection)
{
  if (QueueMessage(NULL, 0, connection, eRMT_ConnectionRequest, GLOBAL_INVALID_TAG) == false)
  {
    //printf("ERROR: couldn't queue connect message on new connection\r\n");
    return false;
  }
  else
  {
    return SendPendingMessages(connection, true); // Connect message is always hot
  }
}

bool ReliableTransport_FinishConnection(ReliableConnection* connection)
{
  if (QueueMessage(NULL, 0, connection, eRMT_ConnectionResponse, GLOBAL_INVALID_TAG) == false)
  {
    //printf("ERROR: couldn't queue finish message on new connection\r\n");
    return false;
  }
  else
  {
    return SendPendingMessages(connection, true); // Connection responses are always hot
  }
}

bool ReliableTransport_Disconnect(ReliableConnection* connection)
{
  if (QueueMessage(NULL, 0, connection, eRMT_DisconnectRequest, GLOBAL_INVALID_TAG) == false)
  {
    //printf("ERROR: couldn't queue disconnect message\r\n");
    return false;
  }
  else
  {
    bool ret = SendPendingMessages(connection, true);
    return ret;
  }
}

/** Receive ACKs of reliable packets from the specified connection.
 * @param connection ReliableConnection for which we have received ACK
 * @param seqId The highest sequence number which has been ACKed
 * @return The number of messages which were just acked.
 */
uint8_t ICACHE_FLASH_ATTR UpdateLastAckedMessage(ReliableConnection* connection, ReliableSequenceId seqId)
{
  uint8_t updated = 0;
  if (seqId != k_InvalidReliableSeqId && connection->numPendingReliableMessages > 0)
  {
    uint16_t numBytesAcked = 0;
    const uint8_t numPending = connection->numPendingReliableMessages;
    while (updated < numPending)
    {
      const ReliableSequenceId firstID = connection->pendingMsgMeta[updated].sequenceNumber;
      const ReliableSequenceId lastID  = connection->pendingMsgMeta[numPending-1].sequenceNumber;
      if (IsSequenceIdInRange(seqId, firstID, lastID))
      {
        numBytesAcked += connection->pendingMsgMeta[updated].messageSize;
        updated += 1;
      }
      else
      {
        break;
      }
    }
    // Shift down the message data and meta data
    connection->pendingReliableBytes -= numBytesAcked;
    connection->numPendingReliableMessages -= updated;
    if (connection->numPendingReliableMessages != 0)
    {
      memcpy((void*)(connection->pendingMessages), (void*)(connection->pendingMessages + numBytesAcked), connection->pendingReliableBytes);
      memcpy((void*)(connection->pendingMsgMeta),  (void*)(connection->pendingMsgMeta  + updated),       (connection->numPendingReliableMessages * sizeof(PendingReliableMessageMetaData)));
    }
  }
  connection->latestRecvTime = GetMicroCounter();
  return updated;
}

/// Process and respond to a ping message
void ICACHE_FLASH_ATTR ReceivePing(uint8_t* msg, uint16_t msgLen, ReliableConnection* connection)
{
  // Just echo the ping back for now
  ReliableTransport_SendMessage(msg, msgLen, connection, eRMT_Ping, false, false);
}

/// Process one message
void ICACHE_FLASH_ATTR HandleSubMessage(uint8_t* msg, uint16_t msgLen, EReliableMessageType msgType,
                      ReliableSequenceId seqId, ReliableConnection* connection)
{
  if (!IsMessageTypeAlwaysSentUnreliably(msgType)) // If is reliable message type, handle order and duplication
  {
    if (connection->nextInSequenceId == seqId) // This is the reliable message we were expecting
    {
      connection->nextInSequenceId = NextSequenceId(connection->nextInSequenceId);
    }
    else // A duplicate (or out of order) reliable message
    {
      //printf("INFO: Duplicate / out of order message seqID %d (expecting %d) type %d\r\n", seqId, connection->nextInSequenceId, msgType);
      return;
    }
  }

  switch (msgType)
  {
    case eRMT_ConnectionRequest:
    {
      Receiver_OnConnectionRequest(connection);
      break;
    }
    case eRMT_ConnectionResponse:
    {
      Receiver_OnConnected(connection);
      break;
    }
    case eRMT_DisconnectRequest:
    {
      Receiver_OnDisconnect(connection);
      break;
    }
    case eRMT_SingleReliableMessage:
    case eRMT_SingleUnreliableMessage:
    {
      Receiver_ReceiveData(msg, msgLen, connection);
      break;
    }
    case eRMT_MultiPartMessage:
    {
      printf("ERROR: Embedded reliable transport doesn't implement Multi part messages\r\n");
      break;
    }
    case eRMT_MultipleReliableMessages:
    case eRMT_MultipleUnreliableMessages:
    case eRMT_MultipleMixedMessages:
    {
      printf("ERROR: MultipleMessages should have been handled before here\r\n");
      break;
    }
    case eRMT_ACK:
    {
      // ACK is already handled before here
      break;
    }
    case eRMT_Ping:
    {
      ReceivePing(msg, msgLen, connection);
      break;
    }
    default:
    {
      printf("ERROR: ReliableTransport unhandled message type %d\r\n", msgType);
    }
  }
}

bool ReliableTransport_SendMessage(const uint8_t* buffer, const uint16_t bufferSize, ReliableConnection* connection, const EReliableMessageType messageType, const bool hot, const uint8_t tag)
{
  if (bufferSize > ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE)
  {
    //printf("ERROR: ReliableTransport send message, %d is above MTU %d\r\n", bufferSize, (int)ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE);
    return false;
  }
  else if (!IsMessageTypeAlwaysSentUnreliably(messageType)) // Is reliable
  {
    if (QueueMessage(buffer, bufferSize, connection, messageType, tag) == false)
    {
      //printf("ERROR: Couldn't queue reliable message %d[%d]\r\n", buffer[0], bufferSize);
      return false;
    }
    else
    {
      if (hot)
      {
        return SendPendingMessages(connection, true);
      }
      else
      {
        return true;
      }
    }
  }
  else // Unreliable message
  {
    uint16_t msgSubHeaderSizeField = bufferSize + 1*(tag != GLOBAL_INVALID_TAG);
    if ((bufferSize + ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH + 1) > // + 1 for tag byte. Easier to assume it's there than check
        (ReliableTransport_MAX_TOTAL_BYTES_PER_MESSAGE - connection->txQueued))
    { // No room in the current packet
      // So finish this packet and send it out
      AppendPendingMessages(connection, false);
      if (haveDataToSend(connection))
      {
        if (false == SendTxBuf(connection))
        {
          return false;
        }
      }
    }
    // Add this message to the queue
    // Add submessage header
    connection->txBuf[connection->txQueued] = messageType;
    connection->txQueued += 1;
    // Need to do two writes because unaligned write is not supported
    connection->txBuf[connection->txQueued] = msgSubHeaderSizeField & 0xff;
    connection->txQueued += 1;
    connection->txBuf[connection->txQueued] = msgSubHeaderSizeField >> 8;
    connection->txQueued += 1;

    // Add tag if specified
    if (tag != GLOBAL_INVALID_TAG)
    {
      connection->txBuf[connection->txQueued] = tag;
      connection->txQueued += 1;
    }
    memcpy(connection->txBuf + connection->txQueued, buffer, bufferSize);
    connection->txQueued += bufferSize;
    // If this needs to go out right away
    if (hot)
    {
      AppendPendingMessages(connection, true); // Append any pending reliable traffic that fits
      return SendTxBuf(connection);
    }
    return true;
  }
}

int16_t ICACHE_FLASH_ATTR ReliableTransport_ReceiveData(ReliableConnection* connection, uint8_t* buffer, uint16_t bufferSize) {
  if (bufferSize < sizeof(AnkiReliablePacketHeader))
  {
    //printf("WARN: RealiableTransport RX data too small (%d) for header\n", bufferSize);
    return -1;
  }
  else if (strncmp((char*)buffer, RELIABLE_PACKET_HEADER_PREFIX, RELIABLE_PACKET_HEADER_PREFIX_LENGTH) != 0)
  {
    //printf("WARN: ReliableTransport RX header didn't match %d[%d]\r\n", buffer[0], bufferSize);
    return -2;
  }
  else
  {
    AnkiReliablePacketHeader header;
    memcpy(&header, buffer, sizeof(AnkiReliablePacketHeader));
    uint8_t* msg = buffer + sizeof(AnkiReliablePacketHeader);
    uint16_t bytesProcessed = sizeof(AnkiReliablePacketHeader);

    //printf("Recv: siz=%d typ=%d seq=%d..%d ack=%d\r\n", connection->txQueued,
    //  header.type, header.seqIdMin, header.seqIdMax, header.lastReceivedId);

    // Unpack the message
    if (IsMutlipleMessagesType(header.type))
    { // Unpack submessages
      ReliableSequenceId seqId = header.seqIdMin;
      while (bytesProcessed < bufferSize)
      {
        uint16_t msgLen;
        EReliableMessageType msgType;
        msgType = (EReliableMessageType)(*msg); msg++;
        // Two byte read for alighment
        msgLen  = (*msg);      msg++;
        msgLen |= (*msg) << 8; msg++;
        if (msgLen > (bufferSize - bytesProcessed))
        {
          //printf("ERROR ReliableTransport not enough data for submessage %d < %d-%d\r\n", msgLen, bufferSize, bytesProcessed);
          return -3;
        }
        bytesProcessed += msgLen + ReliableTransport_MULTIPLE_MESSAGE_SUB_HEADER_LENGTH;
        HandleSubMessage(msg, msgLen, msgType, seqId, connection);
        msg += msgLen;
        if (!IsMessageTypeAlwaysSentUnreliably(msgType)) // Not an unreliable message type
        {
          seqId = NextSequenceId(seqId);
        }
      }
    }
    else // Single message
    {
      if (header.seqIdMax != header.seqIdMin)
      {
        //printf("ERROR: ReliableTransport seqIds don't match %d %d %d\r\n", header.seqIdMin, header.seqIdMax, header.type);
        return -4;
      }
      else
      {
        HandleSubMessage(buffer + sizeof(AnkiReliablePacketHeader), bufferSize - sizeof(AnkiReliablePacketHeader),
                         header.type, header.seqIdMin, connection);
      }
    }

    // Update the acks we've received
    UpdateLastAckedMessage(connection, header.lastReceivedId);
    // Acking messages is automatically taken care of

    return 0;
  }
}

bool ICACHE_FLASH_ATTR ReliableTransport_Update(ReliableConnection* connection)
{
  uint32_t currentTime = GetMicroCounter();
  if (currentTime > connection->latestRecvTime + ReliableConnection_CONNECTION_TIMEOUT)
  {
    return false;
  }
  else
  {
    printClear();
    if (currentTime > connection->latestUnackedMessageSentTime + ReliableConnection_UNACKED_MESSAGE_SEPARATION_TIME)
    {
      SendPendingMessages(connection, false);
    }
    return true;
  }
}
