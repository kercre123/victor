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

#ifndef __NetworkService_ReliableConnection_H__
#define __NetworkService_ReliableConnection_H__


#include <vector>
#include "util/global/globalDefinitions.h"
#include "util/transport/netTimeStamp.h"
#include "util/transport/pendingMultiPartMessage.h"
#include "util/transport/reliableMessageTypes.h"
#include "util/transport/reliableSequenceId.h"
#include "util/transport/transportAddress.h"
#include "util/stats/recentStatsAccumulator.h"


namespace Anki {
namespace Util {

class PendingMessage;
class ReliableTransport;
class SrcBufferSet;
class TransportAddress;

#if ANKI_PROFILING_ENABLED
  #define ENABLE_RC_PACKET_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_RC_PACKET_TIME_DIAGNOSTICS 0
#endif
  
class ReliableConnection
{
public:

  ReliableConnection(const TransportAddress& inTransportAddress);
  ~ReliableConnection();

  void AddMessage(const SrcBufferSet& srcBuffers, EReliableMessageType messageType, ReliableSequenceId seqId,
                  bool flushPacket, NetTimeStamp queuedTime);

  PendingMultiPartMessage& GetPendingMultiPartMessage();

  ReliableSequenceId GetNextOutSequenceNumber();
  ReliableSequenceId GetLastInAckedMessageId() const { return _lastInAckedMessageId; }

  ReliableSequenceId GetNextInSequenceId() const { return _nextInSequenceId; }
  bool IsWaitingForAnyInRange(ReliableSequenceId minSeqId, ReliableSequenceId maxSeqId) const;
  bool IsNextInSequenceId(ReliableSequenceId sequenceNum) const;

  void AdvanceNextInSequenceId();

  void SendPing(ReliableTransport* reliableTransport, NetTimeStamp incomingPingTime = kNetTimeStampZero, bool isReply = false);
  void ReceivePing(ReliableTransport* reliableTransport, const uint8_t* message, uint32_t messageSize);

  void AckMessage(ReliableSequenceId seqId);              // i.e. we're saying "yes, we saw up to that message"
  void NotifyAckingMessageSent();             // whenever we send an outbound message that can ack something new

  // UpdateLastAckedMessage: i.e. we heard the other end say "yes, we saw up to that message"
  //     returns true if that updated the reliable queue (i.e. newer messages can now be sent/
  bool UpdateLastAckedMessage(ReliableSequenceId seqId);

  // returns number of packets sent (where each packet is >= 1 message), sends up to maxPacketsToSend packets
  uint32_t SendOptimalUnAckedPackets(ReliableTransport* reliableTransport, uint32_t maxPacketsToSend);

  bool Update(ReliableTransport* reliableTransport);

  void Print() const;

  const TransportAddress& GetAddress() const { return _transportAddress; }

  bool HasConnectionTimedOut() const;

  static NetTimeStamp GetTimeBetweenPingsInMS()   { return sTimeBetweenPingsInMS; }
  static NetTimeStamp GetTimeBetweenResendsInMS() { return sTimeBetweenResendsInMS; }
  static NetTimeStamp GetMaxTimeSinceLastSend()   { return sMaxTimeSinceLastSend; }
  static NetTimeStamp GetConnectionTimeoutInMS()  { return sConnectionTimeoutInMS; }
  static NetTimeStamp GetPacketSeparationIntervalInMS()  { return sPacketSeparationIntervalInMS; }
  static NetTimeStamp GetMinExpectedPacketAckTimeMS()    { return sMinExpectedPacketAckTimeMS; }
  static void SetTimeBetweenPingsInMS(NetTimeStamp newVal)   { sTimeBetweenPingsInMS = newVal; }
  static void SetTimeBetweenResendsInMS(NetTimeStamp newVal) { sTimeBetweenResendsInMS = newVal; }
  static void SetMaxTimeSinceLastSend(NetTimeStamp newVal)   { sMaxTimeSinceLastSend = newVal; }
  static void SetConnectionTimeoutInMS(NetTimeStamp newVal)  { sConnectionTimeoutInMS = newVal; }
  static void SetPacketSeparationIntervalInMS(NetTimeStamp newVal)  { sPacketSeparationIntervalInMS = newVal; }
  static void SetMinExpectedPacketAckTimeMS(NetTimeStamp newVal)    { sMinExpectedPacketAckTimeMS = newVal; }

