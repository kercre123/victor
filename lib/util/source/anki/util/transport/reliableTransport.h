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

#ifndef __NetworkService_ReliableTransport_H__
#define __NetworkService_ReliableTransport_H__

#include "util/dispatchQueue/dispatchQueue.h"
#include "util/global/globalDefinitions.h"
#include "util/stats/recentStatsAccumulator.h"
#include "util/transport/iNetTransport.h"
#include "util/transport/iNetTransportDataReceiver.h"
#include "util/transport/netTimeStamp.h"
#include "util/transport/reliableMessageTypes.h"
#include "util/transport/reliableSequenceId.h"
#include "util/transport/transportAddress.h"
#include "util/transport/transportStats.h"
#include <map>
#include <mutex>


#if ANKI_PROFILING_ENABLED
  #define ENABLE_RT_UPDATE_TIME_DIAGNOSTICS 1
  #define ENABLE_RUN_TIME_PROFILING 0
#else
  #define ENABLE_RT_UPDATE_TIME_DIAGNOSTICS 0
  #define ENABLE_RUN_TIME_PROFILING 0
#endif

namespace Anki {
namespace Util {


class IUnreliableTransport;
class ReliableConnection;
class TransportAddress;


class ReliableTransport : public INetTransport, public INetTransportDataReceiver {

public:
  ReliableTransport(IUnreliableTransport* unreliableTransport, INetTransportDataReceiver* dataReceiver);
  virtual ~ReliableTransport();

  // for INetTransport
  virtual void SendData(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer, unsigned int bufferSize, bool flushPacket=false) override;
  virtual void Connect(const TransportAddress& destAddress) override;
  virtual void FinishConnection(const TransportAddress& destAddress) override;
  virtual void Disconnect(const TransportAddress& destAddress) override;
  virtual void StartHost() override;
  virtual void StopHost() override;
  virtual void StartClient() override;
  virtual void StopClient() override;
  virtual void FillAdvertisementBytes(AdvertisementBytes& bytes) override;
  virtual unsigned int FillAddressFromAdvertisement(TransportAddress& address, const uint8_t* buffer, unsigned int size) override;

  // for INetTransportDataReceiver
  virtual void ReceiveData(const uint8_t* buffer, unsigned int size, const TransportAddress& sourceAddress) override;

  void ReSendReliableMessage(ReliableConnection* connectionInfo, const uint8_t* buffer, unsigned int bufferSize, EReliableMessageType messageType, ReliableSequenceId seqIdMin, ReliableSequenceId seqIdMax);

  void QueueMessage(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer, unsigned int bufferSize,
                    EReliableMessageType messageType, bool flushPacket); // threadsafe equivalent of SendMessage, queue is processed safely later

  void SendMessage(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer, unsigned int bufferSize,
                   EReliableMessageType messageType, bool flushPacket, NetTimeStamp queuedTime=kNetTimeStampZero);

  void KillThread();
  void Update();
  void Print() const;

  uint32_t MaxTotalBytesPerMessage() const;

  const TransportStats& GetTransportStats() const { return _transportStats; }

  static void  SetSendAckOnReceipt(bool newVal)                  { sSendAckOnReceipt = newVal; }
  static void  SetSendUnreliableMessagesImmediately(bool newVal) { sSendUnreliableMessagesImmediately = newVal; }
  static void  SetTrackAckLatency(bool newVal)                  { sTrackAckLatency = newVal; }

  static bool  GetSendAckOnReceipt()                   { return sSendAckOnReceipt; }
  static bool  GetSendUnreliableMessagesImmediately()  { return sSendUnreliableMessagesImmediately; }
  static bool  GetTrackAckLatency()                    { return sTrackAckLatency; }

  static void  SetMaxPacketsToReSendOnAck(uint32_t newVal)       { sMaxPacketsToReSendOnAck = newVal; }
  static void  SetMaxPacketsToSendOnSendMessage(uint32_t newVal) { sMaxPacketsToSendOnSendMessage = newVal; }

  static uint32_t  GetMaxPacketsToReSendOnAck()        { return sMaxPacketsToReSendOnAck; }
  static uint32_t  GetMaxPacketsToSendOnSendMessage()  { return sMaxPacketsToSendOnSendMessage; }
  
  void ChangeSyncMode(bool isSync);
  bool IsSynchronous() const { return _runSynchronous; }

private:
  
  void UpdateNetStats();

  void StopClientInternal();
  void StopHostInternal();

  void QueueAction(std::function<void()> action);

  void HandleSubMessage(const uint8_t* innerMessage, uint32_t innerMessageSize, uint8_t messageType, ReliableSequenceId reliableSequenceId, ReliableConnection* connectionInfo, const TransportAddress& sourceAddress);

  uint32_t BuildHeader(uint8_t* outBuffer, uint32_t outBufferCapacity, uint8_t type, ReliableSequenceId seqIdMin, ReliableSequenceId seqIdMax, ReliableSequenceId lastReceivedId);

  ReliableConnection*   FindConnection(const TransportAddress& sourceAddressIn, bool createIfNew);
  void                  DeleteConnection(const TransportAddress& sourceAddress);
  void                  ClearConnections();

  // ============================== Static Member Vars ==============================

private:

  static bool             sSendAckOnReceipt;
  static bool             sSendUnreliableMessagesImmediately;
  static bool             sTrackAckLatency;
  static uint32_t         sMaxPacketsToReSendOnAck;
  static uint32_t         sMaxPacketsToSendOnSendMessage;

  // ============================== Member Vars ==============================

protected:

  IUnreliableTransport* _unreliable;

private:

  typedef std::map<TransportAddress, ReliableConnection*>  ReliableConnectionMap;
  ReliableConnectionMap   _reliableConnectionMap;
  Dispatch::QueueHandle   _queue;
  TaskHandle              _updateHandle;
  std::mutex              _mutex;

  TransportStats          _transportStats;
  
#if ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  Stats::RecentStatsAccumulator _timesBetweenUpdates;
  NetTimeStamp                  _lastUpdateTime;
  NetTimeStamp                  _lastReportOfSlowUpdate;
#endif // ENABLE_RT_UPDATE_TIME_DIAGNOSTICS
  
  bool                    _runSynchronous;
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_ReliableTransport_H__
