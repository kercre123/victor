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

#include "anki/messaging/basestation/IComms.h"

namespace Anki {
  namespace Cozmo {
    
#include "anki/cozmo/MessageDefinitions.h"
    
    
#define USE_SINGLETON_MESSAGE_HANDLER 0
    
    class Robot;
    class RobotManager;
    class BlockWorld;
    
    // TODO: make singleton or not?
    class MessageHandler
    {
    public:
      
#if USE_SINGLETON_MESSAGE_HANDLER
      // Get a pointer to the singleton instance
      inline static MessageHandler* getInstance();
#else
      MessageHandler(); // Force construction with stuff in Init()?
#endif
      
      // Set the message handler's communications manager
      ReturnCode Init(Comms::IComms* comms,
                      RobotManager*  robotMgr,
                      BlockWorld*    blockWorld);
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      ReturnCode ProcessMessages();
      
      // Send a message to a specified ID
      ReturnCode SendMessage(const RobotID_t robotID, const Message& msg);
      
    protected:
      
#if USE_SINGLETON_MESSAGE_HANDLER
      // Protected default constructor for singleton.  This grabs a pointer
      // to the singleton RobotManager.
      MessageHandler();
      
      static MessageHandler* singletonInstance_;
#endif
      
      Comms::IComms* comms_;
      RobotManager* robotMgr_;
      BlockWorld*   blockWorld_;
      
      bool isInitialized_;

      
      // Process a raw byte buffer as a message and send it to the specified
      // robot
      ReturnCode ProcessPacket(const Comms::MsgPacket& packet);
      
      // Create the enumerated message IDs from the MessageDefinitions file:
      typedef enum {
        NO_MESSAGE_ID = 0,
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        NUM_MSG_IDS // Final entry without comma at end
      } ID;
      
      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
#define MESSAGE_DEFINITION_MODE MESSAGE_PROCESS_METHODS_MODE
#include "anki/cozmo/MessageDefinitions.h"
      
      // Fill in the message information lookup table for getting size and
      // ProcesBufferAs_MessageX function pointers according to enumerated
      // message ID.
      struct {
        u8 priority;
        u8 size;
        ReturnCode (MessageHandler::*ProcessPacketAs)(Robot*, const u8*);
      } lookupTable_[NUM_MSG_IDS+1] = {
        {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        {0, 0, 0} // Final dummy entry without comma at end
      };
      
    }; // class MessageHandler
    

#if USE_SINGLETON_MESSAGE_HANDLER
    inline MessageHandler* MessageHandler::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == singletonInstance_) {
        singletonInstance_ = new MessageHandler();
      }
      
      return singletonInstance_;
    }
#endif
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H