  static uint32_t GetMaxPingRoundTripsToTrack()                { return sMaxPingRoundTripsToTrack; }
  static uint32_t GetMaxAckRoundTripsToTrack()                { return sMaxAckRoundTripsToTrack; }
  static uint32_t GetMaxPacketsToReSendOnUpdate()              { return sMaxPacketsToReSendOnUpdate; }
  static uint32_t GetMaxBytesFreeInAFullPacket()               { return sMaxBytesFreeInAFullPacket; }
  static void     SetMaxPingRoundTripsToTrack(uint32_t newVal)   { sMaxPingRoundTripsToTrack = newVal; }
  static void     SetMaxAckRoundTripsToTrack(uint32_t newVal)   { sMaxAckRoundTripsToTrack = newVal; }
  static void     SetMaxPacketsToReSendOnUpdate(uint32_t newVal) { sMaxPacketsToReSendOnUpdate = newVal; }
  static void     SetMaxBytesFreeInAFullPacket(uint32_t newVal)  { sMaxBytesFreeInAFullPacket = newVal; }

  static bool   GetSendSeparatePingMessages()            { return sSendSeparatePingMessages; }
  static void   SetSendSeparatePingMessages(bool newVal) { sSendSeparatePingMessages = newVal;}
  
  static void  SetSendPacketsImmediately(bool newVal)  { sSendPacketsImmediately = newVal; }
  static bool  GetSendPacketsImmediately()             { return sSendPacketsImmediately; }

  // Percentage of the pings we sent that other side has confirmed - likely to always be 1 ping down as the reply won't have arrived yet
  float GetPingAckedPercentage() const { return (_numPingsSent          > 0) ? (100.0f * float(_numPingsSentThatArrived) / float(_numPingsSent         )) : 0.0f; }
  // Percentage of the pings the other side says they sent towards us that we've received
  float GetPingArrivedPercentage() const { return (_numPingsSentTowardsUs > 0) ? (100.0f * float(_numPingsReceived       ) / float(_numPingsSentTowardsUs)) : 0.0f; }

  const Stats::StatsAccumulator& GetExternalQueuedTimeStats() const { return _externalQueuedTimes_ms.GetPrimaryAccumulator(); }
  const Stats::StatsAccumulator& GetQueuedTimeStats() const { return _queuedTimes_ms.GetPrimaryAccumulator(); }
  
  const Stats::StatsAccumulator& GetPingRoundTripStats() const { return _pingRoundTripTimes.GetPrimaryAccumulator(); }
  const Stats::StatsAccumulator& GetAckRoundTripStats() const { return _ackRoundTripTimes.GetPrimaryAccumulator(); }

