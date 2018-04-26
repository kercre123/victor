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

#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/debug/devLoggingSystem.h"
#include "engine/robot.h"
#include "engine/cozmoAPI/comms/directGameComms.h"
#include "engine/cozmoAPI/comms/tcpSocketComms.h"
#include "engine/cozmoAPI/comms/udpSocketComms.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/messaging/advertisementService.h"


#include "engine/viz/vizManager.h"
#include "engine/buildVersion.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/messaging/engine/IComms.h"

#include "clad/externalInterface/messageGameToEngine_hash.h"
#include "clad/externalInterface/messageEngineToGame_hash.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/enums/enumOperators.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"
#include "util/time/universalTime.h"

#ifdef SIMULATOR
#include "osState/osState.h"
#endif

#define USE_DIRECT_COMMS 0

// The amount of time that the UI must have not been
// returning pings before we consider it disconnected
#ifdef SIMULATOR
// No timeout in sim
static const u32 kPingTimeoutForDisconnect_ms = 0;
#else
static const u32 kPingTimeoutForDisconnect_ms = 5000;
#endif

namespace Anki {
  namespace Cozmo {


#if (defined(ANKI_PLATFORM_IOS) || defined(ANKI_PLATFORM_ANDROID))
  #define ANKI_ENABLE_SDK_OVER_UDP  0
  #if defined(NDEBUG)
    CONSOLE_VAR(bool, kEnableSdkCommsInInternalSdk,  "Sdk", false);
  #else
    CONSOLE_VAR(bool, kEnableSdkCommsInInternalSdk, "Sdk", true);
  #endif
  // TODO: This variable may not make sense in a world where all
  // communication is through an SDK interface.
  CONSOLE_VAR(bool, kEnableSdkCommsAlways,  "Sdk", true);
#else
  #define ANKI_ENABLE_SDK_OVER_UDP  0
  CONSOLE_VAR(bool, kEnableSdkCommsAlways,  "Sdk", true);
  CONSOLE_VAR(bool, kEnableSdkCommsInInternalSdk, "Sdk", true);
#endif

// see https://ankiinc.atlassian.net/browse/VIC-1544
//#if defined(NDEBUG)
//    static_assert(!kEnableSdkCommsAlways, "Must be const and false - we cannot leave the socket open outside of sdk for released builds!");
//    static_assert(!kEnableSdkCommsInInternalSdk, "Must be const and false - we cannot leave the socket open outside of sdk for released builds!");
//#endif

CONSOLE_VAR(bool, kAllowBannedSdkMessages,  "Sdk", false); // can only be enabled in non-SHIPPING apps, for internal dev


#define ANKI_ENABLE_SDK_OVER_TCP 1


    IMPLEMENT_ENUM_INCREMENT_OPERATORS(UiConnectionType);


    CONSOLE_VAR(bool, kAcceptMessagesFromUI,  "UiComms", true);
    CONSOLE_VAR(bool, kAcceptMessagesFromSDK, "UiComms", true);
    CONSOLE_VAR(double, kPingSendFreq_ms, "UiComms", 1000.0); // 0 = never
    CONSOLE_VAR(uint32_t, kSdkStatusSendFreq, "UiComms", 1); // 0 = never


    bool IsExternalSdkConnection(UiConnectionType type)
    {
      switch(type)
      {
        case UiConnectionType::UI:          return false;
        case UiConnectionType::SdkOverUdp:  return true;
        case UiConnectionType::SdkOverTcp:  return true;
        case UiConnectionType::Switchboard: return false;
        default:
        {
          PRINT_NAMED_ERROR("IsExternalSdkConnection.BadType", "type = %d", (int)type);
          assert(0);
          return false;
        }
      }
    }


