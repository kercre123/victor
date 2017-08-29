/**
 * File: reliableConnection
 *
 * Author: Mark Wesley
 * Created: 02/04/15
 *
 * Description: Manage the reliable messages for a single connection / address
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/transport/reliableConnection.h"
#include "util/console/consoleInterface.h"
#include "util/debug/messageDebugging.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/transport/pendingMessage.h"
#include "util/transport/reliableTransport.h"
#include <assert.h>


namespace Anki {
namespace Util {


CONSOLE_VAR_RANGED(uint32_t, kMaxPingTimesToTrackOverride, "Network", 0, 0, 1000);
  
// DEBUG CONSOLE FUNCTIONS
#if REMOTE_CONSOLE_ENABLED

static void SetConnectionTimeoutInMS( ConsoleFunctionContextRef context )
{
  const double newVal = ConsoleArg_Get_Double( context, "timeoutInMS");
  PRINT_NAMED_INFO("Network.SetConnectionTimeoutInMS", "ReliableConnection timeout was %f now %f", ReliableConnection::GetConnectionTimeoutInMS(), newVal);
  ReliableConnection::SetConnectionTimeoutInMS(newVal);
}
CONSOLE_FUNC( SetConnectionTimeoutInMS, "Network", double timeoutInMS );
  
#endif // REMOTE_CONSOLE_ENABLED

NetTimeStamp ReliableConnection::sTimeBetweenPingsInMS = 250.0;
NetTimeStamp ReliableConnection::sTimeBetweenResendsInMS = 50.0;
NetTimeStamp ReliableConnection::sMaxTimeSinceLastSend = sTimeBetweenResendsInMS - 1.0;
NetTimeStamp ReliableConnection::sConnectionTimeoutInMS = 5000.0;
NetTimeStamp ReliableConnection::sPacketSeparationIntervalInMS = kNetTimeStampZero;
NetTimeStamp ReliableConnection::sMinExpectedPacketAckTimeMS = 1.0;
uint32_t ReliableConnection::sMaxPingRoundTripsToTrack = 20; // Smaller number means more recent & responsive but jittery value
uint32_t ReliableConnection::sMaxAckRoundTripsToTrack = 100;
uint32_t ReliableConnection::sMaxPacketsToReSendOnUpdate = 3;
uint32_t ReliableConnection::sMaxBytesFreeInAFullPacket = 0;
bool     ReliableConnection::sSendSeparatePingMessages = true;
bool     ReliableConnection::sSendPacketsImmediately = true;

typedef uint8_t  MultiSubMessageType;
typedef uint16_t MultiSubMessageSize;

static const uint32_t k_ExtraBytesPerMultiSubMessage = sizeof(MultiSubMessageType) + sizeof(MultiSubMessageSize); // type and size in front of each sub-message


ReliableConnection::ReliableConnection(const TransportAddress& inTransportAddress)
  : _transportAddress( inTransportAddress )
  , _nextOutSequenceId( k_MinReliableSeqId )
  , _lastInAckedMessageId( k_InvalidReliableSeqId )
  , _nextInSequenceId( k_MinReliableSeqId )
  , _latestMessageSentTime(kNetTimeStampZero)
  , _latestRecvTime( GetCurrentNetTimeStamp() )
  , _latestPingSentTime( kNetTimeStampZero )
  , _numPingsSent(0)
  , _numPingsReceived(0)
  , _numPingsSentThatArrived(0)
  , _numPingsSentTowardsUs(0)
  , _externalQueuedTimes_ms(ReliableConnection::sMaxAckRoundTripsToTrack) // Track same number as Ack as they're comparable stats
  , _queuedTimes_ms(ReliableConnection::sMaxAckRoundTripsToTrack)         // Track same number as Ack as they're comparable stats
  , _pingRoundTripTimes(ReliableConnection::sMaxPingRoundTripsToTrack)
  , _ackRoundTripTimes(ReliableConnection::sMaxAckRoundTripsToTrack)
#if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  , _timesBetweenIncomingPackets(200)
  , _timesBetweenNewIncomingPackets(200)
  , _timesForAckingANewPacket(200)
  , _lastNewIncomingPacketTime(kNetTimeStampZero)
  , _unackedNewIncomingPacketTime(kNetTimeStampZero)
  , _sentStatsEver(UINT32_MAX)
  , _sentStatsRecent(200)
  , _recvStatsEver(UINT32_MAX)
  , _recvStatsRecent(200)
#endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS

{
  ANKI_NET_PRINT_VERBOSE("ReliableConnection.Constructor", "%p '%s' Constructed _nextOutSequenceId = %u)", this, _transportAddress.ToString().c_str(), _nextOutSequenceId);
}


ReliableConnection::~ReliableConnection()
{
  ANKI_NET_PRINT_VERBOSE("ReliableConnection.Destructor", "%p '%s' Destructed _nextOutSequenceId = %u, %zu (%d..%d)", this, _transportAddress.ToString().c_str(), _nextOutSequenceId, _pendingMessageList.size(), (int)GetFirstUnackedOutId(), (int)GetLastUnackedOutId());

  DestroyPendingMessageList(_pendingMessageList);
}


void ReliableConnection::DestroyPendingMessageList(PendingMessageList& messageList)
{
  for (PendingMessage* message : messageList)
  {
    delete message;
  }
  messageList.clear();
}


void ReliableConnection::AddMessage(const SrcBufferSet& srcBuffers, EReliableMessageType messageType,
                                    ReliableSequenceId seqId, bool flushPacket, NetTimeStamp queuedTime)
{
  PendingMessage* newPendingMessage = new PendingMessage();
  newPendingMessage->Set(srcBuffers, messageType, seqId, flushPacket, queuedTime);

  #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  {
    const bool rel = newPendingMessage->IsReliable();
    _sentStatsEver.GetRecentStats(rel, true).AddStat( newPendingMessage->GetMessageSize() );
    _sentStatsRecent.GetRecentStats(rel, true).AddStat( newPendingMessage->GetMessageSize() );
  }
  #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS

  _pendingMessageList.push_back( newPendingMessage );
}


PendingMultiPartMessage& ReliableConnection::GetPendingMultiPartMessage()
{
  return _pendingMultiPartMessage;
}


ReliableSequenceId ReliableConnection::GetNextOutSequenceNumber()
{
  const ReliableSequenceId nextSequenceId = _nextOutSequenceId;

  _nextOutSequenceId = NextSequenceId(_nextOutSequenceId);

  assert((GetLastUnackedOutId() == k_InvalidReliableSeqId) || (GetLastUnackedOutId() == PreviousSequenceId(nextSequenceId)));

  ANKI_NET_PRINT_VERBOSE("ReliableConnection.GetNextOutSequenceNumber", "%p '%s' GetNextOutSequenceNumber ret = %u", this, _transportAddress.ToString().c_str(), nextSequenceId);

  assert(nextSequenceId != k_InvalidReliableSeqId);
  assert(nextSequenceId >= k_MinReliableSeqId);
  assert(nextSequenceId <= k_MaxReliableSeqId);

  assert( !IsSequenceIdInRange(nextSequenceId, GetFirstUnackedOutId(), GetLastUnackedOutId()) ); // re-using an ack that's still pending!

  return nextSequenceId;
}


bool ReliableConnection::IsWaitingForAnyInRange(ReliableSequenceId minSeqId, ReliableSequenceId maxSeqId) const
{
  return IsSequenceIdInRange(_nextInSequenceId, minSeqId, maxSeqId);
}


bool ReliableConnection::IsNextInSequenceId(ReliableSequenceId sequenceNum) const
{
  return (_nextInSequenceId == sequenceNum);
}


void ReliableConnection::AdvanceNextInSequenceId()
{
  _nextInSequenceId = NextSequenceId(_nextInSequenceId);
}


bool ReliableConnection::UpdateLastAckedMessage(ReliableSequenceId seqId)
{
  bool updatedLastAckedMessage = false;
  
  const NetTimeStamp currentNetTimeStamp = GetCurrentNetTimeStamp();

  if ((seqId != k_InvalidReliableSeqId) && (_pendingMessageList.size() > 0))
  {
    ANKI_NET_PRINT_VERBOSE("ReliableConnection.UpdateLastAckedMessage", "%p '%s' UpdateLastAckedMessage(%u) (%u..%u were unacked)", this, _transportAddress.ToString().c_str(), seqId, GetFirstUnackedOutId(), GetLastUnackedOutId());

    while (IsSequenceIdInRange(seqId, GetFirstUnackedOutId(), GetLastUnackedOutId()))
    {
      // 1st message must by definition be an already sent, reliable, message
      PendingMessage* readMessage = _pendingMessageList[0];
      assert(readMessage->IsReliable() && (readMessage->GetLastSentTime() > kNetTimeStampZero));
      
      // use time since the first time we sent this message, we don't know which send attempt is being acked
      const NetTimeStamp timeForMessageToBeAcked = currentNetTimeStamp - readMessage->GetFirstSentTime();
      _ackRoundTripTimes.AddStat(timeForMessageToBeAcked);
      
      delete readMessage;
      _pendingMessageList.erase( _pendingMessageList.begin() );
      updatedLastAckedMessage = true;
    }
  }
  
  #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  {
    const NetTimeStamp timeSinceLast = (currentNetTimeStamp - _latestRecvTime);
    
    // don't warn on MultiplePacketsPerTick for now - this is happening regularly and is spamming the log
    // so just ensure we track all occurences in the stats
    //if (timeSinceLast < 0.5)
    //{
    //  // Unexpected to see such a short time, is reasonable for 1st ever incoming packet as _latestRecvTime
    //  // is the connection creation time in that case
    //  if (_timesBetweenIncomingPackets.GetNumDbl() > 0.9)
    //  {
    //    PRINT_NAMED_WARNING("ReliableConnection.MultiplePacketsPerTick",
    //                        "Only %f ms since last new packet - multiple packets arriving on same tick?", timeSinceLast);
    //  }
    //}
    //else
    {
      _timesBetweenIncomingPackets.AddStat(timeSinceLast);
    }
    
    static NetTimeStamp sMaxTimeBetweenRecvMessages = 0.0;
    if (timeSinceLast > sMaxTimeBetweenRecvMessages)
    {
      PRINT_CH_INFO("Network", "ReliableConnection.LongestTimeout", "%p '%s' Longest Time between received messages  = %.2f ms (was %.2f)",
                           this, _transportAddress.ToString().c_str(), timeSinceLast, sMaxTimeBetweenRecvMessages);
      
      sMaxTimeBetweenRecvMessages = timeSinceLast;
    }
  }
  #endif // ANKI_NET_MESSAGE_LOGGING_ENABLED
  
  _latestRecvTime = currentNetTimeStamp;

  return updatedLastAckedMessage;
}


struct PingPayload
{
  PingPayload(const uint8_t* message, uint32_t messageSize)
  {
    const uint32_t bytesToCopy = (messageSize > GetPayloadSize()) ? GetPayloadSize() : messageSize;
    assert(bytesToCopy == GetPayloadSize());
    memcpy(this, message, bytesToCopy);
  }

  PingPayload(NetTimeStamp timeSent, uint32_t numPingsSent, uint32_t numPingsReceived, bool isReply)
  {
    Set(timeSent, numPingsSent, numPingsReceived, isReply);
  }

  void Set(NetTimeStamp timeSent, uint32_t numPingsSent, uint32_t numPingsReceived, bool isReply)
  {
    _timeSent         = timeSent;
    _numPingsSent     = numPingsSent;
    _numPingsReceived = numPingsReceived;
    _isReply          = isReply;
  }

  static constexpr uint32_t GetPayloadSize()
  {
    return sizeof(PingPayload) - sizeof(_padding);
  }

  NetTimeStamp                   _timeSent;          // 8  // Time of original send (not reply), RoundTripTime = (CurrentTime - _timeSent) when reply is received
  uint32_t                       _numPingsSent;      // 4
  uint32_t                       _numPingsReceived;  // 4
  bool                           _isReply;           // 1
  uint8_t                        _padding[7];        // 7 // wasted memory padding for alignment of double Timestep
};
static_assert(sizeof(PingPayload) == 24, "Expected size mismatch, check layout and padding" );
static_assert(PingPayload::GetPayloadSize() == 17, "Expected size mismatch, check layout and padding" );


void ReliableConnection::SendPing(ReliableTransport* reliableTransport, NetTimeStamp incomingPingTime, bool isReply)
{
  ++_numPingsSent;
  const NetTimeStamp currentTime = GetCurrentNetTimeStamp();
  const NetTimeStamp pingTime = isReply ? incomingPingTime : currentTime;

  PingPayload pingPayload(pingTime, _numPingsSent, _numPingsReceived, isReply);
  const uint8_t* pingMessage = reinterpret_cast<const uint8_t*>(&pingPayload);
  reliableTransport->SendMessage(false, _transportAddress, pingMessage, pingPayload.GetPayloadSize(), eRMT_Ping, true, GetCurrentNetTimeStamp());

  if (!isReply)
  {
    _latestPingSentTime = currentTime;
  }
}


void ReliableConnection::ReceivePing(ReliableTransport* reliableTransport, const uint8_t* message, uint32_t messageSize)
{
  assert(messageSize == PingPayload::GetPayloadSize());
  if (messageSize >= PingPayload::GetPayloadSize())
  {
    const PingPayload pingPayload(message, messageSize);

    const NetTimeStamp currentTime = GetCurrentNetTimeStamp();

    ++_numPingsReceived;

    // pings can arrive out of order so the information in this might be older than a ping we already read
    const bool isNewPing = (pingPayload._numPingsSent > _numPingsSentTowardsUs);
    if (isNewPing)
    {
      assert(pingPayload._numPingsReceived >= _numPingsSentThatArrived);
      _numPingsSentTowardsUs   = pingPayload._numPingsSent;
      _numPingsSentThatArrived = pingPayload._numPingsReceived;
    }

    if (pingPayload._isReply)
    {
      if (currentTime < pingPayload._timeSent)
      {
        PRINT_NAMED_WARNING("ReliableConnection.ReceivePing.FromFuture", "Ping has returned before it was sent!? (currentTime %f vs ping sent %f)", currentTime, pingPayload._timeSent);
      }

      // Allow ping round trip range to be live tuned
      if (kMaxPingTimesToTrackOverride > 0)
      {
        _pingRoundTripTimes.SetMaxValuesToTrack(kMaxPingTimesToTrackOverride);
      }
      else
      {
        _pingRoundTripTimes.SetMaxValuesToTrack(sMaxPingRoundTripsToTrack);
      }

      const NetTimeStamp roundTripTime = (currentTime - pingPayload._timeSent);
      _pingRoundTripTimes.AddStat(roundTripTime);
    }
    else
    {
      if (sSendSeparatePingMessages)
      {
        SendPing(reliableTransport, pingPayload._timeSent, true);
      }
    }
  }
}


void ReliableConnection::AckMessage(ReliableSequenceId seqId)
{
  #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  {
    const NetTimeStamp currentNetTimeStamp = GetCurrentNetTimeStamp();
    
    if (_lastNewIncomingPacketTime != kNetTimeStampZero)
    {
      const NetTimeStamp timeSinceLast = (currentNetTimeStamp - _lastNewIncomingPacketTime);
      
      // don't warn on MultipleNewPacketsPerTick for now - this is happening regularly and is spamming the log
      //if (timeSinceLast < 0.5)
      //{
      //  PRINT_NAMED_WARNING("ReliableConnection.MultipleNewPacketsPerTick", "Only %f ms since last new packet - multiple packets arriving on same tick?", timeSinceLast);
      //}
      //else
      {
        _timesBetweenNewIncomingPackets.AddStat(timeSinceLast);
      }
    }
    
    // Keep track of time between any incoming reliable message, and the time we send anything to ack that
    if (_unackedNewIncomingPacketTime == kNetTimeStampZero)
    {
      _unackedNewIncomingPacketTime = currentNetTimeStamp;
    }
    
    _lastNewIncomingPacketTime = currentNetTimeStamp;
  }
  #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  
  _lastInAckedMessageId = seqId;
}


void ReliableConnection::NotifyAckingMessageSent()
{
#if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  {
    if (_unackedNewIncomingPacketTime != kNetTimeStampZero)
    {
      // There was something to ack
      
      const NetTimeStamp currentNetTimeStamp = GetCurrentNetTimeStamp();
      const NetTimeStamp timeToAck = (currentNetTimeStamp - _unackedNewIncomingPacketTime);
      _timesForAckingANewPacket.AddStat(timeToAck);

      _unackedNewIncomingPacketTime = kNetTimeStampZero;
    }
  }
#endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
}
  
#if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
void ReliableConnection::LogStatsForSendingMessage(const PendingMessage* pendingMessage)
{
  const bool rel = pendingMessage->IsReliable();
  _sentStatsEver.GetRecentStats(rel, false).AddStat( pendingMessage->GetMessageSize() );
  _sentStatsRecent.GetRecentStats(rel, false).AddStat( pendingMessage->GetMessageSize() );
}
#endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS

size_t ReliableConnection::SendUnAckedMessages(ReliableTransport* reliableTransport, size_t firstToSend)
{
  const size_t pendingMessageListSize = _pendingMessageList.size();
  if (firstToSend >= pendingMessageListSize)
  {
    // no messages left
    return 0;
  }

  // Find how many messages from firstToSend..N we can fit in one message, along with the metadata for that message:

  const uint32_t maxPayloadPerMessage = reliableTransport->MaxTotalBytesPerMessage();

  const PendingMessage* firstPendingMessage = _pendingMessageList[firstToSend];

  size_t   numMessagesToSend = 1;
  uint32_t numBytesToSend = firstPendingMessage->GetMessageSize();
  
  bool sentReliableMessages   = firstPendingMessage->IsReliable();
  bool sentUnreliableMessages = !sentReliableMessages;

  ReliableSequenceId seqIdMin = firstPendingMessage->GetSequenceId();
  ReliableSequenceId seqIdMax = seqIdMin;

  numBytesToSend += k_ExtraBytesPerMultiSubMessage;
  for (size_t i=(firstToSend+1); i < pendingMessageListSize; ++i)
  {
    const PendingMessage* pendingMessage = _pendingMessageList[i];

    const uint32_t numBytesToSendIfAdded = numBytesToSend + k_ExtraBytesPerMultiSubMessage + pendingMessage->GetMessageSize();
    if (numBytesToSendIfAdded <= maxPayloadPerMessage)
    {
      // Add message
      ++numMessagesToSend;
      numBytesToSend = numBytesToSendIfAdded;
      if (pendingMessage->IsReliable())
      {
        const ReliableSequenceId relSeqId = pendingMessage->GetSequenceId();
        if (!sentReliableMessages)
        {
          seqIdMin = relSeqId;
          sentReliableMessages = true;
        }
        seqIdMax = relSeqId;
      }
      else
      {
        sentUnreliableMessages = true;
      }
    }
    else
    {
      // no more messages can fit (we can't skip one or they'd be out of order)
      break;
    }
  }
  
  // See if there's room to pack any earlier messages into the start of this packet too
  // (improves chance of messages making it over)
  
  size_t numEarlierMessagesToSend = 0;
  while (firstToSend > 0)
  {
    const PendingMessage* pendingMessage = _pendingMessageList[firstToSend - 1];
    
    const uint32_t numBytesToSendIfAdded = numBytesToSend + k_ExtraBytesPerMultiSubMessage + pendingMessage->GetMessageSize();
    if (numBytesToSendIfAdded <= maxPayloadPerMessage)
    {
      // Add message (moves first message one earlier)
      
      firstPendingMessage = pendingMessage;
      --firstToSend;
      ++numEarlierMessagesToSend;
      ++numMessagesToSend;
      numBytesToSend = numBytesToSendIfAdded;
      if (pendingMessage->IsReliable())
      {
        const ReliableSequenceId relSeqId = pendingMessage->GetSequenceId();
        if (!sentReliableMessages)
        {
          seqIdMax = relSeqId;
          sentReliableMessages = true;
        }
        seqIdMin = relSeqId;
      }
      else
      {
        sentUnreliableMessages = true;
      }
    }
    else
    {
      // no more messages can fit (we can't keep looking for an even earlier smaller message, as it would overcomplicate
      // the id for each message in send/recv of the multi-message packet, and it wouldn't likely help throughput)
      break;
    }
  }
  
  if (numMessagesToSend == 1)
  {
    // No sub-message header required for a single message
    numBytesToSend -= k_ExtraBytesPerMultiSubMessage;
  }

  assert(numBytesToSend <= maxPayloadPerMessage);

  ANKI_NET_PRINT_VERBOSE("ReliableConnection.SendUnAckedMessages", "%p '%s' SendUnAckedMessages - sending %zu messages (%zu..%zu of %zu), %u bytes, id %u..%u",
                          this, _transportAddress.ToString().c_str(), numMessagesToSend,
                          firstToSend, firstToSend + (numMessagesToSend-1),
                          _pendingMessageList.size(), numBytesToSend, seqIdMin, seqIdMax);
  
  if (numMessagesToSend == 1)
  {
    // Just send that message
    assert(numBytesToSend == firstPendingMessage->GetMessageSize());
    reliableTransport->ReSendReliableMessage(this, firstPendingMessage->GetMessageBytes(), numBytesToSend, firstPendingMessage->GetType(), seqIdMin, seqIdMax );
    
    #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
    {
      LogStatsForSendingMessage( firstPendingMessage );
    }
    #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  }
  else
  {
    // build multi-message buffer and send it

    uint8_t* buffer = new uint8_t[numBytesToSend];
    uint32_t bufferSize = 0;

    for (size_t i=firstToSend; i < (firstToSend + numMessagesToSend); ++i)
    {
      const PendingMessage* pendingMessage = _pendingMessageList[i];

      // Type
      const MultiSubMessageType messageType = Anki::Util::numeric_cast<MultiSubMessageType>( (int)pendingMessage->GetType() );
      memcpy(&buffer[bufferSize], &messageType, sizeof(messageType));
      bufferSize += sizeof(messageType);
      // Size
      const MultiSubMessageSize messageSize = Anki::Util::numeric_cast<MultiSubMessageSize>( pendingMessage->GetMessageSize() );
      memcpy(&buffer[bufferSize], &messageSize, sizeof(messageSize));
      bufferSize += sizeof(messageSize);
      // Message
      memcpy(&buffer[bufferSize], pendingMessage->GetMessageBytes(), messageSize);
      bufferSize += messageSize;
      
      #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
      {
        LogStatsForSendingMessage( pendingMessage );
      }
      #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
    }
    assert(bufferSize == numBytesToSend);

    EReliableMessageType messageType = (sentReliableMessages && sentUnreliableMessages) ? eRMT_MultipleMixedMessages :
                                                                   sentReliableMessages ? eRMT_MultipleReliableMessages
                                                                                        : eRMT_MultipleUnreliableMessages;

    reliableTransport->ReSendReliableMessage(this, buffer, bufferSize, messageType, seqIdMin, seqIdMax );

    delete[] buffer;
  }
  
  #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  {
    _sentStatsEver._bytesPerPacket.AddStat( numBytesToSend );
    _sentStatsRecent._bytesPerPacket.AddStat( numBytesToSend );
  }
  #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS

  const NetTimeStamp currentNetTimeStamp = GetCurrentNetTimeStamp();
  _latestMessageSentTime = currentNetTimeStamp;

  // Update last-sent time for any reliable messages, and delete and remove any unreliable messages
  // iterate and remove in reverse order to reduce amount of shuffling

  for (size_t i=firstToSend + numMessagesToSend; i > firstToSend; )
  {
    --i;
    PendingMessage* pendingMessage = _pendingMessageList[i];
    
    if (!pendingMessage->HasBeenSent())
    {
      const NetTimeStamp extTimeQueued_ms = pendingMessage->GetExternalQueuedDuration();
      _externalQueuedTimes_ms.AddStat(extTimeQueued_ms);
      const NetTimeStamp timeQueued_ms = currentNetTimeStamp - pendingMessage->GetQueuedTime();
      _queuedTimes_ms.AddStat(timeQueued_ms);
    }
    
    if (pendingMessage->IsReliable())
    {
      pendingMessage->UpdateLatestSentTime(currentNetTimeStamp);
    }
    else
    {
      delete pendingMessage;
      _pendingMessageList.erase(_pendingMessageList.begin() + i);
    }
  }
  
  // don't count any earlier messages that we secretly managed to pack in
  const size_t numRequestedMessagesSent = numMessagesToSend - numEarlierMessagesToSend;

  return numRequestedMessagesSent;
}
  
  
#if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
void DumpPacketTimeStats(const char* name, const Stats::RecentStatsAccumulator& recentStats)
{
  const Stats::StatsAccumulator& stats = recentStats.GetPrimaryAccumulator();
  const double num = stats.GetNumDbl();
  
  if (num > 0.0)
  {
    const double avg = stats.GetMean();
    const double sd  = stats.GetStd();

    const double min = stats.GetMin();
    const double max = stats.GetMax();
    
    PRINT_CH_INFO("Network", "ReliableConnection.PacketTimeStats",
                         "%s: %.1f samples, %.2f ms avg (%.2f sd) (%.2f..%.2f ms)",
                         name, num, avg, sd, min, max);
  }
  else
  {
    PRINT_CH_INFO("Network", "ReliableConnection.PacketTimeStats", "%s: NO SAMPLES!", name);
  }
}
  
void DumpMessageStats(const char* name, const char* typeName, const Stats::RecentStatsAccumulator& recentStats)
{
  const Stats::StatsAccumulator& stats = recentStats.GetPrimaryAccumulator();
  const double num = stats.GetNumDbl();
  
  if (num > 0.0)
  {
    const double kBytesToKB = (1.0 / 1024.0);
    const double totalKB = stats.GetVal() * kBytesToKB;
    const double avg = stats.GetMean();
    const double sd  = stats.GetStd();
    
    const double min = stats.GetMin();
    const double max = stats.GetMax();
    
    PRINT_CH_INFO("Network", "ReliableConnection.MessageStats",
                         "%s: %.1f %s, %.2f Byte avg (%.2f sd) (%.0f..%.0f Bytes) (%.1f KB total)",
                         name, num, typeName, avg, sd, min, max, totalKB);
  }
  else
  {
    PRINT_CH_INFO("Network", "ReliableConnection.MessageStats", "%s: NO SAMPLES!", name);
  }
}
#endif //ENABLE_RC_PACKET_TIME_DIAGNOSTICS


bool ReliableConnection::HasConnectionTimedOut() const
{
  const NetTimeStamp currentTime = GetCurrentNetTimeStamp();
  const bool hasTimedOut = (currentTime > (_latestRecvTime + sConnectionTimeoutInMS));
  
  if (hasTimedOut)
  {
    {
      const Stats::StatsAccumulator& ackRTT = _ackRoundTripTimes.GetPrimaryAccumulator();
      const bool hasRTT = (ackRTT.GetNum() > 0);
      const Stats::StatsAccumulator& extQueuedTimes = _externalQueuedTimes_ms.GetPrimaryAccumulator();
      const bool hasEQT = (extQueuedTimes.GetNum() > 0);
      const Stats::StatsAccumulator& queuedTimes = _queuedTimes_ms.GetPrimaryAccumulator();
      const bool hasQT = (queuedTimes.GetNum() > 0);
      PRINT_NAMED_WARNING("ReliableConnection.ConnectionTimedOut",
                          "%p '%s' Timedout after %.2f ms (> %.2f), now = %.2f (latency %.1f ms (%.1f..%.1f)) (extQueued %.1f ms (%.1f..%.1f)) (queued %.1f ms (%.1f..%.1f))",
                          this, _transportAddress.ToString().c_str(), currentTime - _latestRecvTime, sConnectionTimeoutInMS, currentTime,
                          ackRTT.GetMean(), hasRTT ? ackRTT.GetMin() : 0.0, hasRTT ? ackRTT.GetMax() : 0.0,
                          extQueuedTimes.GetMean(), hasEQT ? extQueuedTimes.GetMin() : 0.0, hasEQT ? extQueuedTimes.GetMax() : 0.0,
                          queuedTimes.GetMean(),    hasQT  ? queuedTimes.GetMin()    : 0.0, hasQT  ? queuedTimes.GetMax()    : 0.0 );
    }
    
    
    #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
    {
      DumpPacketTimeStats("Time Between Any Incoming Packets", _timesBetweenIncomingPackets);
      DumpPacketTimeStats("Time Between New Incoming Packets", _timesBetweenNewIncomingPackets);
      DumpPacketTimeStats("Time To Ack A New Packet",          _timesForAckingANewPacket);

      if (_lastNewIncomingPacketTime != kNetTimeStampZero)
      {
        PRINT_CH_INFO("Network", "ReliableConnection.PacketTimeStats",
                             "%.2fms since last new incoming packet (at %.2f)",
                             currentTime - _lastNewIncomingPacketTime, _lastNewIncomingPacketTime);
      }

      if (_unackedNewIncomingPacketTime != kNetTimeStampZero)
      {
        PRINT_CH_INFO("Network", "ReliableConnection.PacketTimeStats",
                             "Still not acked an input message since %.2fms ago (at %.2f)",
                             currentTime - _unackedNewIncomingPacketTime, _unackedNewIncomingPacketTime);
      }
      
      for (int i=0; i < 2; ++i)
      {
        const TransmissionStats& stats = (i==0) ? _sentStatsEver : _sentStatsRecent;

        PRINT_CH_INFO("Network", "ReliableConnection.SentStats", (i==0) ? "Sent Ever:" : "Sent Recent:");
        DumpMessageStats("Queued Reliable",   "messages", stats._reliable._bytesPerUniqueMessage);
        DumpMessageStats("Sent   Reliable",   "messages", stats._reliable._bytesPerRepeatableMessage);
        DumpMessageStats("Queued Unreliable", "messages", stats._unreliable._bytesPerUniqueMessage);
        DumpMessageStats("Sent   Unreliable", "messages", stats._unreliable._bytesPerRepeatableMessage);
        DumpMessageStats("Sent   Packets",    "packets",  stats._bytesPerPacket);
        assert(stats._bytesPerWastedPacket.GetNum() == 0); // we can't track if send was wasted or not? maybe if ping only?
      }
      
      for (int i=0; i < 2; ++i)
      {
        const TransmissionStats& stats = (i==0) ? _recvStatsEver : _recvStatsRecent;
        
        PRINT_CH_INFO("Network", "ReliableConnection.RecvStats", (i==0) ? "Recv Ever:" : "Recv Recent:");
        DumpMessageStats("Recv New  Reliable",   "messages", stats._reliable._bytesPerUniqueMessage);
        DumpMessageStats("Recv Dupe Reliable",   "messages", stats._reliable._bytesPerRepeatableMessage);
        DumpMessageStats("Recv Unreliable", "messages", stats._unreliable._bytesPerUniqueMessage);
        DumpMessageStats("Recv Useful Packets",    "packets",  stats._bytesPerPacket);
        DumpMessageStats("Recv Redundant Packets", "packets",  stats._bytesPerWastedPacket);
      }
    }
    #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  }

  return hasTimedOut;
}


uint32_t ReliableConnection::SendUnAckedPackets(ReliableTransport* reliableTransport, uint32_t maxPacketsToSend, size_t firstToSend)
{
  uint32_t numPacketsSent = 0;
  size_t numMessagesSentThisPacket = 0;
  do
  {
    numMessagesSentThisPacket = SendUnAckedMessages(reliableTransport, firstToSend);
    if (numMessagesSentThisPacket > 0)
    {
      ANKI_NET_PRINT_VERBOSE("ReliableConnection.SendUnAckedPackets", "%p '%s' SendUnackedPackets packet %u: %zu messages sent from msg %zu",
                              this, _transportAddress.ToString().c_str(), numPacketsSent, numMessagesSentThisPacket, firstToSend);

      firstToSend += numMessagesSentThisPacket;
      numPacketsSent++;
    }
  } while((numMessagesSentThisPacket > 0) && (numPacketsSent < maxPacketsToSend));

  return numPacketsSent;
}


bool ReliableConnection::IsPacketWorthSending(const ReliableTransport* reliableTransport, NetTimeStamp currentTime, size_t firstToSend) const
{
  if (sSendPacketsImmediately)
  {
    return true;
  }
  
  if ((_latestMessageSentTime > kNetTimeStampZero) && (currentTime > (_latestMessageSentTime + sMaxTimeSinceLastSend)))
  {
    // Nothing has been sent in ages, just send it
    return true;
  }
  
  const uint32_t minBytesForFullPacket = reliableTransport->MaxTotalBytesPerMessage() - sMaxBytesFreeInAFullPacket;
  
  uint32_t numBytesToSend = 0;
  const size_t pendingMessageListSize = _pendingMessageList.size();
  for (size_t i=firstToSend; i < pendingMessageListSize; ++i)
  {
    const PendingMessage* pendingMessage = _pendingMessageList[i];
    
    if (pendingMessage->ShouldFlushPacket())
    {
      return true;
    }
    
    const NetTimeStamp lastSentTime = pendingMessage->GetLastSentTime();
    
    if ((lastSentTime > kNetTimeStampZero) && (currentTime > (lastSentTime + sMaxTimeSinceLastSend)))
    {
      // This message has been sent once, but long enough ago to resend - just send it
      return true;
    }

    numBytesToSend += k_ExtraBytesPerMultiSubMessage + pendingMessage->GetMessageSize();
    if (numBytesToSend >= minBytesForFullPacket)
    {
      return true;
    }
  }

  return false;
}


uint32_t ReliableConnection::SendOptimalUnAckedPackets(ReliableTransport* reliableTransport, uint32_t maxPacketsToSend)
{
  const NetTimeStamp currentTime = GetCurrentNetTimeStamp();
  
  if (sPacketSeparationIntervalInMS > 0.0f)
  {
    // Don't send more frequently than the interval
    
    if ((_latestMessageSentTime != kNetTimeStampZero) && (currentTime < (_latestMessageSentTime + sPacketSeparationIntervalInMS)))
    {
      return 0;
    }
  }
  
  // Treat "never been sent" messages as if they'd been sent just long enough ago to require re-sending - this means
  // that really old un-acked messages are still re-sent first, but that new messages will have a good chance to be sent
  
  const NetTimeStamp neverSentTime = currentTime - (sTimeBetweenResendsInMS + 1.0);
  const NetTimeStamp shouldBeAckedTime = _latestRecvTime - sMinExpectedPacketAckTimeMS; // anything sent before this date should be acked
  NetTimeStamp lastSentTimeForOldestMessage = kNetTimeStampZero;
  
  size_t oldestMessageIdx = 0;
  for (size_t i=0; i < _pendingMessageList.size(); ++i)
  {
    NetTimeStamp messageSentTime = _pendingMessageList[i]->GetLastSentTime();
    if (messageSentTime == kNetTimeStampZero)
    {
      // message has never been sent
      messageSentTime = neverSentTime;
    }
    else if ((shouldBeAckedTime > kNetTimeStampZero) && (messageSentTime < shouldBeAckedTime))
    {
      // message was sent long enough ago that it should ideally have been acked in the last received message
      // this suggests that it wasn't received, so prioritize re-sending ASAP
      messageSentTime -= sTimeBetweenResendsInMS;
    }
    
    if ((i==0) || (messageSentTime < lastSentTimeForOldestMessage))
    {
      lastSentTimeForOldestMessage = messageSentTime;
      oldestMessageIdx = i;
    }
  }
  
  uint32_t numPacketsSent = 0;
  
  if (IsPacketWorthSending(reliableTransport, currentTime, oldestMessageIdx) && (currentTime > (lastSentTimeForOldestMessage + sTimeBetweenResendsInMS)))
  {
    // Re-send any un-acked messages starting with oldestMessageIdx, up to maxPacketsToSend combined packets
    
    numPacketsSent = SendUnAckedPackets(reliableTransport, maxPacketsToSend, oldestMessageIdx);
    
    ANKI_NET_PRINT_VERBOSE("ReliableConnection.SendOptimalUnAckedPackets",
                           "%p '%s' SendOptimalUnAckedPackets _nextOutSequenceId = %u, %zu (%d..%d) - sent %u packets from msg %zu",
                           this, _transportAddress.ToString().c_str(),
                           _nextOutSequenceId, _pendingMessageList.size(),
                           (int)GetFirstUnackedOutId(), (int)GetLastUnackedOutId(), numPacketsSent, oldestMessageIdx);
  }
  
  return numPacketsSent;
}
  
  
bool ReliableConnection::Update(ReliableTransport* reliableTransport)
{
  const NetTimeStamp currentTime = GetCurrentNetTimeStamp();
  
  // Try to send/queue ping before sending packets (as ping may just be part of one of the packets)
  // Even if not sending separate pings, force a ping to be sent if nothing at all has been sent in
  // the timeout period (to keep the connection open) and there is nothing already pending to send
  
  if (sSendSeparatePingMessages ||
      (_pendingMessageList.empty() && (_latestMessageSentTime > kNetTimeStampZero) &&
       (currentTime > (_latestMessageSentTime + sTimeBetweenResendsInMS))))
  {
    if (currentTime >= (_latestPingSentTime + sTimeBetweenPingsInMS))
    {
      SendPing(reliableTransport);
    }
  }
  
  SendOptimalUnAckedPackets(reliableTransport, sMaxPacketsToReSendOnUpdate);

  return !HasConnectionTimedOut();
}


ReliableSequenceId ReliableConnection::GetFirstUnackedOutId() const
{
  // Find sequence id of first reliable message in pending list
  
  const size_t pendingMessageListSize = _pendingMessageList.size();
  for (size_t i = 0; i < pendingMessageListSize; ++i)
  {
    PendingMessage* pendingMessage = _pendingMessageList[i];
    if (pendingMessage->IsReliable())
    {
      return pendingMessage->GetSequenceId();
    }
  }
  
  return k_InvalidReliableSeqId;
}


ReliableSequenceId ReliableConnection::GetLastUnackedOutId() const
{
  // Find sequence id of last reliable message in pending list
  
  const size_t pendingMessageListSize = _pendingMessageList.size();
  for (size_t i = pendingMessageListSize; i > 0 ; )
  {
    --i;
    PendingMessage* pendingMessage = _pendingMessageList[i];
    if (pendingMessage->IsReliable())
    {
      return pendingMessage->GetSequenceId();
    }
  }
  
  return k_InvalidReliableSeqId;
}


void ReliableConnection::Print() const
{
#if ANKI_NET_MESSAGE_LOGGING_ENABLED
  const NetTimeStamp currentTime = GetCurrentNetTimeStamp();

  const NetTimeStamp timeSinceLastSend = currentTime - _latestMessageSentTime;
  const NetTimeStamp timeSinceLastRecv = currentTime - _latestRecvTime;
  const NetTimeStamp timeSinceLastPingSent = currentTime - _latestPingSentTime;

  PRINT_CH_INFO("Network", "ReliableConnection.Print", "'%s': now = %.1f: Out: %zu unacked (%u..%u), next out %u",
                       _transportAddress.ToString().c_str(), currentTime, _pendingMessageList.size(),
                       GetFirstUnackedOutId(), GetLastUnackedOutId(), _nextOutSequenceId);

  PRINT_CH_INFO("Network", "ReliableConnection.Print", "'%s': ms since last: send=%.1f, recv=%.1f",
                       _transportAddress.ToString().c_str(), timeSinceLastSend, timeSinceLastRecv);

  PRINT_CH_INFO("Network", "ReliableConnection.Print", "'%s': In: Last Acked %u, Waiting for %u",
                       _transportAddress.ToString().c_str(), _lastInAckedMessageId, _nextInSequenceId);

  if (_pendingMultiPartMessage.IsWaitingForParts())
  {
    PRINT_CH_INFO("Network", "ReliableConnection.Print", "'%s': Pending waiting on %u of %u (%u bytes received)",
                         _transportAddress.ToString().c_str(), _pendingMultiPartMessage.GetNextExpectedPart(), _pendingMultiPartMessage.GetLastExpectedPart(), _pendingMultiPartMessage.GetSize());
  }

  const double averagePingRoundTrip = _pingRoundTripTimes.GetMean();
  const double stdDevPingRoundTrip  = _pingRoundTripTimes.GetStd();

  uint32_t numPingSamples = _pingRoundTripTimes.GetNum();
  const float pingRTMin = (numPingSamples > 0) ? _pingRoundTripTimes.GetMin() : 0.0f;
  const float pingRTMax = (numPingSamples > 0) ? _pingRoundTripTimes.GetMax() : 0.0f;

  PRINT_CH_INFO("Network", "ReliableConnection.Print", "'%s': Ping RoundTrip: %u samples (%.2f ms mean, %.2f sd) (%.2f min, %.2f max)",
                       _transportAddress.ToString().c_str(), numPingSamples, averagePingRoundTrip, stdDevPingRoundTrip, pingRTMin, pingRTMax);

  PRINT_CH_INFO("Network", "ReliableConnection.Print", "'%s': Ping Last sent %.1f ms ago. %u of %u (%.1f%%) delivered. %u of %u (%.1f%%) received.",
                       _transportAddress.ToString().c_str(), timeSinceLastPingSent,
                       _numPingsSentThatArrived, _numPingsSent, GetPingAckedPercentage(),
                       _numPingsReceived, _numPingsSentTowardsUs, GetPingArrivedPercentage());
#endif // ANKI_NET_MESSAGE_LOGGING_ENABLED
}


} // end namespace Util
} // end namespace Anki
