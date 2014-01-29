/**
 * File: messages.h  (Basestation)
 *
 * Author: Andrew Stein
 * Date:   1/21/2014
 *
 * Description: Defines a base Message class from which all the other messages
 *              inherit.  The inherited classes are auto-generated via multiple
 *              re-includes of the MessageDefinitions.h file, with specific
 *              "mode" flags set.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_MESSAGE_BASESTATION_H
#define COZMO_MESSAGE_BASESTATION_H

#include "anki/common/types.h"

namespace Anki {

  namespace Cozmo {
    
    // 1. Initial include just defines the definition modes for use below
#undef MESSAGE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
    
    // Base message class
    class Message
    {
    public:
      
      virtual void GetBytes(u8* buffer) const = 0;
      
      virtual ReturnCode SendToRobot(const RobotID_t robotID);
      
      static u8 GetSize() { return 0; }
      
    }; // class Message


    // 2. Define all the message classes:
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"


  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MESSAGE_BASESTATION_H

