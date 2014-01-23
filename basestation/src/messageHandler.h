/**
 * File: messageHandler.h
 *
 * Author: Andrew Stein
 * Date:   1/22/2014
 *
 * Description: Defines a singleton MessageHandler object to serve as the 
 *              middle man between a communications object, which provides
 *              raw byte buffers sent over some kind of communications channel,
 *              and robots.  The handler processes all available messages from
 *              the communications channel (without knowing the means of 
 *              transmission), turns the buffers into a Message objects, and 
 *              doles them out to their respective Robot objects.  It also does
 *              the reverse, creating byte buffers from Message objects, and 
 *              passing them along to the communications channel.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_MESSAGEHANDLER_H
#define COZMO_MESSAGEHANDLER_H

#include "anki/common/types.h"

#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robot.h" 

namespace Anki {
  
  // Placeholder until Kevin creates this
  namespace Comms {
    class CommsManager
    {
    public:
      
      static CommsManager* getInstance();
      
      bool IsInitialized() const;
      
    protected:
      
      static CommsManager* singletonInstance_;
      
      CommsManager() : isInitialized_(false) { }
      
      bool isInitialized_;
      
    }; // class CommsManager
  }
  
  namespace Cozmo {
    
#include "anki/cozmo/MessageDefinitions.h"
    
    // NOTE: this is a singleton class
    class MessageHandler
    {
    public:
      
      // Set the message handler's communications manager
      ReturnCode Init(Comms::CommsManager* comms);
      
      // Get a pointer to the singleton instance
      inline static MessageHandler* getInstance();
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      ReturnCode ProcessMessages();
      
      // Send a message to a specified ID
      ReturnCode SendMessage(const RobotID_t robotID, const Message& msg);
      
    protected:
      
      // Protected default constructor for singleton.  This grabs a pointer
      // to the singleton RobotManager.
      MessageHandler();
      
      static MessageHandler* singletonInstance_;
      
      RobotManager* robotMgr_;
      
      bool isInitialized_;
      Comms::CommsManager* comms_;
      
      // Process a raw byte buffer as a message and send it to the specified
      // robot
      ReturnCode ProcessBuffer(const RobotID_t robotID, const u8* buffer, const u8 bufferSize);
      
      // Create the enumerated message IDs from the MessageDefinitions file:
      typedef enum {
        NO_MESSAGE_ID = 0,
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        NUM_MSG_IDS // Final entry without comma at end
      } ID;
      
      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
#define MESSAGE_DEFINITION_MODE MESSAGE_PROCESS_BUFFER_METHODS_MODE
#include "anki/cozmo/MessageDefinitions.h"
      
      // Fill in the message information lookup table for getting size and
      // ProcesBufferAs_MessageX function pointers according to enumerated
      // message ID.
      struct {
        u8 priority;
        u8 size;
        ReturnCode (MessageHandler::*ProcessBufferAs)(const RobotID_t robotID, const u8* buffer);
      } lookupTable_[NUM_MSG_IDS+1] = {
        {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        {0, 0, 0} // Final dummy entry without comma at end
      };
      
    }; // class MessageHandler
    
    
    inline MessageHandler* MessageHandler::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == singletonInstance_) {
        singletonInstance_ = new MessageHandler();
      }
      
      return singletonInstance_;
    }
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H