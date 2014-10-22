/**
 * File: messages.cpp  (Basestation)
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

#include <cstring> // for memcpy used in the constructor, created by MessageDefinitions
#include <array>

#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/jsonTools.h"

#include "anki/cozmo/basestation/messages.h"



namespace Anki {
  namespace Cozmo {
    
    Message::~Message()
    {
      
    }
    
#define MESSAGE_BASECLASS_NAME Message
    
    // Impelement all the message classes' constructors from byte buffers
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_BUFFER_CONSTRUCTOR_MODE
#include "anki/cozmo/shared/MessageDefinitions.h"
    
    // Implement all the message classes' constructors from json files
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE
#include "anki/cozmo/shared/MessageDefinitions.h"

    // Implement all the message classes' GetID() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETID_MODE
#include "anki/cozmo/shared/MessageDefinitions.h"
    
    // Implement all the message classes' GetSize() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETSIZE_MODE
#include "anki/cozmo/shared/MessageDefinitions.h"

    // Implement all the message classes' GetBytes() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETBYTES_MODE
#include "anki/cozmo/shared/MessageDefinitions.h"

    // Implement all the message classes' CreatJson() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CREATE_JSON_MODE
#include "anki/cozmo/shared/MessageDefinitions.h"
    
#undef MESSAGE_BASECLASS_NAME


  } // namespace Cozmo
} // namespace Anki
