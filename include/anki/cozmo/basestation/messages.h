/**
 * File: messages.h  (Basestation)
 *
 * Author: Andrew Stein
 * Date:   1/21/2014
 *
 * Description: Currently this is basically just a copy of the robot's 
 *              messages.h header file. Once the basestation has its own data
 *              structures for messages, then this file should probably be 
 *              updated.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_MESSAGE_BASESTATION_H
#define COZMO_MESSAGE_BASESTATION_H

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {
      
      // 1. Initial include just defines the definition modes for use below
#include "anki/cozmo/MessageDefinitions.h"
      
      // 2. Define all the message structs:
#define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
      
      // 3. Create the enumerated message IDs:
      typedef enum {
        NO_MESSAGE_ID = 0,
#undef MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        NUM_MSG_IDS // Final entry without comma at end
      } ID;
      
      // Return the size of a message, given its ID
      u8 GetSize(const ID msgID);
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MESSAGE_BASESTATION_H

