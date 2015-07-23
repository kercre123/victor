#include "anki/cozmo/shared/cozmoTypes.h"
#include "BlockMessages.h"
#include <stdio.h>

namespace Anki {
  namespace Cozmo {
    namespace BlockMessages {
      
      
      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
#     define MESSAGE_DEFINITION_MODE MESSAGE_PROCESS_METHODS_MODE
#     include "BlockMessageDefinitions.def"
      
      
      // Fill in the message information lookup table for getting size and
      // ProcesBufferAs_MessageX function pointers according to enumerated
      // message ID.
      struct {
        u8 priority;
        u16 size;
        Result (*ProcessPacketAs)(const u8*);
      } lookupTable_[NUM_BLOCK_MSG_IDS+1] = {
        {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
        
#     define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#     include "BlockMessageDefinitions.def"
        
        {0, 0, 0} // Final dummy entry without comma at end
      };
      
      u16 GetSize(const ID msgID) {
        return lookupTable_[msgID].size;
      }
      
      Result ProcessMessage(const u8* buffer, const u8 bufferSize)
      {
        Result retVal = RESULT_FAIL;
        
        const u8 msgID = buffer[0];
        
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
