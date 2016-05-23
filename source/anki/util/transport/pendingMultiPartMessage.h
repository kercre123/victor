/**
 * File: pendingMultiPartMessage
 *
 * Author: Mark Wesley
 * Created: 02/04/15
 *
 * Description: A large "MultiPart" message that had to be split into pieces to send and is still being assembled by this client
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_PendingMultiPartMessage_H__
#define __NetworkService_PendingMultiPartMessage_H__


#include <stdint.h>
#include <vector>


namespace Anki {
namespace Util {
  
  
const uint32_t k_MultiPartMessageHeaderSize = 2; // 2 bytes to indicate message X of Y
  

class PendingMultiPartMessage
{
public:
  
  PendingMultiPartMessage();
  ~PendingMultiPartMessage();
  
  bool AddMessagePart(const uint8_t* message, uint32_t messageSize);
  
  void Clear();
  
  const uint8_t*  GetBytes() const;
  uint32_t        GetSize()  const;
  
  bool            IsWaitingForParts()   const { return ((_lastExpectedPart > 0) && (_nextExpectedPart <= _lastExpectedPart)); }
  uint32_t        GetLastExpectedPart() const { return _lastExpectedPart; }
  uint32_t        GetNextExpectedPart() const { return _nextExpectedPart; }
  
private:
  
  uint32_t              _lastExpectedPart;
  uint32_t              _nextExpectedPart;
  std::vector<uint8_t>  _bytes;
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_PendingMultiPartMessage_H__
