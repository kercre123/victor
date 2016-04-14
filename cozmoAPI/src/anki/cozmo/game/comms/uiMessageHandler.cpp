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
#include "util/global/globalDefinitions.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
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

#include "util/enums/enumOperators.h"

namespace Anki {
  namespace Cozmo {
    
    
    IMPLEMENT_ENUM_INCREMENT_OPERATORS(UiConnectionType);
    
    
    SocketComms::SocketComms()
      : _comms(new MultiClientComms())
      , _advertisementService(nullptr)
    {
    }
    
    
    SocketComms::~SocketComms()
    {
      delete(_comms);
      delete(_advertisementService);
    }
    
    
    void SocketComms::StartAdvertising(UiConnectionType type)
    {
      const char* socketTypeName = EnumToString(type);
      std::string serviceName = std::string() + "AdvertisementService";
      assert(_advertisementService == nullptr);
      delete(_advertisementService);
      _advertisementService = new Comms::AdvertisementService(serviceName.c_str());

      int registrationPort = (type == UiConnectionType::UI) ? UI_ADVERTISEMENT_REGISTRATION_PORT : SDK_ADVERTISEMENT_REGISTRATION_PORT;
      int advertisingPort  = (type == UiConnectionType::UI) ? UI_ADVERTISING_PORT : SDK_ADVERTISING_PORT;
      
      PRINT_NAMED_INFO("SocketComms::StartAdvertising",
                       "Starting %sAdvertisementService, reg port %d, ad port %d",
                       socketTypeName, registrationPort, advertisingPort);
      
      _advertisementService->StartService(registrationPort, advertisingPort);
    }
    
    void SocketComms::Update()
    {
      _advertisementService->Update();
    }
    
    uint32_t SocketComms::GetNumActiveConnectedDevices() const
    {
      uint32_t numActive = 0;
      for (const AdvertisingUiDevice& device : _connectedDevices)
      {
        // [MARKW-TODO] Track if device is still active (e.g. activity in last N seconds)
        (void)device;
        //if (device is active)
        {
          ++numActive;
        }
      }
      
      return numActive;
    }
    

    UiMessageHandler::UiMessageHandler(u32 hostUiDeviceID)
      : _isInitialized(false)
      , _hostUiDeviceID(hostUiDeviceID)
    {
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        SocketComms& comms = GetSocketComms(i);
        comms.StartAdvertising(i);
      }
    }
    
    // This needs to be defined in the cpp so that the full definition of unique_ptr types is available for destruction
    UiMessageHandler::~UiMessageHandler() = default;
    
    Result UiMessageHandler::Init(const Json::Value& config)
    {
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        SocketComms& comms = GetSocketComms(i);

        using namespace AnkiUtil;
        const char* hostIPKey     = (i==UiConnectionType::UI) ? kP_ADVERTISING_HOST_IP : kP_SDK_ADVERTISING_HOST_IP;
        const char* advertPortKey = (i==UiConnectionType::UI) ? kP_UI_ADVERTISING_PORT : kP_SDK_ADVERTISING_PORT;
        
        const Json::Value& hostIPValue     = config[hostIPKey];
        const Json::Value& advertPortValue = config[advertPortKey];
        
        if (hostIPValue.isString() && advertPortValue.isNumeric())
        {
          Result retVal = comms.GetComms()->Init(hostIPValue.asCString(), advertPortValue.asInt());
          
          if(retVal != RESULT_OK) {
            PRINT_NAMED_ERROR("UiMessageHandler.Init.InitComms", "Failed to initialize %s Comms.", EnumToString(i));
            return retVal;
          }
        }
        else
        {
          if (i==UiConnectionType::UI)
          {
            PRINT_NAMED_ERROR("UiMessageHandler.Init.MissingSettings", "Missing advertising host IP / UI advertising port in Json config file.");
            return RESULT_FAIL;
          }
        }
        
        const char* numDevicesKey = (i==UiConnectionType::UI) ? kP_NUM_UI_DEVICES_TO_WAIT_FOR : kP_NUM_SDK_DEVICES_TO_WAIT_FOR;

        int desiredNumUiDevices = 0;
        if(!config.isMember(numDevicesKey)) {
          desiredNumUiDevices = 1;
          PRINT_NAMED_WARNING("CozmoGameImpl.Init", "No %s defined in Json config, defaulting to %d", numDevicesKey, desiredNumUiDevices);
        } else {
          desiredNumUiDevices = config[numDevicesKey].asInt();
        }
        
        comms.SetDesiredNumDevices(desiredNumUiDevices);
      }
      
