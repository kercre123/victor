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

#ifndef __NetworkService_PendingMessage_H__
#define __NetworkService_PendingMessage_H__

#include "util/transport/netTimeStamp.h"
#include "util/transport/reliableSequenceId.h"
#include "util/transport/reliableMessageTypes.h"

namespace Anki {
namespace Util {

class SrcBufferSet;

class PendingMessage
{
public:
  
  PendingMessage();
  ~PendingMessage();
  
  PendingMessage(PendingMessage&& rhs) noexcept = default;
  PendingMessage& operator=(PendingMessage&& rhs) noexcept = default;
  
  void Set(const SrcBufferSet& srcBuffers, EReliableMessageType messageType, ReliableSequenceId seqId, bool flushPacket);

  void DestroyMessage();

  const uint8_t*        GetMessageBytes() const { return _message; }
  uint32_t              GetMessageSize()  const { return _messageSize; }
  ReliableSequenceId    GetSequenceId()   const { return _sequenceNumber; }
  EReliableMessageType  GetType()         const { return (EReliableMessageType)_messageType; }
  
  bool                  IsReliable()      const { return (_sequenceNumber != k_InvalidReliableSeqId); }

  NetTimeStamp          GetFirstSentTime() const { return _firstSentTime; }
  NetTimeStamp          GetLastSentTime()  const { return _lastSentTime; }
  void                  UpdateLatestSentTime(NetTimeStamp newVal);
  
  bool                  ShouldFlushPacket() const { return _flushPacket; }
  
private:
  
  // Prevent copy-construct/assignment
  PendingMessage(const PendingMessage& rhs);
  PendingMessage& operator=(const PendingMessage& rhs);

  NetTimeStamp        _firstSentTime;
  NetTimeStamp        _lastSentTime;
  uint8_t*            _message;
  uint32_t            _messageSize;
  ReliableSequenceId  _sequenceNumber;
  uint8_t             _messageType;
  bool                _flushPacket; // is the message important enough to send even without a full packet of stuff
};



} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_PendingMessage_H__