  void AddRecvPacketStats(double packetSize, bool anyMessagesToReceive)
  {
  #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
    if (anyMessagesToReceive)
    {
      _recvStatsEver._bytesPerPacket.AddStat(packetSize);
      _recvStatsRecent._bytesPerPacket.AddStat(packetSize);
    }
    else
    {
      _recvStatsEver._bytesPerWastedPacket.AddStat(packetSize);
      _recvStatsRecent._bytesPerWastedPacket.AddStat(packetSize);
    }
  #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  }
  
  void AddRecvMessageStats(double messageSize, bool isReliable, bool isNewMessage)
  {
  #if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
    _recvStatsEver.GetRecentStats(isReliable,   isNewMessage).AddStat(messageSize);
    _recvStatsRecent.GetRecentStats(isReliable, isNewMessage).AddStat(messageSize);
  #endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  }

private:
  
  // ============================== Private Member Functions ==============================
  
  bool IsPacketWorthSending(const ReliableTransport* reliableTransport, NetTimeStamp currentTime, size_t firstMessage) const;
    
  // sends up to 1 packet (with as many messages as can fit, starting with firstToSend)
  // Returns number of messages sent (starting with firstToSend)
  // (it may also pack some messages before firstToSend in the packet too if there's room, but
  //  this is excluded from the count returned)
  size_t SendUnAckedMessages(ReliableTransport* reliableTransport, size_t firstToSend = 0);

  // returns number of packets sent (where each packet is >= 1 message), sends up to maxPacketsToSend packets filled with messages starting with firstToSend
  uint32_t SendUnAckedPackets(ReliableTransport* reliableTransport, uint32_t maxPacketsToSend, size_t firstToSend = 0);

  typedef std::vector<PendingMessage*> PendingMessageList;
  void      DestroyPendingMessageList(PendingMessageList& messageList);

  // ============================== Static Member Vars ==============================

  static NetTimeStamp sTimeBetweenPingsInMS;
  static NetTimeStamp sTimeBetweenResendsInMS;
  static NetTimeStamp sMaxTimeSinceLastSend;
  static NetTimeStamp sConnectionTimeoutInMS;
  static NetTimeStamp sPacketSeparationIntervalInMS;
  // how long between a packet being sent, and receiving an ack (assuming ACK is immediately sent), used to
  // evaluate likelihood of a message having been lost after receiving any ACK-ing message on that connection
  static NetTimeStamp sMinExpectedPacketAckTimeMS;
  static uint32_t sMaxPingRoundTripsToTrack; // Smaller number means more recent & responsive but jittery value
  static uint32_t sMaxAckRoundTripsToTrack;
  static uint32_t sMaxPacketsToReSendOnUpdate;
  static uint32_t sMaxBytesFreeInAFullPacket;
  static bool sSendSeparatePingMessages;
  static bool sSendPacketsImmediately;

  // ============================== Member Vars ==============================

  ReliableSequenceId  GetFirstUnackedOutId() const;
  ReliableSequenceId  GetLastUnackedOutId() const;

  TransportAddress            _transportAddress;

  PendingMultiPartMessage     _pendingMultiPartMessage;
  PendingMessageList          _pendingMessageList;

  // Track messages we've sent
  ReliableSequenceId          _nextOutSequenceId;     // sequence id for next outbound reliable message we send

  // Track incoming messages
  ReliableSequenceId          _lastInAckedMessageId;  // the last message we've acknowledged receiving
  ReliableSequenceId          _nextInSequenceId;

  NetTimeStamp                _latestMessageSentTime;
  NetTimeStamp                _latestRecvTime;

  NetTimeStamp                _latestPingSentTime;
  uint32_t                    _numPingsSent;            // pings we sent
  uint32_t                    _numPingsReceived;        // pings we received
  uint32_t                    _numPingsSentThatArrived; // pings we sent that the other side received
  uint32_t                    _numPingsSentTowardsUs;   // pings the other side claims to have sent

  Stats::RecentStatsAccumulator _externalQueuedTimes_ms; // Time between external Async queue request, and message being queued in connection
  Stats::RecentStatsAccumulator _queuedTimes_ms;         // Time between queued in connection, and message first being sent

  Stats::RecentStatsAccumulator _pingRoundTripTimes;     // Round trip times for a ping to be sent and bounced back
  Stats::RecentStatsAccumulator _ackRoundTripTimes;      // Time between first sending a message over a socket, and it being acknowledged by other end
  
#if ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  Stats::RecentStatsAccumulator _timesBetweenIncomingPackets;    // times between _any_ incoming packets
  Stats::RecentStatsAccumulator _timesBetweenNewIncomingPackets; // times between incoming packets with a new reliable message
  Stats::RecentStatsAccumulator _timesForAckingANewPacket; // times between receiving a new reliable message and acknowleding it
  NetTimeStamp                  _lastNewIncomingPacketTime;
  NetTimeStamp                  _unackedNewIncomingPacketTime;   // time we received the oldest unacked packet
  
  struct MessageStats
  {
    explicit MessageStats(uint32_t maxValuesToTrack) :_bytesPerUniqueMessage(maxValuesToTrack), _bytesPerRepeatableMessage(maxValuesToTrack) {}
    
    Stats::RecentStatsAccumulator& GetStats(bool isUnique) { return isUnique ? _bytesPerUniqueMessage : _bytesPerRepeatableMessage; }
    
    Stats::RecentStatsAccumulator _bytesPerUniqueMessage;       // a message can be queued or ack-read at most once <= 1
    Stats::RecentStatsAccumulator _bytesPerRepeatableMessage;   // a reliable message can be sent or received many times >= 0
  };
  
  struct TransmissionStats
  {
    explicit TransmissionStats(uint32_t maxValuesToTrack)
      : _reliable(maxValuesToTrack), _unreliable(maxValuesToTrack), _bytesPerPacket(maxValuesToTrack), _bytesPerWastedPacket(maxValuesToTrack) {}
    
    MessageStats& GetMessageStats(bool isReliable) { return isReliable ? _reliable : _unreliable; }
    Stats::RecentStatsAccumulator& GetRecentStats(bool isReliable, bool isQueued) { return GetMessageStats(isReliable).GetStats(isQueued); }
    
    MessageStats _reliable;
    MessageStats _unreliable;
    Stats::RecentStatsAccumulator _bytesPerPacket;
    Stats::RecentStatsAccumulator _bytesPerWastedPacket;
  };
  
  void LogStatsForSendingMessage(const PendingMessage* pendingMessage);
  
  TransmissionStats _sentStatsEver;
  TransmissionStats _sentStatsRecent;
  TransmissionStats _recvStatsEver;
  TransmissionStats _recvStatsRecent;
#endif // ENABLE_RC_PACKET_TIME_DIAGNOSTICS
  
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_ReliableConnection_H__
