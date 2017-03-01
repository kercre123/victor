/**
 * File: pendingMessage
 *
 * Author: Mark Wesley
 * Created: 02/04/15
 *
 * Description: A single message that is either:
 *              1) unreliable and not sent yet
 *              2)  reliable and hasn't been confirmed as successfully delivered yet
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/transport/pendingMessage.h"
#include "util/debug/messageDebugging.h"
#include "util/transport/srcBufferSet.h"


namespace Anki {
namespace Util {

PendingMessage::PendingMessage()
  : _externalQueuedTime(kNetTimeStampZero)
  , _queuedTime(kNetTimeStampZero)
  , _firstSentTime(kNetTimeStampZero)
  , _lastSentTime(kNetTimeStampZero)
  , _message(nullptr)
  , _messageSize(0)
  , _sequenceNumber(k_InvalidReliableSeqId)
  , _messageType(eRMT_Invalid)
  , _flushPacket(false)
{
}

  
PendingMessage::~PendingMessage()
{
  DestroyMessage();
}

  
void PendingMessage::Set(const SrcBufferSet& srcBuffers, EReliableMessageType messageType, ReliableSequenceId seqId,
                         bool flushPacket, NetTimeStamp externalQueuedTime)
{
  assert(_message == nullptr);
  DestroyMessage();

  _externalQueuedTime = externalQueuedTime;
  _queuedTime     = GetCurrentNetTimeStamp();
  _firstSentTime  = kNetTimeStampZero;
  _lastSentTime   = kNetTimeStampZero;
  _message        = srcBuffers.CreateCombinedBuffer();
  _messageSize    = srcBuffers.CalculateTotalSize();
  _sequenceNumber = seqId;
  _messageType    = messageType;
  _flushPacket    = flushPacket;
  
  // seqId == invalid implies unreliable message, this must be in sync with reliability of type
  // (otherwise multi-messages will incorrectly determine if that message contributes to the seqId increment)
  assert((seqId == k_InvalidReliableSeqId) == IsMessageTypeAlwaysSentUnreliably(messageType));
  
  ANKI_NET_MESSAGE_VERBOSE(("ReliableMessage.Set", "", _message, _messageSize, &srcBuffers));
}
  
  
void PendingMessage::DestroyMessage()
{
  if (_message != nullptr)
  {
    SrcBufferSet::DestroyCombinedBuffer(_message);
    _message = nullptr;
  }
}
  
  
void PendingMessage::UpdateLatestSentTime(NetTimeStamp newVal)
{
  if (_firstSentTime == kNetTimeStampZero)
  {
    _firstSentTime = newVal;
  }
  _lastSentTime = newVal;
}


} // end namespace Util
} // end namespace Anki
