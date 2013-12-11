#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "anki/common/types.h"
#include "anki/cozmo/messageProtocol.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace CommandHandler {
      
      void ProcessIncomingMessages();
      
      CozmoMessageID ProcessMessage(const u8* buffer);
      
    } // namespace CommandHandler
    
  } // namespace Cozmo
  
} // namespace Anki

#endif