    ISocketComms* CreateSocketComms(UiConnectionType type, GameMessagePort* gameMessagePort,
                                    ISocketComms::DeviceId hostDeviceId, bool isSdkCommunicationEnabled)
    {
      // Note: Some SocketComms are deliberately null depending on the build platform, type etc.
#if FACTORY_TEST
      if(type != UiConnectionType::Switchboard)
      {
        return nullptr;
      }
#endif

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
          #if defined(NDEBUG)
            #error To enable SDK over UDP in SHIPPING builds requires support for closing the socket outside of SDK mode
          #endif
          return new UdpSocketComms(type);
        #else
          return nullptr;
        #endif
        }
        case UiConnectionType::SdkOverTcp:
        {
        #if ANKI_ENABLE_SDK_OVER_TCP
          return new TcpSocketComms(isSdkCommunicationEnabled);
        #else
          return nullptr;
        #endif
        }
        case UiConnectionType::Switchboard:
        {
          ISocketComms* comms = new TcpSocketComms(true, SWITCHBOARD_TCP_PORT);
          // Disable ping timeout disconnects
          comms->SetPingTimeoutForDisconnect(0, nullptr);
          return comms;
        }
        default:
        {
          assert(0);
          return nullptr;
        }
      }
    }


    UiMessageHandler::UiMessageHandler(u32 hostUiDeviceID, GameMessagePort* gameMessagePort)
      : _sdkStatus(this)
      , _hostUiDeviceID(hostUiDeviceID)
      , _messageCountGtE(0)
      , _messageCountEtG(0)
    {

      // Currently not supporting UI connections for any sim robot other
      // than the default ID
      #ifdef SIMULATOR
      const auto robotID = OSState::getInstance()->GetRobotID();
      if (robotID != DEFAULT_ROBOT_ID) {
        PRINT_NAMED_WARNING("UiMessageHandler.Ctor.SkippingUIConnections",
                            "RobotID: %d - Only DEFAULT_ROBOT_ID may accept UI connections",
                            robotID);

        for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i) {
          _socketComms[(uint32_t)i] = 0;
        }
        return;
      }
      #endif

      const bool isSdkCommunicationEnabled = IsSdkCommunicationEnabled();
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        auto& socket = _socketComms[(uint32_t)i];
        socket = CreateSocketComms(i, gameMessagePort, GetHostUiDeviceID(), isSdkCommunicationEnabled);

        // If UI disconnects due to timeout, disconnect Viz too
        if ((i == UiConnectionType::UI) && (socket != nullptr)) {
          ISocketComms::DisconnectCallback disconnectCallback = [this]() {
            _context->GetVizManager()->Disconnect();
          };
          socket->SetPingTimeoutForDisconnect(kPingTimeoutForDisconnect_ms, disconnectCallback);
        }
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


    Result UiMessageHandler::Init(CozmoContext* context, const Json::Value& config)
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

      _context = context;

      // We'll use this callback for simple events we care about
      auto commonCallback = std::bind(&UiMessageHandler::HandleEvents, this, std::placeholders::_1);

      // Subscribe to desired simple events
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::UiDeviceConnectionWrongVersion, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::UiDeviceConnectionSuccess, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::SetStopRobotOnSdkDisconnect, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::SetShouldAutoConnectToCubesAtStart, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::SetShouldAutoDisconnectFromCubesAtEnd, commonCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::TransferFile, commonCallback));

      // We'll use this callback for game to game events we care about (SDK to Unity or vice versa)
      auto gameToGameCallback = std::bind(&UiMessageHandler::HandleGameToGameEvents, this, std::placeholders::_1);

      // Subscribe to desired game to game events
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DeviceAccelerometerValuesRaw, gameToGameCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DeviceAccelerometerValuesUser, gameToGameCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::DeviceGyroValues, gameToGameCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::EnableDeviceIMUData, gameToGameCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::IsDeviceIMUSupported, gameToGameCallback));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::GameToGame, gameToGameCallback));

      // Subscribe to specific events
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::EnterSdkMode,
                                         std::bind(&UiMessageHandler::OnEnterSdkMode, this, std::placeholders::_1)));
      _signalHandles.push_back(Subscribe(ExternalInterface::MessageGameToEngineTag::ExitSdkMode,
                                         std::bind(&UiMessageHandler::OnExitSdkMode, this, std::placeholders::_1)));

      return RESULT_OK;
    }


    bool UiMessageHandler::ShouldHandleMessagesFromConnection(UiConnectionType type) const
    {
      switch(type)
      {
        case UiConnectionType::UI:          return kAcceptMessagesFromUI;
        case UiConnectionType::SdkOverUdp:  return kAcceptMessagesFromSDK;
        case UiConnectionType::SdkOverTcp:  return kAcceptMessagesFromSDK;
        case UiConnectionType::Switchboard: return true;
        default:
        {
          assert(0);
          return true;
        }
      }
    }


    bool UiMessageHandler::IsSdkCommunicationEnabled() const
    {
      return _sdkStatus.IsInExternalSdkMode() || kEnableSdkCommsAlways ||
             (_sdkStatus.IsInInternalSdkMode() && kEnableSdkCommsInInternalSdk);
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


    void UiMessageHandler::DeliverToGame(const ExternalInterface::MessageEngineToGame& message, DestinationId destinationId)
    {
      // There is almost always a connected device, so better to just always pack the message even if it won't be sent
      //if (GetNumConnectedDevicesOnAnySocket() > 0)
      {
        ANKI_CPU_PROFILE("UiMH::DeliverToGame");

        ++_messageCountEtG;

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

        const bool sendToEveryone = (destinationId == kDestinationIdEveryone);
        UiConnectionType connectionType = sendToEveryone ? UiConnectionType(0) : (UiConnectionType)destinationId;
        if (connectionType >= UiConnectionType::Count)
        {
          PRINT_NAMED_WARNING("UiMessageHandler.DeliverToGame.BadDestinationId", "Invalid destinationId %u = UiConnectionType '%s'", destinationId, EnumToString(connectionType));
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
        const size_t bytesUnpacked = message.Unpack(messagePtr, bytesRemaining);
        if (isSingleMessage && (bytesUnpacked != packetSize))
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

        HandleProcessedMessage(message, connectionType, bytesUnpacked, handleMessagesFromConnection);
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


    bool AlwaysHandleMessageTypeForNonSdkConnection(ExternalInterface::MessageGameToEngine::Tag messageTag)
    {
      // Return true for small subset of message types that we handle even if we're not listening to the UI connection
      // We still want to accept certain message types (e.g. enter/exit mode and console vars for debugging)

      using GameToEngineTag = ExternalInterface::MessageGameToEngineTag;
      switch (messageTag)
      {
        case GameToEngineTag::SetDebugConsoleVarMessage:    return true;
        case GameToEngineTag::RunDebugConsoleFuncMessage:   return true;
        case GameToEngineTag::GetDebugConsoleVarMessage:    return true;
        case GameToEngineTag::GetAllDebugConsoleVarMessage: return true;
        case GameToEngineTag::EnterSdkMode:                 return true;
        case GameToEngineTag::ExitSdkMode:                  return true;
        case GameToEngineTag::DeviceAccelerometerValuesRaw: return true;
        case GameToEngineTag::DeviceAccelerometerValuesUser: return true;
        case GameToEngineTag::DeviceGyroValues:             return true;
        case GameToEngineTag::IsDeviceIMUSupported:         return true;

        default:
          return false;
      }
    }


    bool IgnoreMessageTypeForSdkConnection(ExternalInterface::MessageGameToEngine::Tag messageTag)
    {
      if (kAllowBannedSdkMessages)
      {
        return false;
      }

      // Return true for any messages that we want to ignore (blacklist) from SDK usage

      using GameToEngineTag = ExternalInterface::MessageGameToEngineTag;
      switch (messageTag)
      {
        case GameToEngineTag::CalibrateMotors:                  return true;
        case GameToEngineTag::ReadToolCode:                     return true;
        case GameToEngineTag::IMURequest:                       return true;
        case GameToEngineTag::StartControllerTestMode:          return true;
        case GameToEngineTag::RawPWM:                           return true;
        case GameToEngineTag::RequestFeatureToggles:            return true;
        case GameToEngineTag::SetFeatureToggle:                 return true;
        case GameToEngineTag::UpdateFirmware:                   return true;
        case GameToEngineTag::ResetFirmware:                    return true;
        case GameToEngineTag::ControllerGains:                  return true;
        case GameToEngineTag::RestoreRobotFromBackup:           return true;
        case GameToEngineTag::RequestRobotRestoreData:          return true;
        case GameToEngineTag::WipeRobotGameData:                return true;
        case GameToEngineTag::RequestUnlockDataFromBackup:      return true;
        case GameToEngineTag::SetRobotImageSendMode:            return true;
        case GameToEngineTag::SaveImages:                       return true;
        case GameToEngineTag::SaveRobotState:                   return true;
        case GameToEngineTag::ExecuteTestPlan:                  return true;
        case GameToEngineTag::PlannerRunMode:                   return true;
        case GameToEngineTag::StartTestMode:                    return true;
        case GameToEngineTag::TransitionToNextOnboardingState:  return true;
        case GameToEngineTag::RequestSetUnlock:                 return true;
        case GameToEngineTag::GetJsonDasLogsMessage:            return true;
        case GameToEngineTag::SaveCalibrationImage:             return true;
        case GameToEngineTag::ClearCalibrationImages:           return true;
        case GameToEngineTag::ComputeCameraCalibration:         return true;
        case GameToEngineTag::NVStorageEraseEntry:              return true;
        case GameToEngineTag::NVStorageWipeAll:                 return true;
        case GameToEngineTag::NVStorageWriteEntry:              return true;
        case GameToEngineTag::NVStorageClearPartialPendingWriteEntry:  return true;
        case GameToEngineTag::NVStorageReadEntry:               return true;
        case GameToEngineTag::EnterSdkMode:                     return true;
        case GameToEngineTag::ExitSdkMode:                      return true;
        case GameToEngineTag::PerfMetricCommand:                return true;
        case GameToEngineTag::PerfMetricGetStatus:              return true;
        default:
          return false;
      }
    }


    void UiMessageHandler::HandleProcessedMessage(const ExternalInterface::MessageGameToEngine& message,
                                UiConnectionType connectionType, size_t messageSize, bool handleMessagesFromConnection)
    {
      ++_messageCountGtE;

      const ExternalInterface::MessageGameToEngine::Tag messageTag = message.GetTag();
      if (!handleMessagesFromConnection)
      {
        // We still want to accept certain message types (e.g. console vars to allow a connection to enable itself)
        if (!AlwaysHandleMessageTypeForConnection(messageTag))
        {
          return;
        }
      }

      const bool isExternalSdkConnection = IsExternalSdkConnection(connectionType);
      if (isExternalSdkConnection && IgnoreMessageTypeForSdkConnection(messageTag))
      {
        // Ignore - this message type is blacklisted from SDK usage
        PRINT_NAMED_WARNING("sdk.bannedmessage", "%s", MessageGameToEngineTagToString(messageTag));
        return;
      }

      if (_sdkStatus.IsInExternalSdkMode() && !isExternalSdkConnection)
      {
        // Accept only a limited set of messages (e.g. enter/exit mode)
        if (!AlwaysHandleMessageTypeForNonSdkConnection(messageTag))
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

      if (isExternalSdkConnection || _sdkStatus.IsInInternalSdkMode())
      {
        _sdkStatus.OnRecvMessage(message, messageSize);
      }

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

      DEV_ASSERT(nullptr == _context || _context->IsEngineThread(),
                 "UiMessageHandler.GameToEngineRef.BroadcastOffEngineThread");

      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageGameToEngine)


    void UiMessageHandler::Broadcast(ExternalInterface::MessageGameToEngine&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastMove_GToE");

      DEV_ASSERT(nullptr == _context || _context->IsEngineThread(),
                 "UiMessageHandler.GameToEngineRval.BroadcastOffEngineThread");

      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToEngine.Broadcast(AnkiEvent<ExternalInterface::MessageGameToEngine>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageGameToEngine &&)


    // Called from any not main thread and dealt with during the update.
    void UiMessageHandler::BroadcastDeferred(const ExternalInterface::MessageGameToEngine& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastDeferred_GToE");

      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgsToEngine.emplace_back(message);
    }


    void UiMessageHandler::BroadcastDeferred(ExternalInterface::MessageGameToEngine&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastDeferredMove_GToE");

      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgsToEngine.emplace_back(std::move(message));
    }


    // Broadcasting MessageEngineToGame messages also delivers them out of the engine
    void UiMessageHandler::Broadcast(const ExternalInterface::MessageEngineToGame& message)
    {
      ANKI_CPU_PROFILE("UiMH::Broadcast_EToG");

      DEV_ASSERT(nullptr == _context || _context->IsEngineThread(),
                 "UiMessageHandler.EngineToGameRef.BroadcastOffEngineThread");

      DeliverToGame(message);
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), static_cast<u32>(message.GetTag()), message));
    } // Broadcast(MessageEngineToGame)


    void UiMessageHandler::Broadcast(ExternalInterface::MessageEngineToGame&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastMove_EToG");

      DEV_ASSERT(nullptr == _context || _context->IsEngineThread(),
                 "UiMessageHandler.EngineToGameRval.BroadcastOffEngineThread");

      DeliverToGame(message);
      u32 type = static_cast<u32>(message.GetTag());
      _eventMgrToGame.Broadcast(AnkiEvent<ExternalInterface::MessageEngineToGame>(
        BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
    } // Broadcast(MessageEngineToGame &&)


    void UiMessageHandler::BroadcastDeferred(const ExternalInterface::MessageEngineToGame& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastDeferred_EToG");

      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgsToGame.emplace_back(message);
    }


    void UiMessageHandler::BroadcastDeferred(ExternalInterface::MessageEngineToGame&& message)
    {
      ANKI_CPU_PROFILE("UiMH::BroadcastDeferredMove_EToG");

      std::lock_guard<std::mutex> lock(_mutex);
      _threadedMsgsToGame.emplace_back(std::move(message));
    }


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
          _connectionSource = i;
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
        _connectionSource = UiConnectionType::Count;
      }

      return retVal;
    } // ProcessMessages()


    Result UiMessageHandler::Update()
    {
      ANKI_CPU_PROFILE("UiMH::Update");

      ++_updateCount;

      // Update all the comms

      const double currTime_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
      const bool sendPingThisTick = (kPingSendFreq_ms > 0.0) && (currTime_ms - _lastPingTime_ms > kPingSendFreq_ms);

      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms)
        {
          socketComms->Update();

          if(sendPingThisTick && (socketComms->GetNumConnectedDevices() > 0))
          {
            // Ping the connection to let them know we're still here

            ANKI_CPU_PROFILE("UiMH::Update::SendPing");

            ExternalInterface::Ping outPing(socketComms->NextPingCounter(), currTime_ms, false);
            ExternalInterface::MessageEngineToGame message(std::move(outPing));
            DeliverToGame(message, (DestinationId)i);
            _lastPingTime_ms = currTime_ms;
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
              // Force connection to first UI device if not already connected
              if (ConnectToUiDevice(deviceId, i))
              {
                PRINT_CH_INFO("UiComms", "UiMessageHandler.Update.Connected",
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
        ANKI_CPU_PROFILE("UiMH::BroadcastThreadedMessagesToEngine");
        std::lock_guard<std::mutex> lock(_mutex);
        if( _threadedMsgsToEngine.size() > 0 )
        {
          for(auto& threaded_msg : _threadedMsgsToEngine) {
            Broadcast(std::move(threaded_msg));
          }
          _threadedMsgsToEngine.clear();
        }
      }

      {
        ANKI_CPU_PROFILE("UiMH::BroadcastThreadedMessagesToGame");
        std::lock_guard<std::mutex> lock(_mutex);
        if( _threadedMsgsToGame.size() > 0 )
        {
          for(auto& threaded_msg : _threadedMsgsToGame) {
            Broadcast(std::move(threaded_msg));
          }
          _threadedMsgsToGame.clear();
        }
      }

      UpdateSdk();

      return lastResult;
    } // Update()


    void UiMessageHandler::UpdateSdk()
    {
      if (_sdkStatus.IsInExternalSdkMode())
      {
        const ISocketComms* sdkSocketComms = GetSdkSocketComms();
        DEV_ASSERT(sdkSocketComms, "Sdk.InModeButNoComms");

        if (!sdkSocketComms)
        {
          return;
        }

        _sdkStatus.UpdateConnectionStatus(sdkSocketComms);

        const bool sendStatusThisTick = (kSdkStatusSendFreq > 0) && ((_updateCount % kSdkStatusSendFreq) == 0);

        if (sendStatusThisTick)
        {
          // Send status to the UI comms

          const double currentTime_s = _sdkStatus.GetCurrentTime_s();

          ExternalInterface::SdkConnectionStatus connectionStatus(_sdkStatus.GetSdkBuildVersion(),
                                                                  kBuildVersion,
                                                                  _sdkStatus.NumCommandsOverConnection(),
                                                                  _sdkStatus.TimeInCurrentConnection_s(currentTime_s),
                                                                  _sdkStatus.IsConnected(),
                                                                  _sdkStatus.IsWrongSdkVersion());

          std::vector<std::string> sdkStatusStrings;
          sdkStatusStrings.reserve(SdkStatusTypeNumEntries);
          for (uint32_t i=0; i < SdkStatusTypeNumEntries; ++i)
          {
            sdkStatusStrings.push_back( _sdkStatus.GetStatus(SdkStatusType(i)) );
          }

          ExternalInterface::SdkStatus sdkStatus(std::move(connectionStatus),
                                                 std::move(sdkStatusStrings),
                                                 _sdkStatus.NumTimesConnected(),
                                                 _sdkStatus.TimeInMode_s(currentTime_s),
                                                 _sdkStatus.TimeSinceLastSdkMessage_s(currentTime_s),
                                                 _sdkStatus.TimeSinceLastSdkCommand_s(currentTime_s));

          ExternalInterface::MessageEngineToGame message(std::move(sdkStatus));

          DeliverToGame(message, (DestinationId)UiConnectionType::UI);
        }
      }
      else
      {
      #if ANKI_DEV_CHEATS
        static bool sWasSdkCommsAlwaysEnabled = kEnableSdkCommsAlways;
        const bool wasAlwaysEnabledToggled = (sWasSdkCommsAlwaysEnabled != kEnableSdkCommsAlways);
        if (wasAlwaysEnabledToggled)
        {
          sWasSdkCommsAlwaysEnabled = kEnableSdkCommsAlways;
          UpdateIsSdkCommunicationEnabled();
        }
        if (IsSdkCommunicationEnabled() || wasAlwaysEnabledToggled)
        {
          const ISocketComms* sdkSocketComms = GetSdkSocketComms();

          if (sdkSocketComms)
          {
            _sdkStatus.UpdateConnectionStatus(sdkSocketComms);
          }
        }
      #endif // ANKI_DEV_CHEATS
      }
    }


    bool UiMessageHandler::ConnectToUiDevice(ISocketComms::DeviceId deviceId, UiConnectionType connectionType)
    {
      ISocketComms* socketComms = GetSocketComms(connectionType);

      const bool success = socketComms ? socketComms->ConnectToDeviceByID(deviceId) : false;

      std::array<uint8_t, 16> toGameCLADHash;
      std::copy(std::begin(messageEngineToGameHash), std::end(messageEngineToGameHash), std::begin(toGameCLADHash));

      std::array<uint8_t, 16> toEngineCLADHash;
      std::copy(std::begin(messageGameToEngineHash), std::end(messageGameToEngineHash), std::begin(toEngineCLADHash));

      // kReservedForTag is for future proofing - if we need to increase tag size to 16 bits, the
      const uint8_t kReservedForTag = 0;
      ExternalInterface::UiDeviceConnected deviceConnected(kReservedForTag,
                                                           connectionType,
                                                           deviceId,
                                                           success,
                                                           toGameCLADHash,
                                                           toEngineCLADHash,
                                                           kBuildVersion);

      Broadcast( ExternalInterface::MessageEngineToGame(std::move(deviceConnected)) );

      if (success)
      {
        // Ask Robot to send per-robot settings to Game/SDK
        Broadcast( ExternalInterface::MessageGameToEngine(ExternalInterface::RequestRobotSettings()) );
      }

      return success;
    }

    void TransferCodeLabFile(const ExternalInterface::TransferFile& msg, Util::Data::DataPlatform* dataPlatform)
    {
    #if ANKI_DEV_CHEATS
      DEV_ASSERT_MSG((msg.fileType == ExternalInterface::FileType::CodeLab), "TransferCodeLabFile.WrongFileType",
                     "Incorrect FileType %d '%s'", (int)msg.fileType, EnumToString(msg.fileType));

      static uint16_t sExpectedNextPart = std::numeric_limits<uint16_t>::max();
      static uint16_t sExpectedPartCount = std::numeric_limits<uint16_t>::max();

      std::string full_path = dataPlatform->pathToResource(Util::Data::Scope::Cache, msg.filename);

      // Special signal for CodeLab to nuke the directory
      if ((msg.filePart == 0) && (msg.numFileParts == 0) && (msg.fileBytes.size() == 0))
      {
        PRINT_NAMED_INFO("TransferCodeLabFile.DeleteCache", "Delete Cache '%s'", full_path.c_str());
        Util::FileUtils::RemoveDirectory(full_path);
        return;
      }

      // Verify this is the chunk we're waiting for.
      if( sExpectedNextPart == msg.filePart)
      {
        ++sExpectedNextPart;
      }
      else if( msg.filePart == 0 )
      {
        if (sExpectedNextPart != sExpectedPartCount)
        {
          PRINT_NAMED_ERROR("TransferCodeLabFile.FailedToComplete", "Got: 0, Expected: %u/%u", sExpectedNextPart, sExpectedPartCount);
        }

        // New file - delete it if it already exists
        if( Util::FileUtils::FileExists(full_path))
        {
          Util::FileUtils::DeleteFile(full_path);
        }
        sExpectedNextPart = 1;
        sExpectedPartCount = msg.numFileParts;
      }
      else
      {
        PRINT_NAMED_ERROR("TransferCodeLabFile.UnexpectedPart", "Got: %d, Expected: %d", (int)msg.filePart, (int)sExpectedNextPart);
        return;
      }

      const bool append = msg.filePart != 0;
      const bool res1 = Util::FileUtils::CreateDirectory(full_path, true, true);
      const bool res2 = Util::FileUtils::WriteFile(full_path, msg.fileBytes, append);
      if (!res1 || !res2)
      {
        PRINT_NAMED_WARNING("TransferCodeLabFile.WriteFailed", "load '%s' dir_res=%d write_res=%d part %u of %u", full_path.c_str(), (int)res1, (int)res2, msg.filePart, msg.numFileParts);
      }
      else if (sExpectedNextPart == sExpectedPartCount)
      {
        // This was the last part - file is loaded
        PRINT_NAMED_INFO("TransferCodeLabFile.Complete", "Loaded all %u parts of '%s'", msg.numFileParts, full_path.c_str());
      }
    #endif // ANKI_DEV_CHEATS
    }

    void UiMessageHandler::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      switch (event.GetData().GetTag())
      {
        case ExternalInterface::MessageGameToEngineTag::UiDeviceConnectionWrongVersion:
        {
          const ExternalInterface::UiDeviceConnectionWrongVersion& msg = event.GetData().Get_UiDeviceConnectionWrongVersion();
          if (IsExternalSdkConnection(msg.connectionType))
          {
            _sdkStatus.OnWrongVersion(msg);
            ISocketComms* socketComms = GetSocketComms(msg.connectionType);
            if (socketComms)
            {
              socketComms->DisconnectDeviceByID(msg.deviceID);
            }
          }
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::UiDeviceConnectionSuccess:
        {
          const ExternalInterface::UiDeviceConnectionSuccess& msg = event.GetData().Get_UiDeviceConnectionSuccess();
          if (IsExternalSdkConnection(msg.connectionType))
          {
            _sdkStatus.OnConnectionSuccess(msg);

          }
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice:
        {
          const ExternalInterface::ConnectToUiDevice& msg = event.GetData().Get_ConnectToUiDevice();
          const ISocketComms::DeviceId deviceID = msg.deviceID;

          const bool success = ConnectToUiDevice(deviceID, msg.connectionType);
          if(success) {
            PRINT_CH_INFO("UiComms", "UiMessageHandler.HandleEvents", "Connected to %s device %d!",
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
            PRINT_CH_INFO("UiComms", "UiMessageHandler.ProcessMessage", "Disconnected from %s device %d!",
                             EnumToString(msg.connectionType), deviceId);
          }

          break;
        }
        case ExternalInterface::MessageGameToEngineTag::SetStopRobotOnSdkDisconnect:
        {
          const ExternalInterface::SetStopRobotOnSdkDisconnect& msg = event.GetData().Get_SetStopRobotOnSdkDisconnect();
          _sdkStatus.SetStopRobotOnDisconnect(msg.doStop);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::SetShouldAutoConnectToCubesAtStart:
        {
          const ExternalInterface::SetShouldAutoConnectToCubesAtStart& msg = event.GetData().Get_SetShouldAutoConnectToCubesAtStart();
          _sdkStatus.SetShouldAutoConnectToCubes(msg.doAutoConnect);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::SetShouldAutoDisconnectFromCubesAtEnd:
        {
          const ExternalInterface::SetShouldAutoDisconnectFromCubesAtEnd& msg = event.GetData().Get_SetShouldAutoDisconnectFromCubesAtEnd();
          _sdkStatus.SetShouldAutoDisconnectFromCubes(msg.doAutoDisconnect);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::TransferFile:
        {
          const ExternalInterface::TransferFile& msg = event.GetData().Get_TransferFile();
          if( msg.fileType == ExternalInterface::FileType::CodeLab)
          {
            TransferCodeLabFile(msg, _context->GetDataPlatform());
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

    void UiMessageHandler::HandleGameToGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      const bool isFromSdk = IsExternalSdkConnection(_connectionSource);
      UiConnectionType destType = isFromSdk ? UiConnectionType::UI : UiConnectionType::SdkOverTcp;
      DestinationId destinationId = (DestinationId)(destType);

      // Note we have to copy msg to a non-const temporary as MessageEngineToGame constructor only takes r-values
      switch (event.GetData().GetTag())
      {
        case ExternalInterface::MessageGameToEngineTag::DeviceAccelerometerValuesRaw:
        {
          ExternalInterface::DeviceAccelerometerValuesRaw msg = event.GetData().Get_DeviceAccelerometerValuesRaw();
          DeliverToGame(ExternalInterface::MessageEngineToGame(std::move(msg)), destinationId);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::DeviceAccelerometerValuesUser:
        {
          ExternalInterface::DeviceAccelerometerValuesUser msg = event.GetData().Get_DeviceAccelerometerValuesUser();
          DeliverToGame(ExternalInterface::MessageEngineToGame(std::move(msg)), destinationId);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::DeviceGyroValues:
        {
          ExternalInterface::DeviceGyroValues msg = event.GetData().Get_DeviceGyroValues();
          DeliverToGame(ExternalInterface::MessageEngineToGame(std::move(msg)), destinationId);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::EnableDeviceIMUData:
        {
          ExternalInterface::EnableDeviceIMUData msg = event.GetData().Get_EnableDeviceIMUData();
          DeliverToGame(ExternalInterface::MessageEngineToGame(std::move(msg)), destinationId);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::IsDeviceIMUSupported:
        {
          ExternalInterface::IsDeviceIMUSupported msg = event.GetData().Get_IsDeviceIMUSupported();
          DeliverToGame(ExternalInterface::MessageEngineToGame(std::move(msg)), destinationId);
          break;
        }
        case ExternalInterface::MessageGameToEngineTag::GameToGame:
        {
          ExternalInterface::GameToGame msg = event.GetData().Get_GameToGame();
          DeliverToGame(ExternalInterface::MessageEngineToGame(std::move(msg)), destinationId);
          break;
        }
        default:
        {
          PRINT_STREAM_ERROR("HandleGameToGameEvents",
                             "Subscribed to unhandled event of type "
                             << ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) << "!");
        }
      }
    }

    void UiMessageHandler::OnEnterSdkMode(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      const ExternalInterface::EnterSdkMode& msg = event.GetData().Get_EnterSdkMode();
      _sdkStatus.EnterMode(msg.isExternalSdkMode);

      UpdateIsSdkCommunicationEnabled();
    }


    void UiMessageHandler::OnExitSdkMode(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      const ExternalInterface::ExitSdkMode& msg = event.GetData().Get_ExitSdkMode();

      // Note: Robot's message handler also handles this event, and that disconnects the robot
      //       (that's how we ensure that we restore the robot to a safe default state)
      DoExitSdkMode(msg.isExternalSdkMode);
    }


    void UiMessageHandler::DoExitSdkMode(bool isExternalSdkMode)
    {
      _sdkStatus.ExitMode(isExternalSdkMode);
      UpdateIsSdkCommunicationEnabled();
    }


    void UiMessageHandler::UpdateIsSdkCommunicationEnabled()
    {
      const bool isSdkCommunicationEnabled = IsSdkCommunicationEnabled();

      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
        if (IsExternalSdkConnection(i))
        {
          ISocketComms* socketComms = GetSocketComms(i);
          if (socketComms)
          {
            socketComms->EnableConnection(isSdkCommunicationEnabled);
          }
        }
      }
    }


    bool UiMessageHandler::HasDesiredNumUiDevices() const
    {
      for (UiConnectionType i=UiConnectionType(0); i < UiConnectionType::Count; ++i)
      {
	// Ignore switchboard's numDesiredDevices
	if(i == UiConnectionType::Switchboard)
	{
	  continue;
	}
	
        const ISocketComms* socketComms = GetSocketComms(i);
        if (socketComms && socketComms->HasDesiredDevices())
        {
          return true;
        }
      }

      return false;
    }


    void UiMessageHandler::OnRobotDisconnected(uint32_t robotID)
    {
      if (_sdkStatus.IsInInternalSdkMode())
      {
        DoExitSdkMode(false);
      }
      if (_sdkStatus.IsInExternalSdkMode())
      {
        DoExitSdkMode(true);
      }
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
