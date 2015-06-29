/**
 * File: robotMessageHandler.h
 *
 * Author: Andrew Stein
 * Date:   1/22/2014
 *
 * Description: Defines a singleton RobotMessageHandler object to serve as the
 *              middle man between a communications object, which provides
 *              raw byte buffers sent over some kind of communications channel,
 *              and robots.  The handler processes all available messages from
 *              the communications channel (without knowing the means of 
 *              transmission), turns the buffers into a RobotMessage objects, and
 *              doles them out to their respective Robot objects.  It also does
 *              the reverse, creating byte buffers from RobotMessage objects, and
 *              passing them along to the communications channel.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_MESSAGEHANDLER_H
#define COZMO_MESSAGEHANDLER_H

#include "anki/common/types.h"

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

#include "anki/vision/basestation/image.h"

#include "anki/messaging/basestation/IChannel.h"

namespace Anki {
  namespace Cozmo {
    
#define MESSAGE_BASECLASS_NAME RobotMessage
#include "anki/cozmo/shared/RobotMessageDefinitions.h"
    
    class Robot;
    class RobotManager;
    class BlockWorld;
    
    class IRobotMessageHandler
    {
    public:
      
      // TODO: Change these to interface references so they can be stubbed as well
      virtual Result Init(Comms::IChannel* comms,
                          RobotManager*  robotMgr) = 0;
      
      virtual Result ProcessMessages() = 0;
      
      virtual Result SendMessage(const RobotID_t robotID, const RobotMessage& msg, bool reliable = true, bool hot = false) = 0;
      
    }; // IRobotMessageHandler
    
    
    class RobotMessageHandler : public IRobotMessageHandler
    {
    public:
      
      RobotMessageHandler(); // Force construction with stuff in Init()?

      // Set the message handler's communications manager
      virtual Result Init(Comms::IChannel* comms,
                          RobotManager*  robotMgr);
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      virtual Result ProcessMessages();
      
      // Send a message to a specified ID
      virtual Result SendMessage(const RobotID_t robotID, const RobotMessage& msg, bool reliable = true, bool hot = false) override;
      
    protected:
      
      Comms::IChannel* channel_;
      RobotManager* robotMgr_;
      
      bool isInitialized_;

      Vision::ImageDeChunker _imageDeChunker;
      
      // Process a raw byte buffer as a message and send it to the specified
      // robot
      Result ProcessPacket(const Comms::IncomingPacket& packet);
      
      // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
#define MESSAGE_DEFINITION_MODE MESSAGE_PROCESS_METHODS_MODE
#include "anki/cozmo/shared/MessageDefinitionsR2B.def"
      
      // Fill in the message information lookup table for getting size and
      // ProcesBufferAs_MessageX function pointers according to enumerated
      // message ID.
      struct {
        u8 priority;
        u16 size;
        Result (RobotMessageHandler::*ProcessPacketAs)(Robot*, const u8*);
      } lookupTable_[RobotMessage::NUM_MSG_IDS+1] = {
        {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE
#include "anki/cozmo/shared/MessageDefinitionsB2R.def"
        
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#define MESSAGE_HANDLER_CLASSNAME RobotMessageHandler
#include "anki/cozmo/shared/MessageDefinitionsR2B.def"
#undef MESSAGE_HANDLER_CLASSNAME
        {0, 0, 0} // Final dummy entry without comma at end
      };
      
    }; // class MessageHandler
    
    
    class MessageHandlerStub : public IRobotMessageHandler
    {
    public:
      MessageHandlerStub() { }
      
      Result Init(Comms::IChannel* comms,
                  RobotManager*  robotMgr)
      {
        return RESULT_OK;
      }
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      Result ProcessMessages() {
        return RESULT_OK;
      }
      
      // Send a message to a specified ID
      Result SendMessage(const RobotID_t robotID, const RobotMessage& msg, bool reliable = true, bool hot = false) override
      {
        return RESULT_OK;
      }

      u32 GetNumMsgsSentThisTic(const RobotID_t robotID)
      {
        return 0;
      }

    }; // MessageHandlerStub
    
#undef MESSAGE_BASECLASS_NAME
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H