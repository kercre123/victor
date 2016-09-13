/**
* File: messageHandler
*
* Author: damjan stulic
* Created: 9/8/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/comms/robotConnectionManager.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/messaging/basestation/IComms.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "json/json.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/transport/transportAddress.h"

namespace Anki {
namespace Cozmo {
namespace RobotInterface {

MessageHandler::MessageHandler()
: _robotManager(nullptr)
, _isInitialized(false)
{
}
  
MessageHandler::~MessageHandler() = default;

void MessageHandler::Init(const Json::Value& config, RobotManager* robotMgr, const CozmoContext* context)
{
  const char *ipString = config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString();
  int port = config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt();
  if (port < 0 || port >= 0x10000) {
    PRINT_NAMED_ERROR("RobotInterface.MessageHandler.Init", "Failed to initialize RobotComms; bad port %d", port);
    return;
  }
  
  Anki::Util::TransportAddress address(ipString, static_cast<uint16_t>(port));
  PRINT_STREAM_DEBUG("RobotInterface.MessageHandler.Init", "Initializing on address: " << address << ": " << address.GetIPAddress() << ":" << address.GetIPPort() << "; originals: " << ipString << ":" << port);
  
  _robotManager = robotMgr;
  _robotConnectionManager.reset(new RobotConnectionManager(_robotManager));
  _robotConnectionManager->Init();
  
  _isInitialized = true;

  if (context->GetExternalInterface() != nullptr) {
    auto helper = MakeAnkiEventUtil(*context->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ReliableTransportRunMode>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ExitSdkMode>();
  }
}

void MessageHandler::ProcessMessages()
{
  ANKI_CPU_PROFILE("MessageHandler::ProcessMessages");
  
  if(_isInitialized)
  {
    _robotConnectionManager->Update();
    
    std::vector<uint8_t> nextData;
    while (_robotConnectionManager->PopData(nextData))
    {
      // If we don't have a robot to care about this message, throw it away
      Robot* destRobot = _robotManager->GetFirstRobot();
      if (nullptr == destRobot)
      {
        continue;
      }
      
      const size_t dataSize = nextData.size();
      if (dataSize <= 0)
      {
        PRINT_NAMED_ERROR("MessageHandler.ProcessMessages","Tried to process message of invalid size");
        continue;
      }

      auto robotId = destRobot->GetID();

      // see if message type should be filtered out based on potential firmware mismatch
      const RobotInterface::RobotToEngineTag msgType = static_cast<RobotInterface::RobotToEngineTag>(nextData.data()[0]);
      if (_robotManager->ShouldFilterMessage(robotId, msgType)) {
        continue;
      }

      RobotInterface::RobotToEngine message;
      const size_t unpackSize = message.Unpack(nextData.data(), dataSize);
      if (unpackSize != nextData.size()) {
        PRINT_NAMED_ERROR("RobotMessageHandler.MessageUnpack", "Message unpack error, tag %s expecting %zu but have %zu",
                          RobotToEngineTagToString(msgType), unpackSize, dataSize);
        continue;
      }
      
#if ANKI_DEV_CHEATS
      if (nullptr != DevLoggingSystem::GetInstance())
      {
        DevLoggingSystem::GetInstance()->LogMessage(message);
      }
#endif
      
      bool fatalErrorNeedsDisconnect = false;
      // Special case handling of fatal error report messages; means we need to drop connection.
      if (RobotInterface::RobotToEngineTag::robotError == message.GetTag())
      {
        const RobotErrorReport& report = message.Get_robotError();
        const char* errorCode = RobotErrorCodeToString(report.error);
        if (report.fatal)
        {
          fatalErrorNeedsDisconnect = true;
          Util::sEventF("robot.error.fatal", {{DDATA,errorCode}}, "Error type %s", errorCode);
        }
        else
        {
          Util::sEventF("robot.error.nonfatal", {{DDATA,errorCode}}, "Error type %s", errorCode);
        }
      }
      
      Broadcast(robotId, std::move(message));
      
      if (fatalErrorNeedsDisconnect)
      {
        // Anything sent after a fatal error could have corrupted data, so drop it
        _robotConnectionManager->ClearData();
        // This connection can't safely be maintained so be done with it
        Disconnect();
        // This break isn't strictly necessary since we just cleared the remaining data, but it's more explicit
        break;
      }
    }
  }
}

Result MessageHandler::SendMessage(const RobotID_t robotId, const RobotInterface::EngineToRobot& msg, bool reliable, bool hot)
{
  if (!_isInitialized || !_robotConnectionManager->IsValidConnection())
  {
    return RESULT_FAIL;
  }
  
  if (_robotManager->ShouldFilterMessage(robotId, msg.GetTag()))
  {
    return RESULT_FAIL;
  }

#if ANKI_DEV_CHEATS
  if (nullptr != DevLoggingSystem::GetInstance())
  {
    DevLoggingSystem::GetInstance()->LogMessage(msg);
  }
#endif

  const auto expectedSize = msg.Size();
  std::vector<uint8_t> messageData(msg.Size());
  const auto packedSize = msg.Pack(messageData.data(), expectedSize);
  ASSERT_NAMED(packedSize == expectedSize, "MessageHandler.SendMessage.MessageSizeMismatch");
  if (packedSize != expectedSize)
  {
    return RESULT_FAIL;
  }
  
  const bool success = _robotConnectionManager->SendData(messageData.data(), Util::numeric_cast<unsigned int>(packedSize));
  if (!success)
  {
    return RESULT_FAIL;
  }
  
  return RESULT_OK;
}

void MessageHandler::Broadcast(const uint32_t robotId, const RobotInterface::RobotToEngine& message)
{
  ANKI_CPU_PROFILE("Broadcast_R2E");
  
  u32 type = static_cast<u32>(message.GetTag());
  _eventMgr.Broadcast(robotId, AnkiEvent<RobotInterface::RobotToEngine>(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, message));
}

void MessageHandler::Broadcast(const uint32_t robotId, RobotInterface::RobotToEngine&& message)
{
  ANKI_CPU_PROFILE("Broadcast_R2E");
  
  u32 type = static_cast<u32>(message.GetTag());
  _eventMgr.Broadcast(robotId, AnkiEvent<RobotInterface::RobotToEngine>(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
}
  
Result MessageHandler::AddRobotConnection(const ExternalInterface::ConnectToRobot& connectMsg)
{
  int port = !connectMsg.isSimulated ? Anki::Cozmo::ROBOT_RADIO_BASE_PORT : Anki::Cozmo::ROBOT_RADIO_BASE_PORT + connectMsg.robotID;
  Anki::Util::TransportAddress address((const char*)connectMsg.ipAddress.data(), static_cast<uint16_t>(port));
  _robotConnectionManager->Connect(address);
  
  return RESULT_OK;
}
  
void MessageHandler::Disconnect()
{
  _robotConnectionManager->DisconnectCurrent();
}
 
const Util::Stats::StatsAccumulator& MessageHandler::GetQueuedTimes_ms() const
{
  return _robotConnectionManager->GetQueuedTimes_ms();
}
  
template<>
void MessageHandler::HandleMessage(const ExternalInterface::ReliableTransportRunMode& msg)
{
  _robotConnectionManager->SetReliableTransportRunMode(msg.isSync);
}
  
template<>
void MessageHandler::HandleMessage(const ExternalInterface::ExitSdkMode& msg)
{
  // Force robot to disconnect when leaving the sdk - ensures they will be in a default initialized state for main app
  Disconnect();
}
  
} // end namespace RobotInterface
} // end namespace Cozmo
} // end namespace Anki
