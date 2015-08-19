/**
 * File: robotMessages.h  (Basestation)
 *
 * Author: Andrew Stein
 * Date:   1/21/2014
 *
 * Description: Defines a base RobotMessage class from which all the other messages
 *              inherit.  The inherited classes are auto-generated via multiple
 *              re-includes of the MessageDefinitions.h file, with specific
 *              "mode" flags set.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_ROBOT_MESSAGE_BASESTATION_H
#define COZMO_ROBOT_MESSAGE_BASESTATION_H

#include <array>
#include "json/json.h"

#include "anki/common/types.h"

namespace Anki {

  namespace Cozmo {
    
#define MESSAGE_BASECLASS_NAME RobotMessage
    
    // 1. Initial include just defines the definition modes for use below
#undef MESSAGE_DEFINITION_MODE
#include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
    
    // Base message class
    class RobotMessage
    {
    public:
      // Create the enumerated message IDs from the MessageDefinitions file:
      typedef enum : u8 {
        NO_MESSAGE_ID = 0,
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/shared/RobotMessageDefinitions.h"
        NUM_MSG_IDS // Final entry without comma at end
      } ID;
      
      virtual ~RobotMessage();
      
      virtual u8 GetID() const = 0;
      
      virtual void GetBytes(u8* buffer) const = 0;
      
      virtual u16 GetSize() const = 0;
      
      virtual Json::Value CreateJson() const = 0;
      
      virtual Result FillFromJson(const Json::Value& root) = 0;
      
      static RobotMessage* CreateFromJson(const Json::Value& jsonRoot);
      
    }; // class RobotMessage


    // 2. Define all the message classes:
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFINITION_MODE
#include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
#undef MESSAGE_BASECLASS_NAME

  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_ROBOT_MESSAGE_BASESTATION_H

