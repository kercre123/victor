/**
 * File: messages.h  (was messageProtocol.h)
 * 
 * Author: Kevin Yoon
 * Created: 9/24/2013 
 *
 * Major overhaul to use macros for generating message defintions
 * Author: Andrew Stein
 * Date:   10/13/2013
 *
 * Description: This files uses the information and macros in the
 *              MessageDefinitions.h to create the typedef'd message
 *              structs passed via USB / BTLE and between main and 
 *              long execution "threads".  It also creates the enumerated
 *              list of message IDs.  Everything in this file is independent
 *              of specific message definitions -- those are defined in
 *              MessageDefinitions.h.
 *
 * Copyright: Anki, Inc. 2013
 **/

#ifndef ROBOT_2_BLOCK_MESSAGES_H
#define ROBOT_2_BLOCK_MESSAGES_H

#include "anki/common/types.h"
#include <functional>

namespace Anki {
  namespace Cozmo {
    namespace BlockMessages {
      
      // 1. Initial include just defines the definition modes for use below
#     include "BlockMessageDefinitions.def"
      
      // 2. Define all the message structs:
#     define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#     include "BlockMessageDefinitions.def"
      
      // 3. Create the enumerated message IDs:
      typedef enum {
        NO_MESSAGE_ID = 0,
#       undef MESSAGE_DEFINITION_MODE
#       define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#     include "BlockMessageDefinitions.def"
        NUM_BLOCK_MSG_IDS // Final entry without comma at end
      } ID;
      
      
      // Declare registration functions for message handling callbacks
#     define MESSAGE_DEFINITION_MODE MESSAGE_REG_CALLBACK_METHODS_MODE
#     include "BlockMessageDefinitions.def"
     
      
      Result ProcessMessage(const u8* buffer, const u8 bufferSize);
      
    } // namespace BlockMessages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef ROBOT_2_BLOCK_MESSAGES_H

