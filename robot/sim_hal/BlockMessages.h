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
      
      Result ProcessMessage(const u8* buffer, const u8 bufferSize);
      
    } // namespace BlockMessages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef ROBOT_2_BLOCK_MESSAGES_H

