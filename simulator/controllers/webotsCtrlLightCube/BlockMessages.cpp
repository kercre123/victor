#include "anki/types.h"
#include "clad/robotInterface/lightCubeMessage.h"
#include "BlockMessages.h"
#include <stdio.h>

namespace Anki {
  namespace Cozmo {
    
    namespace ActiveBlock {
      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
      #include "clad/robotInterface/lightCubeMessage_declarations.def"

      void ProcessBadTag_LightCubeMessage(const BlockMessages::LightCubeMessage::Tag tag);
    }
    
    namespace BlockMessages {

      Result ProcessMessage(const u8* buffer, const u8 bufferSize)
      {
        using namespace ActiveBlock;
        LightCubeMessage msg;
        
        memcpy(msg.GetBuffer(), buffer, bufferSize);
              
        #include "clad/robotInterface/lightCubeMessage_switch.def"
        
        return RESULT_OK;
      } // ProcessBuffer()      
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
