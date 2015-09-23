#include "anki/cozmo/shared/cozmoTypes.h"
#include "clad/types/activeObjectTypes.h"
#include "BlockMessages.h"
#include <stdio.h>

namespace Anki {
  namespace Cozmo {
    namespace BlockMessages {

      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
      #include "clad/types/activeObjectTypes_declarations.def"

      Result ProcessMessage(const u8* buffer, const u8 bufferSize)
      {
        Result retVal = ;
        LightCubeMessage msg;
        
	      memcpy(msg.GetBuffer(), buffer, bufferSize);
        
        #include "clad/types/activeObjectTypes_switch.def"
        
        return RESULT_SUCCESS;
      } // ProcessBuffer()      
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
