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
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/uiConnectionTypes.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

// Forward declarations
namespace Anki {
namespace Comms {
  class AdvertisementService;
  class MsgPacket;
}
}

namespace Anki {
  namespace Cozmo {
    
    using AdvertisingUiDevice = int;

    class Robot;
    class RobotManager;
    class MultiClientComms;
    
    
    class SocketComms
    {
    public:
      
      SocketComms();
      ~SocketComms();
      
      void StartAdvertising(UiConnectionType type);
      void Update();

      void AddDevice(AdvertisingUiDevice newDevice)
      {
        _connectedDevices.push_back(newDevice);
      }
      
      uint32_t GetNumConnectedDevices() const { return (uint32_t)_connectedDevices.size(); }
      uint32_t GetNumActiveConnectedDevices() const;
      
      bool HasDesiredDevices() const
      {
        const uint32_t numConnectedDevices = GetNumConnectedDevices();
        return (numConnectedDevices >= _desiredNumDevices) && (numConnectedDevices > 0);
      }
      
      bool HasDesiredActiveDevices() const
      {
        const uint32_t numActiveConnectedDevices = GetNumActiveConnectedDevices();
        return (numActiveConnectedDevices >= _desiredNumDevices) && (numActiveConnectedDevices > 0);
      }
      
      MultiClientComms* GetComms() { return _comms; }
      
      ExternalInterface::Ping& GetPing() { return _pingToSocket; }
      
      void SetDesiredNumDevices(uint32_t newVal) { _desiredNumDevices = newVal; }
      
    private:
      
      MultiClientComms*                 _comms;
      Comms::AdvertisementService*      _advertisementService;
      ExternalInterface::Ping           _pingToSocket;
      std::vector<AdvertisingUiDevice>  _connectedDevices;
      uint32_t                          _desiredNumDevices = 0;
    };

    
    class UiMessageHandler : public IExternalInterface
    {
    public:
      
      UiMessageHandler(u32 hostUiDeviceID); // Force construction with stuff in Init()?
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
      
      bool HasDesiredNumUiDevices() const;

    private:
      
      const SocketComms& GetSocketComms(UiConnectionType type) const
      {
        assert((type >= UiConnectionType(0)) && (type < UiConnectionType::Count));
        return _socketComms[(uint32_t)type];
      }
      
      SocketComms& GetSocketComms(UiConnectionType type)
      {
        assert((type >= UiConnectionType(0)) && (type < UiConnectionType::Count));
        return _socketComms[(uint32_t)type];
      }
      
      uint32_t GetNumConnectedDevicesOnAnySocket() const;
      
    protected:
      
      SocketComms _socketComms[(size_t)UiConnectionType::Count];

      bool                                          _isInitialized = false;
      u32                                           _hostUiDeviceID = 0;
      
      std::vector<Signal::SmartHandle>              _signalHandles;
      
      // As long as there are messages available from the comms object,
      // process them and pass them along to robots.
      Result ProcessMessages();
      
      // Process a raw byte buffer as a message and send it to the specified
      // robot
      Result ProcessPacket(const Comms::MsgPacket& packet);
      
      // Send a message to a specified ID
      virtual void DeliverToGame(const ExternalInterface::MessageEngineToGame& message) override;
      
      bool ConnectToUiDevice(AdvertisingUiDevice whichDevice, UiConnectionType connectionType);
      void HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
      
      AnkiEventMgr<ExternalInterface::MessageEngineToGame> _eventMgrToGame;
      AnkiEventMgr<ExternalInterface::MessageGameToEngine> _eventMgrToEngine;
      
      std::vector<ExternalInterface::MessageGameToEngine> _threadedMsgs;
      std::mutex _mutex;
      
    }; // class MessageHandler
    
    
#undef MESSAGE_BASECLASS_NAME
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_MESSAGEHANDLER_H