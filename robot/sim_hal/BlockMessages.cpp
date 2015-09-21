#include "anki/cozmo/shared/cozmoTypes.h"
#include "clad/types/activeObjectTypes.h"
#include "BlockMessages.h"
#include <stdio.h>

namespace Anki {
  namespace Cozmo {
    namespace BlockMessages {

      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
#     include "clad/types/activeObjectTypes_declarations.def"

      
      Result ProcessMessage(const u8* buffer, const u8 bufferSize)
      {
        Result retVal = RESULT_FAIL;
        
	ActiveObjectMessage msg;

	
	
	
        
        if(lookupTable_[msgID].size != bufferSize-1) {
          printf("BlockMessages.MessageBufferWrongSize: "
                 "Buffer's size does not match expected size for this message ID. (Msg %d, expected %d, recvd %d)\n",
                 msgID,
                 lookupTable_[msgID].size,
                 bufferSize - 1
                 );
        }
        else {
          // This calls the registered callback for the message
          retVal = (lookupTable_[msgID].ProcessPacketAs)(buffer+1);
        }
        
        return retVal;
      } // ProcessBuffer()
      
      
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
