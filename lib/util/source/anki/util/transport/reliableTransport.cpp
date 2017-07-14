/**
 * File: reliableTransport
 *
 * Author: Mark Wesley
 * Created: 02/01/15
 *
 * Description: Implements a reliability layer on top of an unreliable network communication protocol
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/transport/reliableTransport.h"
#include "util/console/consoleInterface.h"
#include "util/debug/messageDebugging.h"
#include "util/helpers/assertHelpers.h"
#include "util/logging/logging.h"
#include "util/transport/connectionStats.h"
#include "util/transport/iUnreliableTransport.h"
#include "util/transport/reliableConnection.h"
#include "util/transport/srcBufferSet.h"
#include "util/transport/transportAddress.h"


namespace Anki {
namespace Util {

  
CONSOLE_VAR(bool,      kPrintNetworkStats,              "Network", false);
CONSOLE_VAR_RANGED(uint32_t,  kPrintNetworkStatsTimeSpacingMS, "Network", 1000, 0, 10000);


// ============================== Packet Headers ========================================


// Packet headers for ensuring that the messages are of the correct type. (Used for unreliable messages sent via this transport too)
static const uint8_t k_AnkiReliablePacketHeaderPrefix[3] = {'R', 'E', 01}; // i.e. "Reliable Transport Layer 1"
  
struct AnkiReliablePacketHeader
{
  explicit AnkiReliablePacketHeader(uint8_t messageType, ReliableSequenceId seqIdMin, ReliableSequenceId seqIdMax, ReliableSequenceId lastReceivedId)
  {
    Set(messageType, seqIdMin, seqIdMax, lastReceivedId);
  }
  
  void Set(uint8_t messageType, ReliableSequenceId seqIdMin, ReliableSequenceId seqIdMax, ReliableSequenceId lastReceivedId)
  {
    for (uint32_t i = 0; i < sizeof(k_AnkiReliablePacketHeaderPrefix); ++i)
    {
      _prefix[i] = k_AnkiReliablePacketHeaderPrefix[i];
    }
    _type              = messageType;
    _seqIdMin          = seqIdMin;
    _seqIdMax          = seqIdMax;
    _seqIdLastReceived = lastReceivedId;
  }
  
  bool HasCorrectPrefix() const
  {
    for (uint32_t i = 0; i < sizeof(k_AnkiReliablePacketHeaderPrefix); ++i)
    {
      if (_prefix[i] != k_AnkiReliablePacketHeaderPrefix[i])
      {
        return false;
      }
    }
    return true;
  }
  
  ReliableSequenceId GetSequenceIdMin() const
  {
    assert(IsReliable());
    return _seqIdMin;
  }
  
  ReliableSequenceId GetSequenceIdMax() const
  {
    assert(IsReliable());
    return _seqIdMax;
  }
  
  ReliableSequenceId GetSequenceIdLastReceived() const
  {
    return _seqIdLastReceived;
  }
  
  uint8_t GetType() const
  {
    return _type;
  }
  
  bool IsReliable() const
  {
    return ((_seqIdMin != k_InvalidReliableSeqId) || (_seqIdMax != k_InvalidReliableSeqId));
  }
  
  uint8_t             _prefix[sizeof(k_AnkiReliablePacketHeaderPrefix)]; // 3:  0+3 = 3
  uint8_t             _type;                    // 1:  3+1 = 4
  ReliableSequenceId  _seqIdMin;                // 2:  4+2 = 6
  ReliableSequenceId  _seqIdMax;                // 2:  6+2 = 8
  ReliableSequenceId  _seqIdLastReceived;       // 2:  8+2 = 10
};
static_assert(sizeof(AnkiReliablePacketHeader) == 10, "Expected size mismatch, check layout and padding" );


#define RELIABLE_HEADER_DESCRIPTOR_TEXT   "3cPrefix|Ty|SqMin|SqMax|SqRec|"
static const char* k_ReliableHeaderDescriptor = SIZED_SRC_BUFFER_DESCRIPTOR( RELIABLE_HEADER_DESCRIPTOR_TEXT );

  
// ============================== ReliableTransport ========================================

  
bool ReliableTransport::sSendAckOnReceipt = true;
bool ReliableTransport::sSendUnreliableMessagesImmediately = true;
bool ReliableTransport::sTrackAckLatency = false;
uint32_t ReliableTransport::sMaxPacketsToReSendOnAck = 1;
uint32_t ReliableTransport::sMaxPacketsToSendOnSendMessage = 1;
  
  
ReliableTransport::ReliableTransport(IUnreliableTransport* unreliableTransport, INetTransportDataReceiver* dataReceiver)
  : INetTransport(dataReceiver)
  , _unreliable(unreliableTransport)
  , _queue(Dispatch::create_queue, "RelTransport", ThreadPriority::High)
  , _transportStats("Reliable")
#if ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  , _timesBetweenUpdates(1000)
  , _lastUpdateTime(kNetTimeStampZero)
  , _lastReportOfSlowUpdate(kNetTimeStampZero)
#endif // ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  , _runSynchronous(true)
{
  ChangeSyncMode(false);
  _unreliable->SetDataReceiver(this);
}
  

ReliableTransport::~ReliableTransport()
{
  KillThread();

  ClearConnections();
}
  
  
void ReliableTransport::KillThread()
{
  _queue.reset();
}

  
uint32_t ReliableTransport::MaxTotalBytesPerMessage() const
{
  return _unreliable->MaxTotalBytesPerMessage() - sizeof(AnkiReliablePacketHeader);
}
  
  
void ReliableTransport::ReSendReliableMessage(ReliableConnection* connectionInfo, const uint8_t* buffer, unsigned int bufferSize, EReliableMessageType messageType,
                                              ReliableSequenceId seqIdMin, ReliableSequenceId seqIdMax)
{
  uint8_t  headerBuffer[ sizeof(AnkiReliablePacketHeader) ];
  const uint32_t headerSize = BuildHeader(headerBuffer, sizeof(headerBuffer), messageType, seqIdMin, seqIdMax, connectionInfo->GetLastInAckedMessageId());
  
  SrcBufferSet srcBuffers;
  srcBuffers.AddBuffer( SizedSrcBuffer(headerBuffer, headerSize, k_ReliableHeaderDescriptor) );
  srcBuffers.AddBuffer( SizedSrcBuffer(buffer, bufferSize, SIZED_SRC_BUFFER_DESCRIPTOR("Message...")) );
  
  connectionInfo->NotifyAckingMessageSent();
  
  _unreliable->SendData(connectionInfo->GetAddress(), srcBuffers);
}

  
void ReliableTransport::QueueMessage(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer, unsigned int bufferSize, EReliableMessageType messageType, bool flushPacket)
{
  if(_runSynchronous)
  {
    SendMessage(reliable, destAddress, buffer, bufferSize, messageType, flushPacket);
  }
  else
  {
    uint8_t* newBuffer = new uint8_t[bufferSize];
    memcpy(newBuffer, buffer, bufferSize);
    const NetTimeStamp queuedTime = GetCurrentNetTimeStamp();
    
    QueueAction([this, reliable, destAddress, newBuffer, bufferSize, messageType, flushPacket, queuedTime] {
      SendMessage(reliable, destAddress, newBuffer, bufferSize, messageType, flushPacket, queuedTime);
      delete[] newBuffer;
    });
  }
}


void ReliableTransport::QueueAction(std::function<void ()> action)
{
  Dispatch::Async(_queue.get(), [this, action = std::move(action)] {
    std::lock_guard<std::mutex> lock(_mutex);
    action();
  });
}


void ReliableTransport::SendMessage(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer,
                                    unsigned int bufferSize, EReliableMessageType messageType, bool flushPacket, NetTimeStamp queuedTime)
{
  assert((buffer != nullptr) || (bufferSize == 0));
  
  uint8_t  headerBuffer[ sizeof(AnkiReliablePacketHeader) ];

  // Unless this is a connection request, we should not send messages to people
  // we aren't already connected to
  const bool isConnectRequest = messageType == eRMT_ConnectionRequest;
  ReliableConnection* connectionInfo = FindConnection(destAddress, isConnectRequest);
  if (nullptr == connectionInfo)
  {
    PRINT_NAMED_WARNING("ReliableTransport.SendMessage.NullConnection", "Trying to send a normal message to an unconnected destination! %s", destAddress.ToString().c_str());
    return;
  }
  
  // See if we need to split the message into multiple smaller pieces and reassemble on the other end
 
  const uint32_t maxPayloadPerMessage = MaxTotalBytesPerMessage();
  const uint32_t maxPayloadPerMultiPartMessage = maxPayloadPerMessage - k_MultiPartMessageHeaderSize; // for multi part tracking
  
  const bool isMultiPartMessage = (bufferSize > maxPayloadPerMessage);
  uint32_t bufferSizeSent = 0;
  
  if (isMultiPartMessage && !reliable)
  {
    PRINT_NAMED_WARNING("ReliableTransport.SendMessage.UnreliableMultiPart", "Had to split %u byte unreliable message (> %u bytes) - forcing it to be reliable so it can be reassembled!", bufferSize, maxPayloadPerMessage);
    reliable = true;
  }
  
  if (isMultiPartMessage)
  {
    assert((messageType == eRMT_SingleReliableMessage) || (messageType == eRMT_SingleUnreliableMessage) || (messageType == eRMT_MultiPartMessage));
    messageType = eRMT_MultiPartMessage;
  }
  
  if (reliable)
  {
    _transportStats.AddSentMessage(bufferSize);
  }
  
  uint32_t multiMessageIndex = 1;
  const uint32_t numMessagesToSend = !isMultiPartMessage ? 1 : ((bufferSize + (maxPayloadPerMultiPartMessage - 1)) / maxPayloadPerMultiPartMessage);
  assert(numMessagesToSend > 0);
  
  const bool unreliableAndSendImmediately = !reliable && sSendUnreliableMessagesImmediately;
  
  do
  {
    const uint8_t* messageBuffer = (buffer != nullptr) ? &buffer[bufferSizeSent] : nullptr;
    uint32_t messageBufferSize = bufferSize - bufferSizeSent;
    if (isMultiPartMessage && (messageBufferSize > maxPayloadPerMultiPartMessage))
    {
      messageBufferSize = maxPayloadPerMultiPartMessage;
    }
    
    const ReliableSequenceId seqId = reliable ? connectionInfo->GetNextOutSequenceNumber() : k_InvalidReliableSeqId;
    
    SrcBufferSet srcBuffers;
    if (unreliableAndSendImmediately)
    {
      // only need header if we're sending immediately (i.e. unreliably)
      uint32_t headerSize = BuildHeader(headerBuffer, sizeof(headerBuffer), messageType, seqId, seqId, connectionInfo->GetLastInAckedMessageId());
      srcBuffers.AddBuffer( SizedSrcBuffer(headerBuffer, headerSize, k_ReliableHeaderDescriptor) );
    }
    
    uint8_t multiMessageHeader[k_MultiPartMessageHeaderSize];
    if (isMultiPartMessage)
    {
      multiMessageHeader[0] = multiMessageIndex;
      multiMessageHeader[1] = numMessagesToSend;
      srcBuffers.AddBuffer( SizedSrcBuffer(multiMessageHeader, sizeof(multiMessageHeader), SIZED_SRC_BUFFER_DESCRIPTOR("Mu|Mu|")) );
    }
    
    if (messageBufferSize > 0)
    {
      srcBuffers.AddBuffer( SizedSrcBuffer(messageBuffer, messageBufferSize, SIZED_SRC_BUFFER_DESCRIPTOR("Message...")) );
    }
    
    if (unreliableAndSendImmediately)
    {
      // just send it now
      connectionInfo->NotifyAckingMessageSent();
      _unreliable->SendData(destAddress, srcBuffers);
    }
    else
    {
      // Store message (incase it needs to re-send it later) (without header, but with any multi part info)
      connectionInfo->AddMessage(srcBuffers, messageType, seqId, flushPacket, queuedTime);
    }
    
    bufferSizeSent += messageBufferSize;
    ++multiMessageIndex;
  } while (bufferSizeSent < bufferSize);
  
  assert(multiMessageIndex == (numMessagesToSend + 1));
  
  if (!unreliableAndSendImmediately && (sMaxPacketsToSendOnSendMessage > 0))
  {
    // we queued up some messages - give connection a chance to send some of them
    connectionInfo->SendOptimalUnAckedPackets(this, sMaxPacketsToSendOnSendMessage);
  }
}


void ReliableTransport::SendData(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer, uint32_t bufferSize, bool flushPacket)
{
  const EReliableMessageType messageType = reliable ? eRMT_SingleReliableMessage : eRMT_SingleUnreliableMessage;
  QueueMessage(reliable, destAddress, buffer, bufferSize, messageType, flushPacket);
}
  

void ReliableTransport::Connect(const TransportAddress& destAddress)
{
  QueueMessage(true, destAddress, nullptr, 0, eRMT_ConnectionRequest, true);
}
  

void ReliableTransport::FinishConnection(const TransportAddress& destAddress)
{
  QueueMessage(true, destAddress, nullptr, 0, eRMT_ConnectionResponse, true);
}

  
void ReliableTransport::Disconnect(const TransportAddress& destAddress)
{
  QueueAction([this, destAddress]
  {
    SendMessage(true, destAddress, nullptr, 0, eRMT_DisconnectRequest, true);
    DeleteConnection(destAddress);
  });
}
  

void ReliableTransport::StartClient()
{
  QueueAction([this] { _unreliable->StartClient(); });
}


void ReliableTransport::StopClientInternal()
{
  _unreliable->StopClient();
  ClearConnections();
}
  
  
void ReliableTransport::StopClient()
{
  // queue this so that it doesn't pre-empt things we still have left to send
  QueueAction( [this] { StopClientInternal(); });
}

  
void ReliableTransport::StartHost()
{
  QueueAction( [this] { _unreliable->StartHost(); });
}


void ReliableTransport::StopHostInternal()
{
  _unreliable->StopHost();
  ClearConnections();
}
  
  
void ReliableTransport::StopHost()
{
  // queue this so that it doesn't pre-empt things we still have left to send
  QueueAction( [this] { StopHostInternal(); });
}

  
void ReliableTransport::FillAdvertisementBytes(AdvertisementBytes& bytes)
{
  _unreliable->FillAdvertisementBytes(bytes);
}

  
unsigned int ReliableTransport::FillAddressFromAdvertisement(TransportAddress& address, const uint8_t *buffer, unsigned int size)
{
  return _unreliable->FillAddressFromAdvertisement(address, buffer, size);
}


void ReliableTransport::HandleSubMessage(const uint8_t* innerMessage, uint32_t innerMessageSize, uint8_t messageType, ReliableSequenceId reliableSequenceId, ReliableConnection* connectionInfo, const TransportAddress& sourceAddress)
{
  const bool isReliable = (reliableSequenceId != k_InvalidReliableSeqId);
  const bool isNextReliable = isReliable && connectionInfo->IsNextInSequenceId(reliableSequenceId);
  const bool ignoreMessage = isReliable && !isNextReliable; // message is a repeat or out of order

  connectionInfo->AddRecvMessageStats(innerMessageSize, isReliable, !ignoreMessage);
  
  if (isNextReliable)
  {
    connectionInfo->AdvanceNextInSequenceId();
  }
  else if (ignoreMessage)
  {
    return;
  }
  
  switch (messageType)
  {
    case eRMT_ConnectionRequest:
      if (_dataReceiver)
      {
        // let the net interface know someone wants to connect to us
        _dataReceiver->ReceiveData(INetTransportDataReceiver::OnConnectRequest, 0, sourceAddress);
      }
      break;
    case eRMT_ConnectionResponse:
      if (_dataReceiver)
      {
        // they acknowledged, we're connected
        _dataReceiver->ReceiveData(INetTransportDataReceiver::OnConnected, 0, sourceAddress);
      }
      break;
    case eRMT_DisconnectRequest:
      if (_dataReceiver)
      {
        _dataReceiver->ReceiveData(INetTransportDataReceiver::OnDisconnected, 0, sourceAddress);
      }
      DeleteConnection(sourceAddress);
      break;
    case eRMT_SingleReliableMessage:
    case eRMT_SingleUnreliableMessage:
      if (_dataReceiver)
      {
        _dataReceiver->ReceiveData(innerMessage, innerMessageSize, sourceAddress);
      }
      break;
    case eRMT_MultiPartMessage:
      {
        PendingMultiPartMessage& pendingMultiPart = connectionInfo->GetPendingMultiPartMessage();
        if (pendingMultiPart.AddMessagePart(innerMessage, innerMessageSize))
        {
          // Pass the completed re-assembled message on, and remove it.
          
          ANKI_NET_MESSAGE_VERBOSE(("Multi Complete", "Message...", pendingMultiPart.GetBytes(), pendingMultiPart.GetSize()));
          
          if (_dataReceiver)
          {
            _dataReceiver->ReceiveData(pendingMultiPart.GetBytes(), pendingMultiPart.GetSize(), sourceAddress);
          }
          
          pendingMultiPart.Clear();
        }
      }
      break;
    case eRMT_MultipleReliableMessages:
    case eRMT_MultipleUnreliableMessages:
    case eRMT_MultipleMixedMessages:
      assert(0); // should be handled purely at ReceiveData, and never embedded as a sub-message
      break;
    case eRMT_ACK:
      // Ack id is already handled for all messages from the header earlier - nothing left to do!
      break;
    case eRMT_Ping:
      connectionInfo->ReceivePing(this, innerMessage, innerMessageSize);
      break;
    default:
      PRINT_NAMED_ERROR("ReliableTransport.Sub.BadType", "unknown reliable message type: %u", messageType);
      break;
  }
}


void ReliableTransport::ReceiveData(const uint8_t* buffer, unsigned int size, const TransportAddress& sourceAddress)
{
  bool handledMessageType = false;
  
  // see if this is a message for our layer specifically
  if (size >= sizeof(AnkiReliablePacketHeader))
  {
    const AnkiReliablePacketHeader* reliablePacketHeader = reinterpret_cast<const AnkiReliablePacketHeader*>(buffer);
    
    if (reliablePacketHeader->HasCorrectPrefix())
    {
      ANKI_NET_MESSAGE_VERBOSE(("Rel Recv", "|" RELIABLE_HEADER_DESCRIPTOR_TEXT "Message...", buffer, size));
      
      handledMessageType = true;
      
      const bool isReliable = reliablePacketHeader->IsReliable();
      const uint8_t messageType = reliablePacketHeader->GetType();
      const ReliableSequenceId minSeqId = isReliable ? reliablePacketHeader->GetSequenceIdMin() : k_InvalidReliableSeqId;
      const ReliableSequenceId maxSeqId = isReliable ? reliablePacketHeader->GetSequenceIdMax() : k_InvalidReliableSeqId;

      uint32_t innerMessageSize = size - sizeof(AnkiReliablePacketHeader);
      const uint8_t* innerMessage = (innerMessageSize > 0) ? &buffer[sizeof(AnkiReliablePacketHeader)] : nullptr;

      const bool isMultipleMessages = IsMutlipleMessagesType((EReliableMessageType)messageType);
      
      bool isConnectRequest = messageType == eRMT_ConnectionRequest;
      if (!isConnectRequest)
      {
        // is it multiple messages where the first one is a connect request?
        isConnectRequest = isMultipleMessages && (innerMessage != nullptr) && (innerMessageSize > 0) && (*innerMessage == eRMT_ConnectionRequest);
      }

      // Only create a new connection if this is a connect request
      ReliableConnection* connectionInfo = FindConnection(sourceAddress, isConnectRequest);
      if (nullptr == connectionInfo)
      {
        PRINT_NAMED_WARNING("ReliableTransport.Recv.Unconnected", "Received %s message from unconnected source! %s", ReliableMessageTypeToString((EReliableMessageType)messageType), sourceAddress.ToString().c_str());
        return;
      }

      if (isReliable)
      {
        _transportStats.AddRecvMessage(size);
      }

      // tell reliable layer any messages the other end has received
      if (connectionInfo->UpdateLastAckedMessage( reliablePacketHeader->GetSequenceIdLastReceived() ))
      {
        // the list of reliable messages was updated - give connection a chance to send latest reliable messages now
        if (sMaxPacketsToReSendOnAck > 0)
        {
          connectionInfo->SendOptimalUnAckedPackets(this, sMaxPacketsToReSendOnAck);
        }
      }
      
      bool anyMessagesToReceive = true;
      
      if (isReliable)
      {
        anyMessagesToReceive = connectionInfo->IsWaitingForAnyInRange(minSeqId, maxSeqId);
        
        if (!anyMessagesToReceive)
        {
          // If it's multiple mixed messages we still need to process any unreliable messages within
          if (messageType == eRMT_MultipleMixedMessages)
          {
            anyMessagesToReceive = true;
          }
          else
          {
            _transportStats.AddRecvError(eME_OutOfOrder);
            ANKI_NET_PRINT_VERBOSE("ReliableTransport.Recv.OutOfOrder", "ReceiveData - Ignoring out of order messages %u..%u (waiting for %u)", minSeqId, maxSeqId, connectionInfo->GetNextInSequenceId() );
            
            #if !ENABLE_RC_PACKET_TIME_DIAGNOSTICS
            {
              // We only early-out if we're not interestd on the stats on each message
              return;
            }
            #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
          }
        }
        else
        {
          assert(reliablePacketHeader->GetType() != eRMT_ACK);
          connectionInfo->AckMessage(maxSeqId);
          if (sSendAckOnReceipt)
          {
            SendMessage(false, sourceAddress, nullptr, 0, eRMT_ACK, true);
          }
        }
      }
      
      connectionInfo->AddRecvPacketStats(size, anyMessagesToReceive);

      if (isMultipleMessages)
      {
        // Extract and handle each sub-message
        ReliableSequenceId reliableSequenceId = minSeqId;
        
        uint32_t bytesProcessed = 0;
        while (bytesProcessed < innerMessageSize)
        {
          uint8_t subMessageType = *reinterpret_cast<const uint8_t*>(&innerMessage[bytesProcessed]);
          bytesProcessed += sizeof(subMessageType);
          // If we get a sub message that appears to have an invalid type, this message is no longer valid. Toss it.
          if (!IsValidMessageType((EReliableMessageType)subMessageType))
          {
            PRINT_NAMED_ERROR("ReliableTransport.ReceiveData", "Invalid submessage type %u when processing multiple messages.", subMessageType);
            _transportStats.AddRecvError(eME_BadType);
            break;
          }
          uint16_t subMessageSize = *reinterpret_cast<const uint16_t*>(&innerMessage[bytesProcessed]);
          bytesProcessed += sizeof(subMessageSize);
          // If our sub message is too big, this message is no longer valid. Toss it.
          if (bytesProcessed + subMessageSize > innerMessageSize)
          {
            PRINT_NAMED_ERROR("ReliableTransport.ReceiveData", "Invalid submessage size %u when processing multiple messages.", subMessageSize);
            _transportStats.AddRecvError(eME_TooBig);
            break;
          }
          
          assert(nullptr != FindConnection(sourceAddress, false)); // could theoretically change in loop, but shouldn't in practice
          
          const bool isSubMessageReliable = isReliable && !IsMessageTypeAlwaysSentUnreliably((EReliableMessageType)subMessageType);
          const ReliableSequenceId subMessageSeqId = isSubMessageReliable ? reliableSequenceId : k_InvalidReliableSeqId;
          
          // Message
          
          const uint8_t* subMessageData = (subMessageSize == 0) ? nullptr : &innerMessage[bytesProcessed];
          bytesProcessed += subMessageSize;
          
          HandleSubMessage(subMessageData, subMessageSize, subMessageType, subMessageSeqId, connectionInfo, sourceAddress);
          
          if (isSubMessageReliable)
          {
            reliableSequenceId = NextSequenceId(reliableSequenceId);
          }
        }
        assert(bytesProcessed == innerMessageSize);
        assert(!isReliable || (reliableSequenceId == NextSequenceId(maxSeqId)));
      }
      else
      {
        assert(minSeqId == maxSeqId);
        HandleSubMessage(innerMessage, innerMessageSize, messageType, minSeqId, connectionInfo, sourceAddress);
      }
    }
    else
    {
      PRINT_NAMED_WARNING("ReliableTransport.BadPrefix", "Header '%s' has wrong prefix!",
                          ConvertMessageBufferToString(buffer, sizeof(AnkiReliablePacketHeader), Util::eBTTT_Ascii).c_str());
      _transportStats.AddRecvError(eME_WrongHeader);
    }
  }
  else
  {
    PRINT_NAMED_WARNING("ReliableTransport.BadPrefix.TooSmall", "Header '%s' is too small!",
                        ConvertMessageBufferToString(buffer, size, Util::eBTTT_Ascii).c_str());
    _transportStats.AddRecvError(eME_TooSmall);
  }

  if (!handledMessageType)
  {
    _transportStats.AddRecvError(eME_BadType);
    
    PRINT_NAMED_WARNING("ReliableTransport.Recv.Unhandled", "Unknown/Unhandled type of message!");
    
    ANKI_NET_MESSAGE_VERBOSE(("Rel Unhandled", "Message...", buffer, size));
    
    // Pass it on anyway?
    
    if (_dataReceiver != nullptr)
    {
      _dataReceiver->ReceiveData(buffer, size, sourceAddress);
    }
  }
}


uint32_t ReliableTransport::BuildHeader(uint8_t* outBuffer, uint32_t outBufferCapacity, uint8_t type, ReliableSequenceId seqIdMin, ReliableSequenceId seqIdMax, ReliableSequenceId lastReceivedId)
{
  ASSERT_AND_RETURN_VALUE_IF_FAIL(outBufferCapacity >= sizeof(AnkiReliablePacketHeader), 0);

  AnkiReliablePacketHeader* packetHeader = reinterpret_cast<AnkiReliablePacketHeader*>(outBuffer);
  packetHeader->Set(type, seqIdMin, seqIdMax, lastReceivedId);
  
  return sizeof(AnkiReliablePacketHeader);
}
  

#if ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
void DumpUpdateStats(const Stats::StatsAccumulator& timesBetweenUpdates)
{
  const double num = timesBetweenUpdates.GetNumDbl();

  if (num > 0.0)
  {
    const double avg = timesBetweenUpdates.GetMean();
    const double sd  = timesBetweenUpdates.GetStd();

    const double min = timesBetweenUpdates.GetMin();
    const double max = timesBetweenUpdates.GetMax();
    
    PRINT_CH_INFO("Network", "ReliableTransport.UpdateStats",
                         "UpdateTimes: %.1f samples, %.2f ms avg (%.2f sd) (%.2f..%.2f ms)", num, avg, sd, min, max);

  }
  else
  {
    PRINT_CH_INFO("Network", "ReliableTransport.UpdateStats", "UpdateTimes: NO SAMPLES!");
  }
}
#endif //ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  
  
void ReliableTransport::Update()
{
  // lock on mutex to make sure nothing sends at the same time we receive data or examine our connection data
  std::lock_guard<std::mutex> lock(_mutex);
  
  #if ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  const NetTimeStamp updateStartTimeMs = GetCurrentNetTimeStamp();
  {
    if (_lastUpdateTime != kNetTimeStampZero)
    {
      const double timeSinceLastUpdate = updateStartTimeMs - _lastUpdateTime;
      _timesBetweenUpdates.AddStat(timeSinceLastUpdate);
      
      // We expect ~2ms, but it's only a big issue if it seriously impacts our ability to read and ack packets in a timely manner
      if (timeSinceLastUpdate > 15.0 && !_reliableConnectionMap.empty())
      {
        const double timeSinceLastReport = updateStartTimeMs - _lastReportOfSlowUpdate;
        const double kMinTimeBetweenSlowSleepReports_ms = 60000.0;
        if (timeSinceLastReport > kMinTimeBetweenSlowSleepReports_ms) {
          Anki::Util::sChanneledInfoF("Network", "reliable_transport.update.sleep.slow",
                                      {{DDATA,"2"}}, "%.2f", timeSinceLastUpdate);
          _lastReportOfSlowUpdate = updateStartTimeMs;
        }
      }
    }
    _lastUpdateTime = updateStartTimeMs;
  }
  #endif // ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  
  _unreliable->Update();
  
  for (ReliableConnectionMap::iterator it = _reliableConnectionMap.begin(); it != _reliableConnectionMap.end(); )
  {
    ReliableConnection* existingConnection = it->second;
    if (!existingConnection->Update(this))
    {
      // Connection has timed out!
      // TODO - allow game to turn this off in lobby to workaround bug where BLE scanning prevents all IP received on Android
      const bool disconnectOnTimeout = true;
      if (disconnectOnTimeout)
      {
        PRINT_CH_INFO("Network", "ReliableTransport.Update.ConnectionTimedOut", "Disconnecting TimedOut Connection to '%s'!", existingConnection->GetAddress().ToString().c_str());
        
        #if ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
        {
          DumpUpdateStats(_timesBetweenUpdates.GetPrimaryAccumulator());
        }
        #endif // ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
        
        _dataReceiver->ReceiveData(INetTransportDataReceiver::OnDisconnected, 0, existingConnection->GetAddress());
        
        delete existingConnection;
        existingConnection = nullptr;
        it = _reliableConnectionMap.erase(it);
      }
    }

    const bool deletedExistingConnection = (existingConnection == nullptr);
    if (!deletedExistingConnection)
    {
      ++it;
    }
  }
  
#if ANKI_NET_MESSAGE_LOGGING_ENABLED
  if (kPrintNetworkStats)
  {
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    static std::chrono::steady_clock::time_point sLastPrintTime = now;
    
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - sLastPrintTime).count() >= Util::kPrintNetworkStatsTimeSpacingMS)
    {
      sLastPrintTime = now;
      Print();
    }
  }
#endif // #if ANKI_NET_MESSAGE_LOGGING_ENABLED
  
  #if REMOTE_CONSOLE_ENABLED
  if (kNetConnStatsUpdate)
  {
    UpdateNetStats();
  }
  #endif
  
  
#if ENABLE_RUN_TIME_PROFILING
  {
    const NetTimeStamp currentTimePostUpdate = GetCurrentNetTimeStamp();
    if (_lastUpdateTime != kNetTimeStampZero)
    {
      const double timeSinceLastUpdate = currentTimePostUpdate - updateStartTimeMs;
      if (timeSinceLastUpdate > 2.0)
      {
        Anki::Util::sEventF("reliable_transport.update.run.slow", {{DDATA,"2"}}, "%.2f", timeSinceLastUpdate);
      }
    }
  }
#endif // ENABLE_RUN_TIME_PROFILING

}


void ReliableTransport::UpdateNetStats()
{
#if REMOTE_CONSOLE_ENABLED
  uint32_t numConnections = 0;
  float latencyAvg    = 0.0f;
  float latencySD     = 0.0f;
  float latencyMin    = 0.0;
  float latencyMax    = 0.0;
  float pingArrivedPC = 0.0f;
  float queuedAvg_ms  = 0.0f;
  float queuedMin_ms  = 0.0f;
  float queuedMax_ms  = 0.0f;
  float extQueuedAvg_ms  = 0.0f;
  float extQueuedMin_ms  = 0.0f;
  float extQueuedMax_ms  = 0.0f;
  
  uint32_t numPingStatsAdded   = 0;
  uint32_t numExtQueuedStatsAdded = 0;
  uint32_t numQueuedStatsAdded = 0;
  
  for (ReliableConnectionMap::const_iterator it = _reliableConnectionMap.begin(); it != _reliableConnectionMap.end(); ++it)
  {
    ++numConnections;
    const ReliableConnection* existingConnection = it->second;
    
    {
      const Stats::StatsAccumulator& pingRoundTripStats = sTrackAckLatency ? existingConnection->GetAckRoundTripStats()
                                                                           : existingConnection->GetPingRoundTripStats();
      if (pingRoundTripStats.GetNum() > 0)
      {
        latencyAvg += pingRoundTripStats.GetMean();
        latencySD  += pingRoundTripStats.GetStd();
        latencyMin = (numPingStatsAdded == 0) ? pingRoundTripStats.GetMin() : Min(latencyMin, (float)pingRoundTripStats.GetMin());
        latencyMax = (numPingStatsAdded == 0) ? pingRoundTripStats.GetMax() : Max(latencyMax, (float)pingRoundTripStats.GetMax());
        pingArrivedPC += existingConnection->GetPingArrivedPercentage();
        ++numPingStatsAdded;
      }
    }
    
    {
      const Stats::StatsAccumulator& extQueuedTimeStats = existingConnection->GetExternalQueuedTimeStats();
      if (extQueuedTimeStats.GetNum() > 0)
      {
        extQueuedAvg_ms += extQueuedTimeStats.GetMean();
        extQueuedMin_ms = (numExtQueuedStatsAdded == 0) ? extQueuedTimeStats.GetMin() : Min(extQueuedMin_ms, (float)extQueuedTimeStats.GetMin());
        extQueuedMax_ms = (numExtQueuedStatsAdded == 0) ? extQueuedTimeStats.GetMax() : Max(extQueuedMax_ms, (float)extQueuedTimeStats.GetMax());
        ++numExtQueuedStatsAdded;
      }
    }
    
    {
      const Stats::StatsAccumulator& queuedTimeStats = existingConnection->GetQueuedTimeStats();
      if (queuedTimeStats.GetNum() > 0)
      {
        queuedAvg_ms += queuedTimeStats.GetMean();
        queuedMin_ms = (numQueuedStatsAdded == 0) ? queuedTimeStats.GetMin() : Min(queuedMin_ms, (float)queuedTimeStats.GetMin());
        queuedMax_ms = (numQueuedStatsAdded == 0) ? queuedTimeStats.GetMax() : Max(queuedMax_ms, (float)queuedTimeStats.GetMax());
        ++numQueuedStatsAdded;
      }
    }
  }
  
  // averaging the stats isn't ideal (especially SD...), but better than just showing one value
  if (numPingStatsAdded > 1)
  {
    const float statMult = 1.0f / float(numPingStatsAdded);
    latencyAvg    *= statMult;
    latencySD     *= statMult;
    pingArrivedPC *= statMult;
  }
  
  if (numExtQueuedStatsAdded > 1)
  {
    const float statMult = 1.0f / float(numExtQueuedStatsAdded);
    extQueuedAvg_ms *= statMult;
  }

  if (numQueuedStatsAdded > 1)
  {
    const float statMult = 1.0f / float(numQueuedStatsAdded);
    queuedAvg_ms *= statMult;
  }
  
  gNetStat1NumConnections  = numConnections;
  gNetStat2LatencyAvg      = latencyAvg;
  gNetStat3LatencySD       = latencySD;
  gNetStat4LatencyMin      = latencyMin;
  gNetStat5LatencyMax      = latencyMax;
  gNetStat6PingArrivedPC   = pingArrivedPC;
  gNetStat7ExtQueuedAvg_ms = extQueuedAvg_ms;
  gNetStat8ExtQueuedMin_ms = extQueuedMin_ms;
  gNetStat9ExtQueuedMax_ms = extQueuedMax_ms;
  gNetStatAQueuedAvg_ms    = queuedAvg_ms;
  gNetStatBQueuedMin_ms    = queuedMin_ms;
  gNetStatCQueuedMax_ms    = queuedMax_ms;
#endif // REMOTE_CONSOLE_ENABLED
}


void ReliableTransport::Print() const
{
#if ANKI_NET_MESSAGE_LOGGING_ENABLED
  PRINT_CH_INFO("Network", "ReliableTransport.Print", "NumConnections: %zu", _reliableConnectionMap.size());
  _transportStats.Print();
  _unreliable->Print();
  for (auto& it : _reliableConnectionMap)
  {
    const ReliableConnection* existingConnection = it.second;
    existingConnection->Print();
  }
#endif // ANKI_NET_MESSAGE_LOGGING_ENABLED
}


ReliableConnection* ReliableTransport::FindConnection(const TransportAddress& sourceAddress, bool createIfNew)
{
  ReliableConnectionMap::iterator it = _reliableConnectionMap.find(sourceAddress);
  if (it != _reliableConnectionMap.end())
  {
    ReliableConnection* existingConnection = it->second;
    return existingConnection;
  }
  
  if (createIfNew)
  {
    ReliableConnection* newConnection = new ReliableConnection(sourceAddress);
    _reliableConnectionMap[sourceAddress] = newConnection;
    return newConnection;
  }
  
  return nullptr;
}


void ReliableTransport::DeleteConnection(const TransportAddress &sourceAddress)
{
  auto it = _reliableConnectionMap.find(sourceAddress);
  if (it != _reliableConnectionMap.end())
  {
    delete it->second;
    _reliableConnectionMap.erase(it);
  }
}


void ReliableTransport::ClearConnections()
{
  for (auto& it : _reliableConnectionMap)
  {
    ReliableConnection* existingConnection = it.second;
    delete existingConnection;
  }
  
  _reliableConnectionMap.clear();
}

void ReliableTransport::ChangeSyncMode(bool isSync)
{
  if(!_runSynchronous && isSync)
  {
    _updateHandle.reset();
  }
  else if(_runSynchronous && !isSync)
  {
    _updateHandle = Dispatch::ScheduleCallback(_queue.get(), std::chrono::milliseconds(2), [this] { Update(); } );
  }
  _runSynchronous = isSync;
}


} // end namespace Util
} // end namespace Anki
