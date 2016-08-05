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
#include "anki/cozmo/game/comms/directGameComms.h"
#include "anki/cozmo/game/comms/tcpSocketComms.h"
#include "anki/cozmo/game/comms/udpSocketComms.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"

#include "anki/cozmo/basestation/behaviorManager.h"

#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include <anki/messaging/basestation/IComms.h>
#include <anki/messaging/basestation/advertisementService.h>

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/enums/enumOperators.h"
#include "util/time/universalTime.h"

#if defined(ANDROID) || defined(ANKI_IOS_BUILD)
#define USE_DIRECT_COMMS 1
#else
#define USE_DIRECT_COMMS 0
#endif

namespace Anki {
  namespace Cozmo {
    
    
#if (defined(ANKI_IOS_BUILD) || defined(ANDROID))
  #define ANKI_ENABLE_SDK_OVER_UDP  0
  #define ANKI_ENABLE_SDK_OVER_TCP  1
#else
  #define ANKI_ENABLE_SDK_OVER_UDP  1
  #define ANKI_ENABLE_SDK_OVER_TCP  0
#endif
    
    
    IMPLEMENT_ENUM_INCREMENT_OPERATORS(UiConnectionType);
    
    
    CONSOLE_VAR(bool, kAcceptMessagesFromUI,  "SDK", true);
    CONSOLE_VAR(bool, kAcceptMessagesFromSDK, "SDK", true);
    
    
    bool IsSdkConnection(UiConnectionType type)
    {
      switch(type)
      {
        case UiConnectionType::UI:          return false;
        case UiConnectionType::SdkOverUdp:  return true;
        case UiConnectionType::SdkOverTcp:  return true;
        default:
        {
          assert(0);
          return false;
        }
      }
    }
    
    
    ISocketComms* CreateSocketComms(UiConnectionType type, GameMessagePort* gameMessagePort, ISocketComms::DeviceId hostDeviceId)
    {
      // Note: Some SocketComms are deliberately null depending on the build platform, type etc.

      switch(type)
      {
        case UiConnectionType::UI:
        {
          #if USE_DIRECT_COMMS
          return new DirectGameComms(gameMessagePort, hostDeviceId);
          #else
          return new UdpSocketComms(type);
          #endif
        }
        case UiConnectionType::SdkOverUdp:
        {
        #if ANKI_ENABLE_SDK_OVER_UDP
          return new UdpSocketComms(type);
        #else
          return nullptr;
        #endif
        }
        case UiConnectionType::SdkOverTcp:
        {
        #if ANKI_ENABLE_SDK_OVER_TCP
          return new TcpSocketComms();
        #else
          return nullptr;
        #endif
        }
        default:
        {
          assert(0);
          return nullptr;
        }
      }
    }
    

