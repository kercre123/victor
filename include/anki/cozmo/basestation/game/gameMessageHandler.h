/**
 * File: gameMessageHandler.h
 *
 * Author: Andrew Stein
 * Date:   12/15/2014
 *
 * Description: Handles messages from basestation (eventually, game) to UI, 
 *              analogously to the way MessageHandler handles messages from
 *              robot to basestation and UiMessageHandler handles messages
 *              from basestation to UI.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_GAME_MESSAGE_HANDLER_H
#define ANKI_COZMO_GAME_MESSAGE_HANDLER_H

#include "anki/common/types.h"

#include "anki/cozmo/basestation/uiTcpComms.h"
#include "anki/cozmo/basestation/game/gameMessages.h"

namespace Anki {
  
  namespace Comms {
    class IComms;
  }
  
namespace Cozmo {
  
  // Forward definitions:
  class Robot;
  class RobotManager;

  
#define MESSAGE_BASECLASS_NAME GameMessage
#include "anki/cozmo/basestation/game/MessageDefinitionsG2U.h"
  
  // Define interface for a GameMessage handler
  class IGameMessageHandler
  {
  public:
    
    // TODO: Change these to interface references so they can be stubbed as well
    virtual Result Init(Comms::IComms* comms,
                        RobotManager*  robotMgr) = 0;
    
    virtual Result ProcessMessages() = 0;
    
    virtual Result SendMessage(const UserDeviceID_t devID, const GameMessage& msg) = 0;
    
  }; // class IGameMessageHandler
  
  
  // The actual GameMessage handler implementation
  class GameMessageHandler : public IGameMessageHandler
  {
    GameMessageHandler(); // Force construction with stuff in Init()?
    
    // Set the message handler's communications manager
    virtual Result Init(Comms::IComms* comms,
                        RobotManager*  robotMgr);
    
    // As long as there are messages available from the comms object,
    // process them and pass them along to robots.
    virtual Result ProcessMessages();
    
    // Send a message to a specified ID
    Result SendMessage(const UserDeviceID_t devID, const GameMessage& msg);
    
  protected:
    
    Comms::IComms* comms_;
    RobotManager* robotMgr_;
    
    bool isInitialized_;
    
    // Process a raw byte buffer as a message and send it to the specified
    // robot
    Result ProcessPacket(const Comms::MsgPacket& packet);
    
    // Auto-gen the ProcessBufferAs_MessageX() method prototypes using macros:
#define MESSAGE_DEFINITION_MODE MESSAGE_PROCESS_METHODS_MODE
#include "anki/cozmo/basestation/game/MessageDefinitionsG2U.h"
    
    // Fill in the message information lookup table for getting size and
    // ProcesBufferAs_MessageX function pointers according to enumerated
    // message ID.
    struct {
      u8 priority;
      u8 size;
      Result (GameMessageHandler::*ProcessPacketAs)(Robot*, const u8*);
    } lookupTable_[NUM_UI_MSG_IDS+1] = {
      {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#define MESSAGE_HANDLER_CLASSNAME GameMessageHandler
#include "anki/cozmo/basestation/game/MessageDefinitionsG2U.h"
#undef MESSAGE_HANDLER_CLASSNAME
      {0, 0, 0} // Final dummy entry without comma at end
    };
    
  }; // class GameMessageHandler
  

  // A stub for testing without a GameMessage handler
  class GameMessageHandlerStub : public IGameMessageHandler
  {
    GameMessageHandlerStub() { }
    
    Result Init(Comms::IComms* comms,
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
    Result SendMessage(const UserDeviceID_t devID, const GameMessage& msg) {
      return RESULT_OK;
    }

  }; // class GameMessageHandlerStub

  
} // namesapce Cozmo
} // namespace Anki


#endif // ANKI_COZMO_GAME_MESSAGE_HANDLER_H
