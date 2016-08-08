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
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/game/comms/iSocketComms.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/uiConnectionTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

// Forward declarations
namespace Anki {
namespace Comms {
  class AdvertisementService;
  class MsgPacket;
}
namespace Util {
  namespace Stats {
    class StatsAccumulator;
  }
}
}

namespace Anki {
  namespace Cozmo {
    

    class Robot;
    class RobotManager;
    class GameMessagePort;
    
    
    class UiMessageHandler : public IExternalInterface
    {
    public:
      
      UiMessageHandler(u32 hostUiDeviceID, GameMessagePort* messagePipe); // Force construction with stuff in Init()?
      virtual ~UiMessageHandler();
      
      Result Init(const Json::Value& config);
      
      Result Update();
      
      virtual void Broadcast(const ExternalInterface::MessageGameToEngine& message) override;
      virtual void Broadcast(ExternalInterface::MessageGameToEngine&& message) override;
      virtual void BroadcastDeferred(const ExternalInterface::MessageGameToEngine& message) override;
      virtual void BroadcastDeferred(ExternalInterface::MessageGameToEngine&& message) override;
      
      virtual void Broadcast(const ExternalInterface::MessageEngineToGame& message) override;
      virtual void Broadcast(ExternalInterface::MessageEngineToGame&& message) override;
      
      virtual Signal::SmartHandle Subscribe(const ExternalInterface::MessageEngineToGameTag& tagType, std::function<void(const AnkiEvent<ExternalInterface::MessageEngineToGame>&)> messageHandler) override;
      
      virtual Signal::SmartHandle Subscribe(const ExternalInterface::MessageGameToEngineTag& tagType, std::function<void(const AnkiEvent<ExternalInterface::MessageGameToEngine>&)> messageHandler) override;
      
      inline u32 GetHostUiDeviceID() const { return _hostUiDeviceID; }
      
      AnkiEventMgr<ExternalInterface::MessageEngineToGame>& GetEventMgrToGame() { return _eventMgrToGame; }
      AnkiEventMgr<ExternalInterface::MessageGameToEngine>& GetEventMgrToEngine() { return _eventMgrToEngine; }
      
      const Util::Stats::StatsAccumulator& GetLatencyStats(UiConnectionType type) const;
      
      bool HasDesiredNumUiDevices() const;

    private:
      
      const ISocketComms* GetSocketComms(UiConnectionType type) const
      {
        const uint32_t typeIndex = (uint32_t)type;
        const bool inRange = (typeIndex < uint32_t(UiConnectionType::Count));
        assert(inRange);
        return inRange ? _socketComms[typeIndex] : nullptr;
      }
      
      ISocketComms* GetSocketComms(UiConnectionType type)
      {
        return const_cast<ISocketComms*>( const_cast<const UiMessageHandler*>(this)->GetSocketComms(type) );
      }
      
      uint32_t GetNumConnectedDevicesOnAnySocket() const;
      
      bool ShouldHandleMessagesFromConnection(UiConnectionType type) const;
      
    protected:
      
      ISocketComms* _socketComms[(size_t)UiConnectionType::Count];

      bool                                          _isInitialized = false;
      u32                                           _hostUiDeviceID = 0;
      
      std::vector<Signal::SmartHandle>              _signalHandles;
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      Result ProcessMessages();
      
      // Process a raw byte buffer as a GameToEngine CLAD message and broadcast it
      Result ProcessMessageBytes(const uint8_t* packetBytes, size_t packetSize,
                                 UiConnectionType connectionType, bool isSingleMessage, bool handleMessagesFromConnection);
      void HandleProcessedMessage(const ExternalInterface::MessageGameToEngine& message, UiConnectionType connectionType, bool handleMessagesFromConnection);
      
      // Send a message to a specified ID
      virtual void DeliverToGame(const ExternalInterface::MessageEngineToGame& message, DestinationId = kDestinationIdEveryone) override;
      
      bool ConnectToUiDevice(ISocketComms::DeviceId deviceId, UiConnectionType connectionType);
      void HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
      
      AnkiEventMgr<ExternalInterface::MessageEngineToGame> _eventMgrToGame;
      AnkiEventMgr<ExternalInterface::MessageGameToEngine> _eventMgrToEngine;
      
      std::vector<ExternalInterface::MessageGameToEngine> _threadedMsgs;
      std::mutex _mutex;
      
      uint32_t   _updateCount = 0;
      
    }; // class MessageHandler
    
    
#undef MESSAGE_BASECLASS_NAME
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H