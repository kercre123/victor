/**
 * File: transportStats
 *
 * Author: Mark Wesley
 * Created: 2/06/15
 *
 * Description: Stats for messages sent and received on a given transport connection or layer
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_TransportStats_H__
#define __NetworkService_TransportStats_H__


#include <stdint.h>


namespace Anki {
namespace Util {

  
#define ANKI_MESSAGE_ERRORS       \
  ANKI_MESSAGE_ERROR(TooSmall)    \
  ANKI_MESSAGE_ERROR(TooBig)      \
  ANKI_MESSAGE_ERROR(WrongHeader) \
  ANKI_MESSAGE_ERROR(BadCRC)      \
  ANKI_MESSAGE_ERROR(BadType)     \
  ANKI_MESSAGE_ERROR(OutOfOrder)  \
  ANKI_MESSAGE_ERROR(SendFailed)  \


enum EMessageError
{
 #define ANKI_MESSAGE_ERROR(expr) eME_ ## expr,
  ANKI_MESSAGE_ERRORS
 #undef ANKI_MESSAGE_ERROR
  eME_Count
};
  
  
const char* MessageErrorToString(EMessageError messageError);
  
  
class MessageStats
{
public:
  
  MessageStats(const char* inName);
  MessageStats() { }

  void Reset();
  
  void AddMessage(uint32_t messageSize);
  void AddError(EMessageError messageError);
  
  void Print(const char* inName) const;
  
  uint64_t  GetNumBytes()        const { return _numBytes; }
  uint32_t  GetNumMessages()     const { return _numMessages; }
  uint32_t  GetSmallestMessage() const { return _smallestMessage; }
  uint32_t  GetLargestMessage()  const { return _largestMessage; }
  uint32_t  GetErrorCount(EMessageError messageError) const;
  
private:
  
  const char* _name;
  
  uint64_t  _numBytes;
  uint32_t  _numMessages;
  uint32_t  _smallestMessage;
  uint32_t  _largestMessage;
  uint32_t  _errorCounts[eME_Count];
};
  
  
class TransportStats
{
public:
  
  TransportStats(const char* inName);
  ~TransportStats() { }
  
  void Reset();
  
  void AddRecvMessage(uint32_t messageSize);
  void AddSentMessage(uint32_t messageSize);
  
  void AddRecvError(EMessageError messageError);
  void AddSendError(EMessageError messageError);
  
  void Print() const;
  
  const MessageStats& GetSentStats() const { return _sentStats; }
  const MessageStats& GetRecvStats() const { return _recvStats; }
  
private:
  
  MessageStats  _sentStats;
  MessageStats  _recvStats;
  
  const char* _name;
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_TransportStats_H__
