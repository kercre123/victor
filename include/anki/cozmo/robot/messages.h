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

#ifndef COZMO_MESSAGE_ROBOT_H
#define COZMO_MESSAGE_ROBOT_H

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
      
      void ProcessBTLEMessages();
      void ProcessUARTMessages();
      
      void ProcessMessage(const ID msgID, const u8* buffer);
      
      // Start looking for a particular message ID
      void LookForID(const ID msgID);
      
      // Did we see the message ID we last set? (Or perhaps we timed out)
      bool StillLookingForID(void);
      
      // These return true if a mailbox messages was available, and they copy
      // that message into the passed-in message struct.
      bool CheckMailbox(BlockMarkerObserved& msg);
      bool CheckMailbox(MatMarkerObserved&   msg);
      bool CheckMailbox(DockingErrorSignal&  msg);
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MESSAGE_ROBOT_H

