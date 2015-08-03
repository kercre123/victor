/**
 * File: uiMessageHandler.h
 *
 * Author: Kevin Yoon
 * Date:   7/11/2014
 *
 * Description: Handles messages between UI and basestation just as 
 *              MessageHandler handles messages between basestation and robot.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_UI_MESSAGEHANDLER_H
#define COZMO_UI_MESSAGEHANDLER_H

#include "anki/common/types.h"
#include <anki/messaging/basestation/IComms.h>

#include "anki/cozmo/basestation/cozmoEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

// Enable this if you want to receive/send messages via socket connection.
// Eventually, this should be disabled by default once the UI layer starts
// handling the comms and communication with the basestation is purely through messageQueue
// TODO: MessageQueue mode not yet supported!!!
//       Will do this after messageDefinitions auto-generation tool has been created.
#define RUN_UI_MESSAGE_TCP_SERVER 1



namespace Anki {
  namespace Cozmo {
    

    class Robot;
    class RobotManager;
    
    class IUiMessageHandler
    {
    public:
      
      // TODO: Change these to interface references so they can be stubbed as well
      virtual Result Init(Comms::IComms*   comms) = 0;
      
      virtual Result ProcessMessages() = 0;
      
      virtual Result SendMessage(const ExternalInterface::MessageEngineToGame& msg) = 0;
      
    }; // IMessageHandler
    
    
    class UiMessageHandler : public IUiMessageHandler, public IExternalInterface
    {
    public:
      
      UiMessageHandler(u32 hostUiDeviceID); // Force construction with stuff in Init()?
      
      // Set the message handler's communications manager
      virtual Result Init(Comms::IComms* comms) override;
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      virtual Result ProcessMessages();
      
      // Send a message to a specified ID
      virtual Result SendMessage(const ExternalInterface::MessageEngineToGame& msg);
      void DeliverToGame(const ExternalInterface::MessageEngineToGame&& message) override;
      
      inline void RegisterCallbackForMessage(const std::function<void(const ExternalInterface::MessageGameToEngine&)>& messageCallback)
      {
        this->messageCallback = messageCallback;
      }
      inline u32 GetHostUiDeviceID() const { return _hostUiDeviceID; }
    protected:
      
      Comms::IComms* comms_;
      
      bool isInitialized_;
      u32 _hostUiDeviceID;
      
      std::function<void(const ExternalInterface::MessageGameToEngine&)> messageCallback;
      
      // Process a raw byte buffer as a message and send it to the specified
      // robot
      Result ProcessPacket(const Comms::MsgPacket& packet);

      
    }; // class MessageHandler
    
    
    class UiMessageHandlerStub : public IUiMessageHandler
    {
    public:
      UiMessageHandlerStub() { }
      
      virtual Result Init(Comms::IComms* comms) override
      {
        return RESULT_OK;
      }
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      virtual Result ProcessMessages() {
        return RESULT_OK;
      }
      
      // Send a message to a specified ID
      virtual Result SendMessage(const ExternalInterface::MessageEngineToGame& msg) {
        return RESULT_OK;
      }
      
    }; // MessageHandlerStub
    
    
#undef MESSAGE_BASECLASS_NAME
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H