    UiMessageHandler::UiMessageHandler(u32 hostUiDeviceID, GameMessagePort* gameMessagePort)
      : _isInitialized(false)
      , _hostUiDeviceID(hostUiDeviceID)
    {
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        _socketComms[(uint32_t)i] = CreateSocketComms(i, gameMessagePort, GetHostUiDeviceID());
      }
    }
    
    
    UiMessageHandler::~UiMessageHandler()
    {
      for (uint32_t i=0; i < (uint32_t)UiConnectionType::Count; ++i)
      {
        delete _socketComms[i];
        _socketComms[i] = nullptr;
      }
    }
    
    
    Result UiMessageHandler::Init(const Json::Value& config)
    {
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms)
        {
          if (!socketComms->Init(i, config))
          {
            return RESULT_FAIL;
          }
        }
      }
      
      _isInitialized = true;
      
      // We'll use this callback for simple events we care about
      auto commonCallback = std::bind(&UiMessageHandler::HandleEvents, this, std::placeholders::_1);
      
      // Subscribe to desired events
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice, commonCallback));
      
      return RESULT_OK;
    }
    
    
    bool UiMessageHandler::ShouldHandleMessagesFromConnection(UiConnectionType type) const
    {
      switch(type)
      {
        case UiConnectionType::UI:          return kAcceptMessagesFromUI;
        case UiConnectionType::SdkOverUdp:  return kAcceptMessagesFromSDK;
        case UiConnectionType::SdkOverTcp:  return kAcceptMessagesFromSDK;
        default:
        {
          assert(0);
          return false;
        }
      }
    }
    

    uint32_t UiMessageHandler::GetNumConnectedDevicesOnAnySocket() const
    {
      uint32_t numCommsConnected = 0;
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        const ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms)
        {
          numCommsConnected += socketComms->GetNumConnectedDevices();
        }
      }
      return numCommsConnected;
    }

    
    void UiMessageHandler::DeliverToGame(const ExternalInterface::MessageEngineToGame& message, DestinationId desinationId)
    {
      // There is almost always a connected device, so better to just always pack the message even if it won't be sent
      //if (GetNumConnectedDevicesOnAnySocket() > 0)
      {
        ANKI_CPU_PROFILE("UiMH::DeliverToGame");
        
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

        const bool sendToEveryone = (desinationId == kDestinationIdEveryone);
        UiConnectionType connectionType = sendToEveryone ? UiConnectionType(0) : (UiConnectionType)desinationId;
        if (connectionType >= UiConnectionType::Count)
        {
          PRINT_NAMED_WARNING("UiMessageHandler.DeliverToGame.BadDestinationId", "Invalid destinationId %u = UiConnectionType '%s'", desinationId, EnumToString(connectionType));
        }
        
        while (connectionType < UiConnectionType::Count)
        {
          ISocketComms* socketComms = GetSocketComms(connectionType);
          if (socketComms)
          {
            socketComms->SendMessage(p);
          }
          
          if (sendToEveryone)
          {
            ++connectionType;
          }
          else
          {
            return;
          }
        }
      }
    }
 
    
    Result UiMessageHandler::ProcessMessageBytes(const uint8_t* const packetBytes, const size_t packetSize,
                                                 UiConnectionType connectionType, bool isSingleMessage, bool handleMessagesFromConnection)
    {
      ANKI_CPU_PROFILE("UiMH::ProcessMessageBytes");
      
      ExternalInterface::MessageGameToEngine message;
      uint16_t bytesRemaining = packetSize;
      const uint8_t* messagePtr = packetBytes;

      while (bytesRemaining > 0)
      {
        size_t bytesUnpacked = message.Unpack(messagePtr, bytesRemaining);
        if (isSingleMessage && bytesUnpacked != packetSize)
        {
          PRINT_STREAM_ERROR("UiMessageHandler.MessageBufferWrongSize",
                             "Buffer's size does not match expected size for this message ID. (Msg "
                              << ExternalInterface::MessageGameToEngineTagToString(message.GetTag())
                              << ", expected " << message.Size() << ", recvd " << packetSize << ")" );
          return RESULT_FAIL;
        }
        else if (!isSingleMessage && (bytesUnpacked > bytesRemaining || bytesUnpacked == 0))
        {
          PRINT_STREAM_ERROR("UiMessageHandler.MessageBufferWrongSize",
                            "Buffer overrun reading messages, last message: "
                             << ExternalInterface::MessageGameToEngineTagToString(message.GetTag()));

          return RESULT_FAIL;
        }
        bytesRemaining -= bytesUnpacked;
        messagePtr += bytesUnpacked;

        HandleProcessedMessage(message, connectionType, handleMessagesFromConnection);
      }

      return RESULT_OK;
    }
    
    
    bool AlwaysHandleMessageTypeForConnection(ExternalInterface::MessageGameToEngine::Tag messageTag)
    {
      // Return true for small subset of message types that we handle even if we're not listening to that connection
      // We still want to accept certain message types (e.g. console vars to allow a connection to enable itself)
      
      using GameToEngineTag = ExternalInterface::MessageGameToEngineTag;
      switch (messageTag)
      {
        case GameToEngineTag::SetDebugConsoleVarMessage:    return true;
        case GameToEngineTag::GetDebugConsoleVarMessage:    return true;
        case GameToEngineTag::GetAllDebugConsoleVarMessage: return true;
        default:
          return false;
      }
    }

    
    void UiMessageHandler::HandleProcessedMessage(const ExternalInterface::MessageGameToEngine& message,
                                                  UiConnectionType connectionType, bool handleMessagesFromConnection)
    {
      const ExternalInterface::MessageGameToEngine::Tag messageTag = message.GetTag();
      if (!handleMessagesFromConnection)
      {
        // We still want to accept certain message types (e.g. console vars to allow a connection to enable itself)
        if (!AlwaysHandleMessageTypeForConnection(messageTag))
        {
          return;
        }
      }
      
      #if ANKI_DEV_CHEATS
      if (nullptr != DevLoggingSystem::GetInstance())
      {
        DevLoggingSystem::GetInstance()->LogMessage(message);
      }
      #endif
      // We must handle pings at this level because they are a connection type specific message
      // and must be dealt with at the transport level rather than at the app level
      if (messageTag == ExternalInterface::MessageGameToEngineTag::Ping)
      {
        ISocketComms* socketComms = GetSocketComms(connectionType);
        if (socketComms)
        {
          const ExternalInterface::Ping& pingMsg = message.Get_Ping();
          if (pingMsg.isResponse)
          {
            socketComms->HandlePingResponse(pingMsg);
          }
          else
          {
            ExternalInterface::Ping outPing(pingMsg.counter, pingMsg.timeSent_ms, true);
            ExternalInterface::MessageEngineToGame toSend;
            toSend.Set_Ping(outPing);
            DeliverToGame(std::move(toSend), (DestinationId)connectionType);
          }
        }
      }
      else
      {
        // Send out this message to anyone that's subscribed
        Broadcast(message);
      }
    }

    // Broadcasting MessageGameToEngine messages are only internal
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageGameToEngine& message)
    {
      ANKI_CPU_PROFILE("UiMH::Broadcast_GToE"); // Some expensive and untracked - TODO: Capture message type for profile
      
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageGameToEngine)
    
    
    void UiMessageHandler::Broadcast(ExternalInterface::MessageGameToEngine&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastMove_GToE");
      
      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageGameToEngine &&)
    
    
    // Called from any not main thread and dealt with during the update.
    void UiMessageHandler::BroadcastDeferred(const ExternalInterface::MessageGameToEngine& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastDeferred_GToE");
      
      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgs.emplace_back(message);
    }
    
    
    void UiMessageHandler::BroadcastDeferred(ExternalInterface::MessageGameToEngine&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastDeferredMove_GToE");
      
      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgs.emplace_back(std::move(message));
    }
    
    
    // Broadcasting MessageEngineToGame messages also delivers them out of the engine
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageEngineToGame& message)
    {
      ANKI_CPU_PROFILE("UiMH::Broadcast_EToG");
      
      DeliverToGame(message);
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageEngineToGame)
    
    
    void UiMessageHandler::Broadcast(ExternalInterface::MessageEngineToGame&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastMove_EToG");
      
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
      ANKI_CPU_PROFILE("UiMH::ProcessMessages");
      
      Result retVal = RESULT_FAIL;
      
      if(_isInitialized)
      {
        retVal = RESULT_OK;

        for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
        {
          ISocketComms* socketComms = GetSocketComms(i);
          if (socketComms)
          {
            bool keepReadingMessages = true;
            const bool isSingleMessage = !socketComms->AreMessagesGrouped();
            const bool handleMessagesFromConnection = ShouldHandleMessagesFromConnection(i);
            while(keepReadingMessages)
            {
              std::vector<uint8_t> buffer;
              keepReadingMessages = socketComms->RecvMessage(buffer);

              if (keepReadingMessages)
              {
                Result res = ProcessMessageBytes(buffer.data(), buffer.size(), i, isSingleMessage, handleMessagesFromConnection);
                if (res != RESULT_OK)
                {
                  retVal = RESULT_FAIL;
                }
              }
            }
          }
        }
        
      }
      
      return retVal;
    } // ProcessMessages()
    
    
    Result UiMessageHandler::Update()
    {
      ANKI_CPU_PROFILE("UiMH::Update");
      
      // Update all the comms
      
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms)
        {
          socketComms->Update();
          
          if(socketComms->GetNumConnectedDevices() > 0)
          {
            // Ping the connection to let them know we're still here
            // TODO - throttle so it only sends every few ticks or ms
            
            ANKI_CPU_PROFILE("UiMH::Update::SendPing");
            
            ExternalInterface::Ping outPing( socketComms->NextPingCounter(),
                                            Util::Time::UniversalTime::GetCurrentTimeInMilliseconds(), false );
            ExternalInterface::MessageEngineToGame message(std::move(outPing));
            DeliverToGame(message, (DestinationId)i);
          }
        }
      }
      
      // Read messages from all the comms
      
      Result lastResult = ProcessMessages();
      if (RESULT_OK != lastResult)
      {
        return lastResult;
      }
      
      // Send to all of the comms
      
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms)
        {
          std::vector<ISocketComms::DeviceId> advertisingUiDevices;
          socketComms->GetAdvertisingDeviceIDs(advertisingUiDevices);
          for (ISocketComms::DeviceId deviceId : advertisingUiDevices)
          {
            if (deviceId == GetHostUiDeviceID())
            {
              // Force connection to first (local) UI device
              if (ConnectToUiDevice(deviceId, i))
              {
                PRINT_NAMED_INFO("UiMessageHandler.Update.Connected",
                                 "Automatically connected to local %s device %d!", EnumToString(i), deviceId);
              }
              else
              {
                PRINT_NAMED_WARNING("UiMessageHandler.Update.FailedToConnect",
                                 "Failed to connected to local %s device %d!", EnumToString(i), deviceId);
              }
            }
            else
            {
              Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceAvailable(i, deviceId)));
            }
          }
        }
      }
      
      {
        ANKI_CPU_PROFILE("UiMH::BroadcastThreadedMessages");
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

    
    bool UiMessageHandler::ConnectToUiDevice(ISocketComms::DeviceId deviceId, UiConnectionType connectionType)
    {
      ISocketComms* socketComms = GetSocketComms(connectionType);
      
      const bool success = socketComms ? socketComms->ConnectToDeviceByID(deviceId) : false;
      Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceConnected(connectionType, deviceId, success)));
    
      return success;
    }
    
    
    void UiMessageHandler::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      switch (event.GetData().GetTag())
      {
        case ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice:
        {
          const ExternalInterface::ConnectToUiDevice& msg = event.GetData().Get_ConnectToUiDevice();
          const ISocketComms::DeviceId deviceID = msg.deviceID;
          
          const bool success = ConnectToUiDevice(deviceID, msg.connectionType);
          if(success) {
            PRINT_NAMED_INFO("UiMessageHandler.HandleEvents", "Connected to %s device %d!",
                             EnumToString(msg.connectionType), deviceID);
          } else {
            PRINT_NAMED_ERROR("UiMessageHandler.HandleEvents", "Failed to connect to %s device %d!",
                              EnumToString(msg.connectionType), deviceID);
          }
          
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice:
        {
          const ExternalInterface::DisconnectFromUiDevice& msg = event.GetData().Get_DisconnectFromUiDevice();
          
          ISocketComms* socketComms = GetSocketComms(msg.connectionType);
          const ISocketComms::DeviceId deviceId = msg.deviceID;

          if (socketComms && socketComms->DisconnectDeviceByID(deviceId))
          {
            PRINT_NAMED_INFO("UiMessageHandler.ProcessMessage", "Disconnected from %s device %d!",
                             EnumToString(msg.connectionType), deviceId);
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
        const ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms && socketComms->HasDesiredDevices())
        {
          return true;
        }
      }
      
      return false;
    }
    
    const Util::Stats::StatsAccumulator& UiMessageHandler::GetLatencyStats(UiConnectionType type) const
    {
      const ISocketComms* socketComms = GetSocketComms(type);
      if (socketComms)
      {
        return socketComms->GetLatencyStats();
      }
      else
      {
        static Util::Stats::StatsAccumulator sDummyStats;
        return sDummyStats;
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