      _isInitialized = true;
      
      // We'll use this callback for simple events we care about
      auto commonCallback = std::bind(&UiMessageHandler::HandleEvents, this, std::placeholders::_1);
      
      // Subscribe to desired events
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice, commonCallback));
      
      return RESULT_OK;
    }

    uint32_t UiMessageHandler::GetNumConnectedDevicesOnAnySocket() const
    {
      uint32_t numCommsConnected = 0;
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        const SocketComms& socketComms = GetSocketComms(i);
        numCommsConnected += socketComms.GetNumConnectedDevices();
      }
      return numCommsConnected;
    }

    void UiMessageHandler::DeliverToGame(const ExternalInterface::MessageEngineToGame& message)
    {
      if (GetNumConnectedDevicesOnAnySocket() > 0)
      {
        Comms::MsgPacket p;
        message.Pack(p.data, Comms::MsgPacket::MAX_SIZE);
        
#if ANKI_DEV_CHEATS
        if (nullptr != DevLoggingSystem::GetInstance())
        {
          DevLoggingSystem::GetInstance()->LogMessage(message);
        }
#endif
        
        p.dataLen = message.Size();
        p.destId = _hostUiDeviceID;

        for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
        {
          SocketComms& socketComms = GetSocketComms(i);
          if (socketComms.GetComms()->GetNumConnectedDevices() > 0)
          {
            socketComms.GetComms()->Send(p);
          }
        }
      }
    }
  
    Result UiMessageHandler::ProcessPacket(const Comms::MsgPacket& packet)
    {
      ExternalInterface::MessageGameToEngine message;
      if (message.Unpack(packet.data, packet.dataLen) != packet.dataLen) {
        PRINT_STREAM_ERROR("UiMessageHandler.MessageBufferWrongSize",
                          "Buffer's size does not match expected size for this message ID. (Msg " << ExternalInterface::MessageGameToEngineTagToString(message.GetTag()) << ", expected " << message.Size() << ", recvd " << packet.dataLen << ")");
        
        return RESULT_FAIL;
      }
      
#if ANKI_DEV_CHEATS
      if (nullptr != DevLoggingSystem::GetInstance())
      {
        DevLoggingSystem::GetInstance()->LogMessage(message);
      }
#endif
      
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
    
    // Called from any not main thread and dealt with during the update.
    void UiMessageHandler::BroadcastDeferred(const ExternalInterface::MessageGameToEngine& message)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgs.emplace_back(message);
    }
    void UiMessageHandler::BroadcastDeferred(ExternalInterface::MessageGameToEngine&& message)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgs.emplace_back(std::move(message));
    }
    
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
      return _eventMgrToGame.Subscribe(static_cast<u32>(tagType), messageHandler);
    } // Subscribe(MessageEngineToGame)
    
    Signal::SmartHandle UiMessageHandler::Subscribe(const ExternalInterface::MessageGameToEngineTag& tagType,
                                                    std::function<void(const AnkiEvent<ExternalInterface::MessageGameToEngine>&)> messageHandler)
    {
      return _eventMgrToEngine.Subscribe(static_cast<u32>(tagType), messageHandler);
    } // Subscribe(MessageGameToEngine)

    
    Result UiMessageHandler::ProcessMessages()
    {
      Result retVal = RESULT_FAIL;
      
      if(_isInitialized) {
        retVal = RESULT_OK;

        for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
        {
          SocketComms& socketComms = GetSocketComms(i);

          while(socketComms.GetComms()->GetNumPendingMsgPackets() > 0)
          {
            Comms::MsgPacket packet;
            bool res = socketComms.GetComms()->GetNextMsgPacket(packet);
            
            if(res && (ProcessPacket(packet) != RESULT_OK)) {
              retVal = RESULT_FAIL;
            }
          } // while messages are still available from comms
        }
      }
      
      return retVal;
    } // ProcessMessages()
    
    
    Result UiMessageHandler::Update()
    {
      // Update comms
      
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        SocketComms& socketComms = GetSocketComms(i);

        if(socketComms.GetComms()->IsInitialized())
        {
          socketComms.GetComms()->Update();
          
          if(socketComms.GetComms()->GetNumConnectedDevices() > 0)
          {
            // Ping the UI to let them know we're still here
            ExternalInterface::MessageEngineToGame message;
            message.Set_Ping(socketComms.GetPing());
            Broadcast(message);
            ++socketComms.GetPing().counter;
          }
        }
      }
      
      Result lastResult = ProcessMessages();
      if (RESULT_OK != lastResult)
      {
        return lastResult;
      }
      
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        SocketComms& socketComms = GetSocketComms(i);

        if (!socketComms.HasDesiredActiveDevices())
        {
          socketComms.Update();
          
          std::vector<int> advertisingUiDevices;
          socketComms.GetComms()->GetAdvertisingDeviceIDs(advertisingUiDevices);
          for(auto & device : advertisingUiDevices) {
            if(device == GetHostUiDeviceID()) {
              // Force connection to first (local) UI device
              if(true == ConnectToUiDevice(device, i)) {
                PRINT_NAMED_INFO("CozmoGameImpl.Update",
                                 "Automatically connected to local UI device %d!", device);
              }
            } else {
              Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceAvailable(i, device)));
            }
          }
        }
      }
      
      {
        std::lock_guard<std::mutex> lock(_mutex);
        if( _threadedMsgs.size() > 0 )
        {
          for(auto& threaded_msg : _threadedMsgs) {
            Broadcast(std::move(threaded_msg));
          }
          _threadedMsgs.clear();
        }
      }
      
      return lastResult;
    } // Update()
    
    bool UiMessageHandler::ConnectToUiDevice(AdvertisingUiDevice whichDevice, UiConnectionType connectionType)
    {
      SocketComms& socketComms = GetSocketComms(connectionType);
      
      const bool success = socketComms.GetComms()->ConnectToDeviceByID(whichDevice);
      if(success) {
        socketComms.AddDevice(whichDevice);
      }

      Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceConnected(connectionType, whichDevice, success)));

      return success;
    }
    
    void UiMessageHandler::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      switch (event.GetData().GetTag())
      {
        case ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice:
        {
          const ExternalInterface::ConnectToUiDevice& msg = event.GetData().Get_ConnectToUiDevice();
          
          const bool success = ConnectToUiDevice(msg.deviceID, msg.connectionType);
          if(success) {
            PRINT_NAMED_INFO("UiMessageHandler.HandleEvents", "Connected to %s device %d!",
                             EnumToString(msg.connectionType), msg.deviceID);
          } else {
            PRINT_NAMED_ERROR("UiMessageHandler.HandleEvents", "Failed to connect to %s device %d!",
                              EnumToString(msg.connectionType), msg.deviceID);
          }
          
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice:
        {
          const ExternalInterface::DisconnectFromUiDevice& msg = event.GetData().Get_DisconnectFromUiDevice();
          
          SocketComms& socketComms = GetSocketComms(msg.connectionType);

          if (socketComms.GetComms()->DisconnectDeviceByID(msg.deviceID))
          {
            PRINT_NAMED_INFO("UiMessageHandler.ProcessMessage", "Disconnected from %s device %d!", EnumToString(msg.connectionType), msg.deviceID);

            if(socketComms.GetComms()->GetNumConnectedDevices() == 0)
            {
              //PRINT_NAMED_INFO("UiMessageHandler.ProcessMessage",
              //                 "Last UI device just disconnected: forcing re-initialization.");
              // LeeC TODO: Should we be doing this init anymore? It kind of breaks things, and I'm not sure it reflects
              // the flow we should be doing now. It was also done in the game and not here, before this got moved
              //Init(_config);
            }
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
    
    bool UiMessageHandler::HasDesiredNumUiDevices() const
    {
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        const SocketComms& socketComms = GetSocketComms(i);
        if (socketComms.HasDesiredDevices())
        {
          return true;
        }
      }
      
      return false;
    }
    
  } // namespace Cozmo
} // namespace Anki
