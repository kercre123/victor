/**
 * File: uiMessages.h  (Basestation)
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_UI_MESSAGE_BASESTATION_H
#define COZMO_UI_MESSAGE_BASESTATION_H

#include <array>
#include "json/json.h"

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    
#define MESSAGE_BASECLASS_NAME UiMessage
    
    // 1. Initial include just defines the definition modes for use below
#undef MESSAGE_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
    
    // Create the enumerated message IDs from the MessageDefinitions file:
    typedef enum {
      NO_UI_MESSAGE_ID = 0,
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
      NUM_UI_MSG_IDS // Final entry without comma at end
    } UI_MSG_ID;
    
    
    // Base message class
    class UiMessage
    {
    public:
      
      virtual u8 GetID() const = 0;
      
      virtual void GetBytes(u8* buffer) const = 0;
      
      virtual u16 GetSize() const = 0;
      
      virtual ~UiMessage() { }
      
    }; // class UiMessage
    
    
    // 2. Define all the ui message classes:
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
    
#undef MESSAGE_BASECLASS_NAME

  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_UI_MESSAGE_BASESTATION_H

