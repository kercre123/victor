/**
 * File: uiMessages.cpp  (Basestation)
 *
 * Copyright: Anki, Inc. 2014
 **/

#include <cstring> // for memcpy used in the constructor, created by MessageDefinitions
#include <array>

#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/jsonTools.h"

#include "anki/cozmo/game/comms/messaging/uiMessages.h"



namespace Anki {
  namespace Cozmo {
    
#define MESSAGE_BASECLASS_NAME UiMessage
    
    // Impelement all the message classes' constructors from byte buffers
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_BUFFER_CONSTRUCTOR_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
    
    // Implement all the message classes' constructors from json files
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

    // Implement all the message classes' GetID() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETID_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
    
    // Implement all the message classes' GetSize() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETSIZE_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

    // Implement all the message classes' GetBytes() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETBYTES_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

    // Implement all the message classes' CreatJson() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CREATE_JSON_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
 
#undef MESSAGE_BASECLASS_NAME
    
  } // namespace Cozmo
} // namespace Anki
