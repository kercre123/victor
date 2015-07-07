/**
 * File: robotMessages.cpp  (Basestation)
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

#include <cstring> // for memcpy used in the constructor, created by MessageDefinitions
#include <array>

#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/logging/logging.h"

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"




namespace Anki {
  namespace Cozmo {
    
#define MESSAGE_BASECLASS_NAME RobotMessage
    
    RobotMessage::~RobotMessage()
    {
      
    }
    
    RobotMessage* RobotMessage::CreateFromJson(const Json::Value &jsonRoot)
    {
      RobotMessage* msg = nullptr;
      
      if(!jsonRoot.isMember("Name")) {
        PRINT_NAMED_ERROR("RobotMessage.CreateFromJson.MissingName", "No 'Name' member for RobotMessage in Json file.\n");
      } else {
        
        const std::string msgType = jsonRoot["Name"].asString();
        
#       define MESSAGE_DEFINITION_MODE MESSAGE_CREATEFROMJSON_LADDER_MODE
#       include "anki/cozmo/shared/RobotMessageDefinitions.h"
        {
          PRINT_NAMED_WARNING("RobotMessage.CreateFromJson.UnknownMessageType",
                              "Encountered unknown RobotMessage type '%s' in Json file.\n",
                              msgType.c_str());
        } // if/else kfMsgType matches each string
        
      } // if/else isMember("Name")
      
      return msg;
      
    } // CreateFromJson()

    // Implement all the message classes' default constructors
#   define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFAULT_CONSTRUCTOR_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
    // Implement all the message classes' constructors from byte buffers
#   define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_BUFFER_CONSTRUCTOR_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
    // Implement all the message classes' constructors from json files
#   define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"

    // Implement all the message classes' GetID() methods
#   define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETID_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
    // Implement all the message classes' GetSize() methods
#   define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETSIZE_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"

    // Implement all the message classes' GetBytes() methods
#   define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETBYTES_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"

    // Implement all the message classes' CreatJson() methods
#   define MESSAGE_DEFINITION_MODE MESSAGE_CREATE_JSON_MODE
#   include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
#undef MESSAGE_BASECLASS_NAME


  } // namespace Cozmo
} // namespace Anki
