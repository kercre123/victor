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

#include "util/transport/pendingMultiPartMessage.h"
#include "util/debug/messageDebugging.h"
#include "util/math/math.h"
#include <assert.h>


namespace Anki {
namespace Util {
      

PendingMultiPartMessage::PendingMultiPartMessage()
{
  Clear();
}

      
PendingMultiPartMessage::~PendingMultiPartMessage()
{
}


bool PendingMultiPartMessage::AddMessagePart(const uint8_t* message, uint32_t messageSize)
{
  if (messageSize > k_MultiPartMessageHeaderSize)
  {
    ANKI_NET_MESSAGE_VERBOSE(("HandleMultiPartMessage Recv", "|Mu|Mu|Message...", message, messageSize));
    
    const uint8_t messageIndex = message[0];
    const uint8_t messageCount = message[1];
    
    messageSize -= k_MultiPartMessageHeaderSize;
    message = &message[k_MultiPartMessageHeaderSize];
    
    if (messageIndex == _nextExpectedPart)
    {
      if ((messageIndex == 1) && (_nextExpectedPart == 1))
      {
        // start new
        ANKI_NET_PRINT_VERBOSE("MultiPartMessage", "new message %d of %d", messageIndex, messageCount);
        _lastExpectedPart = messageCount;
      }
      
      if (messageCount != _lastExpectedPart)
      {
        // Message size changed!?
        assert(0);
        //return false;
      }
      
      ANKI_NET_PRINT_VERBOSE("MultiPartMessage", "Message part %d of %d - adding %u bytes to %u existing", messageIndex, messageCount, messageSize, (uint32_t)_bytes.size());
      
      // Resize the buffer and copy all the bytes in one go
      const size_t oldSize = _bytes.size();
      _bytes.resize(oldSize + messageSize);
      memcpy(&_bytes[oldSize], message, messageSize);
      
      ++_nextExpectedPart;
      
      if (messageIndex == _lastExpectedPart)
      {
        // message is ready!
        return true;
      }
    }
    else
    {
      // incorrect order!?
      assert(0);
    }
  }
  else
  {
    // too small to hold a message!?
    assert(0);
  }
  
  return false;
}


void PendingMultiPartMessage::Clear()
{
  _lastExpectedPart = 0;
  _nextExpectedPart = 1;
  _bytes.clear();
}


const uint8_t* PendingMultiPartMessage::GetBytes() const
{
  return &_bytes[0];
}


uint32_t PendingMultiPartMessage::GetSize() const
{
  return Anki::Util::numeric_cast<uint32_t>(_bytes.size());
}


} // end namespace Util
} // end namespace Anki
