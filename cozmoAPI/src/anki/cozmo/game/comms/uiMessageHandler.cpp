/**
 * File: uiMessageHandler.cpp
 *
 * Author: Kevin Yoon
 * Date:   7/11/2014
 *
 * Description: Handles messages between UI and basestation just as
 *              RobotMessageHandler handles messages between basestation and robot.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "util/logging/logging.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/multiClientComms.h"

#include "anki/cozmo/basestation/behaviorManager.h"

#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include <anki/messaging/basestation/IComms.h>
#include <anki/messaging/basestation/advertisementService.h>

namespace Anki {
  namespace Cozmo {

    UiMessageHandler::UiMessageHandler(u32 hostUiDeviceID)
    : _uiComms(new MultiClientComms{})
    , _uiAdvertisementService(new Comms::AdvertisementService("UIAdvertisementService"))
    , _isInitialized(false)
    , _hostUiDeviceID(hostUiDeviceID)
    {
      PRINT_NAMED_INFO("UiMessageHandler.Constructor",
                       "Starting UIAdvertisementService, reg port %d, ad port %d",
                       UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
      
      _uiAdvertisementService->StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                           UI_ADVERTISING_PORT);
    }
    
    // This needs to be defined in the cpp so that the full definition of unique_ptr types is available for destruction
    UiMessageHandler::~UiMessageHandler() = default;
    
    Result UiMessageHandler::Init(const Json::Value& config)
    {
      if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP) ||
         !config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
        
        PRINT_NAMED_ERROR("UiMessageHandler.Init", "Missing advertising hosdt / UI advertising port in Json config file.");
        return RESULT_FAIL;
      }
      
      Result retVal = _uiComms->Init(config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                     config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());
      
      if(retVal != RESULT_OK) {
        PRINT_NAMED_ERROR("UiMessageHandler.Init.Init", "Failed to initialize host uiComms.");
        return retVal;
      }
      
      if(!config.isMember(AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR)) {
        PRINT_NAMED_WARNING("CozmoGameImpl.Init", "No NumUiDevicesToWaitFor defined in Json config, defaulting to 1.");
        _desiredNumUiDevices = 1;
      } else {
        _desiredNumUiDevices = config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR].asInt();
      }
      
      _isInitialized = true;
      
      // We'll use this callback for simple events we care about
      auto commonCallback = std::bind(&UiMessageHandler::HandleEvents, this, std::placeholders::_1);
      
      // Subscribe to desired events
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice, commonCallback));
      
      return retVal;
    }

    void UiMessageHandler::DeliverToGame(const ExternalInterface::MessageEngineToGame& message)
    {
      Comms::MsgPacket p;
      message.Pack(p.data, Comms::MsgPacket::MAX_SIZE);
      p.dataLen = message.Size();
      p.destId = _hostUiDeviceID;
      _uiComms->Send(p);
    }
  
    Result UiMessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      ExternalInterface::MessageGameToEngine message;
      if (message.Unpack(packet.data, packet.dataLen) != packet.dataLen) {
        PRINT_STREAM_ERROR("UiMessageHandler.MessageBufferWrongSize",
                          "Buffer's size does not match expected size for this message ID. (Msg " << ExternalInterface::MessageGameToEngineTagToString(message.GetTag()) << ", expected " << message.Size() << ", recvd " << packet.dataLen << ")");
        
        return RESULT_FAIL;
      }
      
      // Send out this message to anyone that's subscribed
      Broadcast(std::move(message));
      
      return RESULT_OK;
    } // ProcessBuffer()
    
    // Broadcasting MessageGameToEngine messages are only internal
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageGameToEngine& message)
    {
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageGameToEngine)
    
    void UiMessageHandler::Broadcast(ExternalInterface::MessageGameToEngine&& message)
    {
      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageGameToEngine &&)
    
    
    // Broadcasting MessageEngineToGame messages also delivers them out of the engine
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageEngineToGame& message)
    {
      DeliverToGame(message);
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageEngineToGame)
    
    void UiMessageHandler::Broadcast(ExternalInterface::MessageEngineToGame&& message)
    {
      DeliverToGame(message);
      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageEngineToGame &&)
    
    // Provides a way to subscribe to message types using the AnkiEventMgrs
    Signal::SmartHandle UiMessageHandler::Subscribe(const ExternalInterface::MessageEngineToGameTag& tagType,
                                                    std::function<void(const AnkiEvent<ExternalInterface::MessageEngineToGame>&)> messageHandler)
    {
      return _eventMgrToGame.Subcribe(static_cast<u32>(tagType), messageHandler);
    } // Subscribe(MessageEngineToGame)
    
    Signal::SmartHandle UiMessageHandler::Subscribe(const ExternalInterface::MessageGameToEngineTag& tagType,
                                                    std::function<void(const AnkiEvent<ExternalInterface::MessageGameToEngine>&)> messageHandler)
    {
      return _eventMgrToEngine.Subcribe(static_cast<u32>(tagType), messageHandler);
    } // Subscribe(MessageGameToEngine)

    
    Result UiMessageHandler::ProcessMessages()
    {
      Result retVal = RESULT_FAIL;
      
      if(_isInitialized) {
        retVal = RESULT_OK;
        
        while(_uiComms->GetNumPendingMsgPackets() > 0)
        {
          Comms::MsgPacket packet;
          _uiComms->GetNextMsgPacket(packet);
          
          if(ProcessPacket(packet) != RESULT_OK) {
            retVal = RESULT_FAIL;
          }
        } // while messages are still available from comms
      }
      
      return retVal;
    } // ProcessMessages()
    
    
    Result UiMessageHandler::Update()
    {
      // Update UI comms
      if(_uiComms->IsInitialized()) {
        _uiComms->Update();
        
        if(_uiComms->GetNumConnectedDevices() > 0) {
          // Ping the UI to let them know we're still here
          ExternalInterface::MessageEngineToGame message;
          message.Set_Ping(_pingToUI);
          Broadcast(message);
          ++_pingToUI.counter;
        }
      }
      
      Result lastResult = ProcessMessages();
      if (RESULT_OK != lastResult)
      {
        return lastResult;
      }
      
      if (!_hasDesiredUiDevices)
      {
        _uiAdvertisementService->Update();
        
        // TODO: Do we want to do this all the time in case UI devices want to join later?
        // Notify the UI that there are advertising devices
        std::vector<int> advertisingUiDevices;
        _uiComms->GetAdvertisingDeviceIDs(advertisingUiDevices);
        for(auto & device : advertisingUiDevices) {
          if(device == GetHostUiDeviceID()) {
            // Force connection to first (local) UI device
            if(true == ConnectToUiDevice(device)) {
              PRINT_NAMED_INFO("CozmoGameImpl.Update",
                               "Automatically connected t o local UI device %d!", device);
            }
          } else {
            Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceAvailable(device)));
          }
        }
      }
      return lastResult;
    } // Update()
    
    bool UiMessageHandler::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
    {
      const bool success = _uiComms->ConnectToDeviceByID(whichDevice);
      if(success) {
        _connectedUiDevices.push_back(whichDevice);
        if (_connectedUiDevices.size() >= _desiredNumUiDevices)
        {
          _hasDesiredUiDevices = true;
        }
      }
      Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceConnected(whichDevice, success)));
      return success;
    }
    
    void UiMessageHandler::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      switch (event.GetData().GetTag())
      {
        case ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice:
        {
          const ExternalInterface::ConnectToUiDevice& msg = event.GetData().Get_ConnectToUiDevice();
          const bool success = ConnectToUiDevice(msg.deviceID);
          if(success) {
            PRINT_NAMED_INFO("UiMessageHandler.HandleEvents", "Connected to UI device %d!", msg.deviceID);
          } else {
            PRINT_NAMED_ERROR("UiMessageHandler.HandleEvents", "Failed to connect to UI device %d!", msg.deviceID);
          }
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice:
        {
          const ExternalInterface::DisconnectFromUiDevice& msg = event.GetData().Get_DisconnectFromUiDevice();
          _uiComms->DisconnectDeviceByID(msg.deviceID);
          PRINT_NAMED_INFO("UiMessageHandler.ProcessMessage", "Disconnected from UI device %d!", msg.deviceID);
          
          if(_uiComms->GetNumConnectedDevices() == 0) {
            //PRINT_NAMED_INFO("UiMessageHandler.ProcessMessage",
            //                 "Last UI device just disconnected: forcing re-initialization.");
            // LeeC TODO: Should we be doing this init anymore? It kind of breaks things, and I'm not sure it reflects
            // the flow we should be doing now. It was also done in the game and not here, before this got moved
            //Init(_config);
          }
          break;
        }
        default:
        {
          PRINT_STREAM_ERROR("UiMessageHandler.HandleEvents",
                             "Subscribed to unhandled event of type "
                             << ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) << "!");
        }
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
