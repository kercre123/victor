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
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "util/signals/simpleSignal_fwd.h"



namespace Anki {
  namespace Cozmo {
    
    using AdvertisingUiDevice = int;

    class Robot;
    class RobotManager;
    class MultiClientComms;
    
    class UiMessageHandler : public IExternalInterface
    {
    public:
      
      UiMessageHandler(u32 hostUiDeviceID); // Force construction with stuff in Init()?
      
      Result Init(const Json::Value& config);
      
      Result Update();
      
      virtual void Broadcast(const ExternalInterface::MessageGameToEngine& message) override;
      virtual void Broadcast(ExternalInterface::MessageGameToEngine&& message) override;
      
      virtual void Broadcast(const ExternalInterface::MessageEngineToGame& message) override;
      virtual void Broadcast(ExternalInterface::MessageEngineToGame&& message) override;
      
      virtual Signal::SmartHandle Subscribe(const ExternalInterface::MessageEngineToGameTag& tagType, std::function<void(const AnkiEvent<ExternalInterface::MessageEngineToGame>&)> messageHandler) override;
      
      virtual Signal::SmartHandle Subscribe(const ExternalInterface::MessageGameToEngineTag& tagType, std::function<void(const AnkiEvent<ExternalInterface::MessageGameToEngine>&)> messageHandler) override;
      
      inline u32 GetHostUiDeviceID() const { return _hostUiDeviceID; }
      
      AnkiEventMgr<ExternalInterface::MessageEngineToGame>& GetEventMgrToGame() { return _eventMgrToGame; }
      AnkiEventMgr<ExternalInterface::MessageGameToEngine>& GetEventMgrToEngine() { return _eventMgrToEngine; }
      
      bool HasDesiredNumUiDevices() const { return _hasDesiredUiDevices; }
      
    protected:
      
      MultiClientComms*                       _uiComms;
      Comms::AdvertisementService             _uiAdvertisementService;
      
      bool                                    isInitialized_ = false;
      u32                                     _hostUiDeviceID = 0;
      int                                     _desiredNumUiDevices = 0;
      bool                                    _hasDesiredUiDevices = false;
      ExternalInterface::Ping                 _pingToUI = {};
      std::vector<Signal::SmartHandle>        _signalHandles;
      std::vector<AdvertisingUiDevice>        _connectedUiDevices;
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      Result ProcessMessages();
      
      // Process a raw byte buffer as a message and send it to the specified
      // robot
      Result ProcessPacket(const Comms::MsgPacket& packet);
      
      // Send a message to a specified ID
      virtual void DeliverToGame(const ExternalInterface::MessageEngineToGame& message) override;
      
      bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
      void HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
      
      AnkiEventMgr<ExternalInterface::MessageEngineToGame> _eventMgrToGame;
      AnkiEventMgr<ExternalInterface::MessageGameToEngine> _eventMgrToEngine;
      
    }; // class MessageHandler
    
    
#undef MESSAGE_BASECLASS_NAME
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H